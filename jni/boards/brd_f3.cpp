// Taito F3 audio (68000 + ES5505 + ES5510)

/*
 All about shared RAM and the "link-level" protocol
 
 The 68020 sees commram as a contiguous byte array.  We also handle it that
 way here since we're emulating the 68020's driver program.  The 68000 sees it
 mapped every other byte, using the 680x0 MOVEP instruction as with 8-bit 
 peripherals.  This comes into play for the command read/write addresses, which
 are always stored from the perspective of the 68000 rather than the 68020.

 Map (addresses in hex):
 000-3FF: 1K  : command strings are written here according to the protocol  

 480-481: WORD: current 68020 command write address (*2 from the 020 perspective)
 482-483: WORD: current 68000 command read address  (*2 from the 020 perspective)
 7F8-7FF: BYTE: 4 bytes of master volume settings

 The command protocol works like this:
 1) The 68020 reads the word at 0x480.  Dividing by 2 gives a byte address in
    the first 4k of commram to start writing commands to.
 2) The '020 then writes bytes starting at that address.  Command strings can
    be anywhere from 3 to 1024 bytes, at least in theory.
 3) If the '020 runs out of the first 1k of commram, writing begins again at
    address zero.  The 68000 automatically knows to wrap at that point.
 4) The byte address then should be multiplied by 2 again and written to the
    word at commram address 0x480.  The 68000 will see that the 020 has written
    past where it last read from (which is stored at 0x482) and process the 
    command string.
*/


#include "m1snd.h"

#define M68K_CLOCK (16000000)

static void F3_Init(long srate);
static void F3_SendCmd(int cmda, int cmdb);

static unsigned int f3_read_memory_8(unsigned int address);
static unsigned int f3_read_memory_16(unsigned int address);
static unsigned int f3_read_memory_32(unsigned int address);
static void f3_write_memory_8(unsigned int address, unsigned int data);
static void f3_write_memory_16(unsigned int address, unsigned int data);
static void f3_write_memory_32(unsigned int address, unsigned int data);

static int f3_irq_callback(int int_level);

static unsigned char f3work[0x20000];
static unsigned char commram[0x2000];
static unsigned char bExtCodeBuf[128];

static int lastcmd = 1, lastcode;

static TimerT *timer_68681=(TimerT *)NULL;
static int timer_mode,m68681_imr;
static int counter,imr_status;
static int vector_reg;
static data16_t es5510_dsp_ram[0x200];
static data32_t	es5510_gpr[0xc0];
static data32_t	es5510_gpr_latch;
static int es_tmp=1;

#define M68681_CLOCK	2000000 /* Actually X1, not the main clock */

enum { TIMER_SINGLESHOT, TIMER_PULSE };

static struct ES5505interface es5505_interface =
{
	1,					/* total number of chips */
	{ (30476100/2) },		/* freq */
	{ RGN_SAMP2 },	/* Bank 0: Unused by F3 games? */
	{ RGN_SAMP1 },	/* Bank 1: All games seem to use this */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },		/* master volume */
};

static M168KT f3_readwritemem =
{
	f3_read_memory_8,
	f3_read_memory_16,
	f3_read_memory_32,
	f3_write_memory_8,
	f3_write_memory_16,
	f3_write_memory_32,
};

void es5506_update(int num, INT16 **buffer, int length);

static double TIME_IN_CYCLES(double cycles, int cpu)
{
	return (cycles / (double)M68K_CLOCK);
}

static void timer_callback(int param)
{
	if (timer_mode==TIMER_SINGLESHOT) timer_68681=NULL;

	/* Only cause IRQ if the mask is set to allow it */
	if (m68681_imr&8) 
	{
		m68k_set_int_ack_callback(f3_irq_callback);	
		m68k_set_irq(M68K_IRQ_6);
		imr_status|=0x8;
	}
}

static READ16_HANDLER(f3_68681_r)
{
	if (offset==0x5) {
		int ret=imr_status;
//		printf("rd IMR (PC=%x)\n", m68k_get_reg(NULL, M68K_REG_PC));
		imr_status=0;
		return ret;
	}

	if (offset==0xe)
		return 1;

	/* IRQ ack */
	if (offset==0xf) {
//		printf("ACK (PC=%x)\n", m68k_get_reg(NULL, M68K_REG_PC));
		m68k_set_irq(M68K_IRQ_NONE);
		return 0;
	}

	return 0xff;
}

static WRITE16_HANDLER(f3_68681_w)
{
//	if (offset != 0xa || data != 0x8)
//		printf("write 68681: offset %x data %x mask %x PC=%x\n", offset, data, mem_mask, m68k_get_reg(NULL, M68K_REG_PC));
	switch (offset) {
		case 0x04: /* ACR */
			switch ((data>>4)&7) {
				case 0:
//					logerror("Counter:  Unimplemented external IP2\n");
					break;
				case 1:
//					logerror("Counter:  Unimplemented TxCA - 1X clock of channel A\n");
					break;
				case 2:
//					logerror("Counter:  Unimplemented TxCB - 1X clock of channel B\n");
					break;
				case 3:
//					logerror("Counter:  X1/Clk - divided by 16, counter is %04x, so interrupt every %d cycles\n",counter,(M68K_CLOCK/M68681_CLOCK)*counter*16);
					if (timer_68681) timer_remove(timer_68681);
					timer_mode=TIMER_SINGLESHOT;
					timer_68681=timer_set(TIME_IN_CYCLES((M68K_CLOCK/M68681_CLOCK)*counter*16,1), 0, timer_callback);
					break;
				case 4:
//					logerror("Timer:  Unimplemented external IP2\n");
					break;
				case 5:
//					logerror("Timer:  Unimplemented external IP2/16\n");
					break;
				case 6:
//					logerror("Timer:  X1/Clk, counter is %04x, so interrupt every %d cycles\n",counter,(M68K_CLOCK/M68681_CLOCK)*counter);
					if (timer_68681) timer_remove(timer_68681);
					timer_mode=TIMER_PULSE;
					timer_68681=timer_pulse(TIME_IN_CYCLES((M68K_CLOCK/M68681_CLOCK)*counter,1), 0, timer_callback);
					break;
				case 7:
//					logerror("Timer:  Unimplemented X1/Clk - divided by 16\n");
					break;
			}
			break;

		case 0x05: /* IMR */
//			logerror("68681:  %02x %02x\n",offset,data&0xff);
			m68681_imr=data&0xff;
			break;

		case 0x06: /* CTUR */
			counter=((data&0xff)<<8)|(counter&0xff);
			break;
		case 0x07: /* CTLR */
			counter=(counter&0xff00)|(data&0xff);
			break;
		case 0x08: break; /* MR1B (Mode register B) */
		case 0x09: break; /* CSRB (Clock select register B) */
		case 0x0a: break; /* CRB (Command register B) */
		case 0x0b: break; /* TBB (Transmit buffer B) */
		case 0x0c: /* IVR (Interrupt vector) */
			vector_reg=data&0xff;
			break;
		case 0x0f:	// written when dying
//			logerror("68681:  %02x %02x (PC=%x)\n",offset,data&0xff, m68k_get_reg(NULL, M68K_REG_PC));
			break;
		default:
//			logerror("68681:  %02x %02x\n",offset,data&0xff);
			break;
	}
}

READ16_HANDLER(es5510_dsp_r)
{
//	logerror("%06x: DSP read offset %04x (data is %04x)\n",activecpu_get_pc(),offset,es5510_dsp_ram[offset]);
//	if (es_tmp) return es5510_dsp_ram[offset];
/*
	switch (offset) {
		case 0x00: return (es5510_gpr_latch>>16)&0xff;
		case 0x01: return (es5510_gpr_latch>> 8)&0xff;
		case 0x02: return (es5510_gpr_latch>> 0)&0xff;
		case 0x03: return 0;
	}
*/
//	offset<<=1;

//if (offset<7 && es5510_dsp_ram[0]!=0xff) return rand()%0xffff;

	if (offset==0x12) return 0;

//	if (offset>4)
	if (offset==0x16) return 0x27;

	return es5510_dsp_ram[offset];
}

WRITE16_HANDLER(es5510_dsp_w)
{
	UINT8 *snd_mem = (UINT8 *)memory_region(REGION_SOUND1);

//	if (offset>4 && offset!=0x80  && offset!=0xa0  && offset!=0xc0  && offset!=0xe0)
//		logerror("%06x: DSP write offset %04x %04x\n",activecpu_get_pc(),offset,data);

	COMBINE_DATA(&es5510_dsp_ram[offset]);

	switch (offset) {
		case 0x00: es5510_gpr_latch=(es5510_gpr_latch&0x00ffff)|((data&0xff)<<16);
		case 0x01: es5510_gpr_latch=(es5510_gpr_latch&0xff00ff)|((data&0xff)<< 8);
		case 0x02: es5510_gpr_latch=(es5510_gpr_latch&0xffff00)|((data&0xff)<< 0);
		case 0x03: break;

		case 0x80: /* Read select - GPR + INSTR */
	//		logerror("ES5510:  Read GPR/INSTR %06x (%06x)\n",data,es5510_gpr[data]);

			/* Check if a GPR is selected */
			if (data<0xc0) {
				es_tmp=0;
				es5510_gpr_latch=es5510_gpr[data];
			} else es_tmp=1;
			break;

		case 0xa0: /* Write select - GPR */
	//		logerror("ES5510:  Write GPR %06x %06x (0x%04x:=0x%06x\n",data,es5510_gpr_latch,data,snd_mem[es5510_gpr_latch>>8]);
			if (data<0xc0)
				es5510_gpr[data]=snd_mem[es5510_gpr_latch>>8];
			break;

		case 0xc0: /* Write select - INSTR */
	//		logerror("ES5510:  Write INSTR %06x %06x\n",data,es5510_gpr_latch);
			break;

		case 0xe0: /* Write select - GPR + INSTR */
	//		logerror("ES5510:  Write GPR/INSTR %06x %06x\n",data,es5510_gpr_latch);
			break;
	}
}

static unsigned int f3_read_memory_8(unsigned int address)
{
	if ((address < 0x40000))
	{
		return f3work[address&0xffff];
	}

	if ((address >= 0xff0000))
	{
		return f3work[(address&0xffff)];
	}

	if (address >= 0x140000 && address <= 0x140fff)
	{
/*		if (commram[(address&0xfff)/2] != 0)
		{
			if (((address & 0xfff)/2) < 0x7f0)
				printf("read %x from commram at %x\n", commram[(address&0xfff)/2], (address&0xfff)/2);
		} */
		return commram[(address&0xfff)/2];
	}

	if (address == 0x200001)
	{
		return 0;
	}

	if (address >= 0x260000 && address < 0x26ffff)
	{
		if (address & 1)
		{
			return es5510_dsp_r((address&0xffff)/2, 0xff00);
		}
		else
		{
			return es5510_dsp_r((address&0xffff)/2, 0x00ff)>>8;
		}
	}

	if (address >= 0x280000 && address <= 0x28ffff)
	{
		if (address & 1)
		{
			return (f3_68681_r((address&0xffff)/2, 0xff00));
		}
		else
		{
			return (f3_68681_r((address&0xffff)/2, 0x00ff))>>8;
		}
	}

	if (address >= 0xc00000 && address <= 0xc7ffff)
	{
		return prgrom[address&0x7ffff];
	}

	printf("Unmapped read 8 at %x\n", address);
	return 0;
}

static unsigned int f3_read_memory_16(unsigned int address)
{
	if ((address < 0x40000))
	{
		address &= 0xffff;
		return mem_readword_swap((unsigned short *)(f3work+address));
	}

	if ((address >= 0xff0000))
	{
		address &= 0xffff;
		return mem_readword_swap((unsigned short *)(f3work+address));
	}

	if (address >= 0x200000 && address <= 0x200460)
	{
		return ES5505_data_0_r((address&0xfff)/2, 0);
	}

	if (address >= 0x280000 && address <= 0x28ffff)
	{
		return f3_68681_r((address&0xffff)/2, 0);
	}

	if (address >= 0xc00000 && address <= 0xc7ffff)
	{
		address &= 0x7ffff;
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	printf("Unmapped read 16 at %x (PC=%x)\n", address, m68k_get_reg(NULL, M68K_REG_PC));
//		exit(-1);
	return 0;
}

static unsigned int f3_read_memory_32(unsigned int address)
{
	if ((address < 0x40000))
	{
		address &= 0xffff;
		return mem_readlong_swap((unsigned int *)(f3work+address));
	}

	if ((address >= 0xff0000))
	{
		address &= 0xffff;
		return mem_readlong_swap((unsigned int *)(f3work+address));
	}

	return (f3_read_memory_16(address)<<16) | (f3_read_memory_16(address+2));
}

static void f3_write_memory_8(unsigned int address, unsigned int data)
{
	if ((address < 0x40000))
	{
		f3work[address&0xffff] = data;
		return;
	}

	if ((address >= 0xff0000))
	{
		f3work[(address&0xffff)] = data;
		return;
	}

	if (address >= 0x140000 && address <= 0x140fff)
	{
//		printf("write %x to commram at %x\n", data, (address&0xfff)/2);
//		if (((address&0xfff)/2)&0xff0 == 0x480)
//			printf("write %x to commram at %x\n", data, (address&0xfff)/2);

		commram[(address&0xfff)/2] = data;
		return;
	}

	if (address >= 0x260000 && address <= 0x26ffff)
	{
		if (address & 1)
		{
			es5510_dsp_w((address&0xffff)/2, data&0xff, 0xff00);		
		}
		else
		{
			es5510_dsp_w((address&0xffff)/2, data<<8, 0x00ff);		
		}
		return;
	}

	if (address >= 0x280000 && address <= 0x28ffff)
	{
		if (address & 1)
		{
			f3_68681_w((address&0xffff)/2, data, 0xff00);
		}
		else
		{
			f3_68681_w((address&0xffff)/2, data<<8, 0x00ff);
		}
		return;
	}

	if (address >= 0x300000 && address <= 0x300080)
	{
		unsigned int max_banks_this_game=(rom_getregionsize(RGN_SAMP1)/0x200000)-1;
		int offset = (address&0xff)/2;

//		printf("ES550x bankswitch: voice %d to %x\n", (address&0xff)/2, data);
		if ((data&0x7)>max_banks_this_game)
			ES5506_voice_bank_0_w(offset, max_banks_this_game<<20);
		else
			ES5506_voice_bank_0_w(offset, (data&0x7)<<20);
		return;
	}

	if (address >= 0x340000 && address <= 0x340003)
	{
		return;	// write to volume control
	}

	printf("Unmapped write 8 %x to %x\n", data, address);
}

static void f3_write_memory_16(unsigned int address, unsigned int data)
{
	if ((address < 0x40000))
	{
		address &= 0xffff;
		mem_writeword_swap((unsigned short *)(f3work+address), data);
		return;
	}

	if ((address >= 0xff0000))
	{
		address &= 0xffff;
		mem_writeword_swap((unsigned short *)(f3work+address), data);
		return;
	}

	if (address >= 0x200000 && address <= 0x20ffff)
	{
		ES5505_data_0_w((address&0xfff)/2, data, 0);
		return;
	}

	if (address >= 0x280000 && address <= 0x28ffff)
	{
		f3_68681_w((address&0xffff)/2, data, 0);
		return;
	}

	printf("Unmapped write 16 %x to %x\n", data, address);
}

static void f3_write_memory_32(unsigned int address, unsigned int data)
{
	if ((address < 0x40000))
	{
		address &= 0xffff;
		mem_writelong_swap((unsigned int *)(f3work+address), data);
		return;
	}

	if (address >= 0xff0000)
	{
		address &= 0xffff;
		mem_writelong_swap((unsigned int *)(f3work+address), data);
		return;
	}

	if (address >= 0x200000 && address <= 0x20ffff)
	{
		f3_write_memory_16(address, data>>16);
		f3_write_memory_16(address+2, data&0xffff);
		return;
	}

	printf("Unk W32 @ %x\n", address);
}

static int f3_irq_callback(int int_level)
{
	return vector_reg;
}

static int addr;

static void F3_Init(long srate)
{
	memset(f3work, 0, 64*1024);

	// copy down the 68k vector table
	memcpy(f3work, prgrom, 256);

	memset(bExtCodeBuf, 0, sizeof(bExtCodeBuf));
	lastcode = 0;

	memset(commram, 0, 0x1000);
	commram[0x7f8] = 0x30;
	commram[0x7f9] = 0x30;
	commram[0x7fa] = 0x30;
	commram[0x7fb] = 0x30;
							  
	addr = 0;
}

static void f3_write_byte(int byte)
{
	commram[addr] = byte;
	addr++;

//	printf("%x\n", byte);

	// the 68020 and 68000 agree to wrap at this point
	// and this point only!
	if (addr >= 0x400)
	{
		addr = 0;
	}
}

void ExtCodeCase0(UINT8 *s)
{
	UINT8 ch = *s;

	if (ch < 0x80) {
		// $00 - $7F
		bExtCodeBuf[ch] = ch;

		f3_write_byte(0x03);
		f3_write_byte(0x81);
		f3_write_byte(ch);
		f3_write_byte(0x04);
		f3_write_byte(0x86);
		f3_write_byte(*s++);
		f3_write_byte(*s);
	}
	else {
		// $80 - $FF
		UINT8 ch2 = ch & 0x7f;

		bExtCodeBuf[ch2] = ch;

		f3_write_byte(0x03);
		f3_write_byte(0x81);
		f3_write_byte(*s);
		f3_write_byte(0x04);
		f3_write_byte(0x86);
		f3_write_byte(*s);
		f3_write_byte(*(s + 1));
		f3_write_byte(0x04);
		f3_write_byte(0x86);
		f3_write_byte(ch2);
		f3_write_byte(*(s + 1));
	}
}

void ExtCodeCase1(UINT8 *s)
{
	UINT8 ch = *s & 0x7f;

	bExtCodeBuf[ch] = *s;

	f3_write_byte(0x03);
	f3_write_byte(0x81);
	f3_write_byte(*s);
	f3_write_byte(0x04);
	f3_write_byte(0x86);
	f3_write_byte(*s);
	f3_write_byte(*(s + 1));
	f3_write_byte(0x04);
	f3_write_byte(0x88);
	f3_write_byte(*(s + 2));
	f3_write_byte(*s);
}

void ExtCodeCase2(UINT8 *s)
{
	UINT8 ch = *s;
	INT16 offs = ((ch - 1) << 3) + *(s + 2);

	if (bExtCodeBuf[offs]) {
		f3_write_byte(0x05);
		f3_write_byte(0x8f);
		f3_write_byte(ch);
		f3_write_byte(*(s + 2));
		f3_write_byte(bExtCodeBuf[offs]);
	}

	f3_write_byte(0x06);
	f3_write_byte(0x8d);
	f3_write_byte(*s);
	f3_write_byte(*(s + 2));
	f3_write_byte(*(s + 3));
	f3_write_byte(*(s + 4));
	f3_write_byte(0x06);
	f3_write_byte(0x8e);
	f3_write_byte(*s);
	f3_write_byte(*(s + 2));
	f3_write_byte(*(s + 5));
	f3_write_byte(*(s + 1));

	bExtCodeBuf[offs] = *(s + 5);
}

void ExtCodeCase3(UINT8 *s)
{
	int size = *s++;

	while (size > 0) {
		f3_write_byte(*s++);
		size--;
	}
}

void ExtCodeCase4(UINT8 *s)
{
	UINT8 ch = *s & 0x7f, ch2;

	ch2 = bExtCodeBuf[ch];
	if (ch2) {
		bExtCodeBuf[ch] = 0;
		f3_write_byte(0x04);
		f3_write_byte(0x86);
		f3_write_byte(ch2);
		f3_write_byte(0x00);
		f3_write_byte(0x03);
		f3_write_byte(0x82);
		f3_write_byte(ch2);
	}
}

void ExtCodeCase5(UINT8 *s)
{
	UINT8 ch = *s, ch2;
	INT16 offs = ((ch - 1) << 3) + *(s + 2);

	ch2 = bExtCodeBuf[offs];
	if (ch2) {
		bExtCodeBuf[offs] = 0;

		f3_write_byte(0x05);
		f3_write_byte(0x8f);
		f3_write_byte(*s);
		f3_write_byte(*(s + 2));
		f3_write_byte(ch2);
	}
}

void FetchCode(int type, int _code)
{
	UINT8 *tbl, *src, job;
	UINT16 code;
	int tbladr, adrofs;

	code = _code & 0xffff;
	tbl = &prgrom2[Machine->refcon];
	adrofs = Machine->refcon + (code << 2);
	tbladr = prgrom2[adrofs]<<24 | prgrom2[adrofs+1]<<16 | prgrom2[adrofs+2]<<8 | prgrom2[adrofs+3];

	if (tbladr < 0) return;
	if (tbladr > 2*1024*1024) return;

//	printf("refcon %x, tbladr %x\n", Machine->refcon, tbladr);
	src = &prgrom2[tbladr];
	job = *src++;

//	printf("job %x\n", job);

	if (type == 0)	// stop
	{
		switch (job)
		{
			case 0:
			case 1:
				ExtCodeCase4(src);
				break;
			case 2:
				ExtCodeCase5(src);
				break;
			default:
				break;
		}
	}
	else
	{
		switch (job)
		{
			case 0:
				ExtCodeCase0(src);
				break;
			case 1:
				ExtCodeCase1(src);
				break;
			case 2:
				ExtCodeCase2(src);
				break;
			case 3:
				ExtCodeCase3(src);
				break;
			case 4:
			default:
				break;
		}
	}
}

void Play(int code)
{
	FetchCode(0, lastcode);
	FetchCode(1, code);

	lastcode = code;
}

static void F3_SendCmd(int cmda, int cmdb)
{
	addr = commram[0x480]<<8 | commram[0x481];
//	printf("addr in = %x, 68k last=%x\n", addr, commram[0x0482]<<8|commram[0x0483]);
	addr /= 2;

//	printf("cmda %x\n", cmda);

	if (Machine->refcon != 0)
	{
		if (cmda != 0)
		{
			Play(cmda);
		}
	}
	else
	{
		if (cmda)
		{
			if (cmda < 0x100)
			{
				// play music command
				f3_write_byte(0x03);
				f3_write_byte(0x81);     
				f3_write_byte(cmda);

				// set volume (to 7f)
				f3_write_byte(0x04);     
				f3_write_byte(0x86);     
				f3_write_byte(cmda);
				f3_write_byte(0x7f);
			}
			else	// effect play
			{
	#if 0
				f3_write_byte(0x06);
				f3_write_byte(0x8e);
				f3_write_byte(cmda>>8);
				f3_write_byte(0x07);
				f3_write_byte(cmda&0xff);
				f3_write_byte(0x7f);	// volume
	#endif
			}

			lastcmd = cmda;
		}
		else
		{
			if (lastcmd < 0x100)
			{
				// set volume to 0
				f3_write_byte(0x04);
				f3_write_byte(0x86);
				f3_write_byte(lastcmd);
				f3_write_byte(0x00);

				// stop music
				f3_write_byte(0x03);
				f3_write_byte(0x82);
				f3_write_byte(lastcmd);
			}
			else
			{	// effect stop
	#if 0
				f3_write_byte(0x06);
				f3_write_byte(0x8d);
				f3_write_byte(0x01);
				f3_write_byte(0x07);
				f3_write_byte(0x40);
				f3_write_byte(0x00);
	#endif
			}
		}
	}

	addr *= 2;
//	printf("addr out = %x\n", addr);

	commram[0x480] = addr>>8;
	commram[0x481] = addr&0xff;
}

M1_BOARD_START( f3 )
	MDRV_NAME( "F3/Super Z/JC Systems" )
	MDRV_HWDESC( "68000, ES5505, ES5510 ESP" )
	MDRV_DELAYS( 2000, 200 )
	MDRV_INIT( F3_Init )
	MDRV_SEND( F3_SendCmd )

	MDRV_CPU_ADD(MC68000, M68K_CLOCK)
	MDRV_CPUMEMHAND(&f3_readwritemem)

	MDRV_SOUND_ADD(ES5505, &es5505_interface)
M1_BOARD_END
