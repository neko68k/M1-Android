// Sega System C2 hardware
// Driver based on AMUSE by CAB.

#include "m1snd.h"

static void C2_Init(long srate);
static void C2_SendCmd(int cmda, int cmdb);

static unsigned int c2_read_memory_8(unsigned int address);
static unsigned int c2_read_memory_16(unsigned int address);
static unsigned int c2_read_memory_32(unsigned int address);
static void c2_write_memory_8(unsigned int address, unsigned int data);
static void c2_write_memory_16(unsigned int address, unsigned int data);
static void c2_write_memory_32(unsigned int address, unsigned int data);

static int vdp_status = 0;
static int ym_int = 0, vbl_int = 0, enable_vbl = 0;
static int trace = 0;
static int control;

#define MASTER_CLOCK		(53693100)

static M168KT c2_readwritemem =
{
	c2_read_memory_8,
	c2_read_memory_16,
	c2_read_memory_32,
	c2_write_memory_8,
	c2_write_memory_16,
	c2_write_memory_32,
};

static struct UPD7759_interface upd7759_intf =
{
	1,			    		/* One chip */
	{ 50 },			    		/* Volume */
	{ RGN_SAMP1 },				/* Memory pointer (gen.h) */
	UPD7759_STANDALONE_MODE			/* Chip mode */
};

static void ym3438_interrupt(int state)
{
	// irq 2
	printf("YMIRQ: %d\n", state);
	ym_int = state;
	if (state)
	{
		m68k_set_irq(M68K_IRQ_2);
	}
	else
	{
		if (vbl_int)
		{
			printf("queued vbl int\n");
			vbl_int = 0;
			m68k_set_irq(M68K_IRQ_6);
		}
	}
}

static struct YM2612interface ym3438_intf =
{
	1,								/* One chip */
	MASTER_CLOCK/7,					/* Clock: 7.67 MHz */
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) },	/* Volume */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ ym3438_interrupt }			/* IRQ handler */
};

static struct SN76496interface sn76489_intf =
{
	1,								/* One chip */
	{ MASTER_CLOCK/15 },			/* Clock: 3.58 MHz */
	{ 50 }							/* Volume */
};

M1_BOARD_START( segac2 )
	MDRV_NAME("Sega C2 System")
	MDRV_HWDESC("68000, YM3834, uDP7759, SN76496")
	MDRV_INIT( C2_Init )
	MDRV_SEND( C2_SendCmd )
	MDRV_DELAYS(2200, 400)

	MDRV_CPU_ADD(MC68000, MASTER_CLOCK/8)
	MDRV_CPUMEMHAND(&c2_readwritemem)

	MDRV_SOUND_ADD(YM2612, &ym3438_intf)
	MDRV_SOUND_ADD(SN76496, &sn76489_intf)
	MDRV_SOUND_ADD(UPD7759, &upd7759_intf)
M1_BOARD_END

static unsigned int c2_read_memory_8(unsigned int address)
{
	if (address < 0x200000)
	{
		return prgrom[address];
	}

	if (address >= 0x7f0000 && address <= 0x7fffff)
	{
		return workram[(address&0xffff)+0x10000];
	}

	if (address == 0x800201)
	{
		return control;
	}

	if (address == 0x840009)
	{
		return (UPD7759_0_busy_r(0)<<6) | 0xbf;
	}

	if (address == 0x84000b) return 0xff;
	if (address == 0x84000d) return 0xff;

	if (address == 0x840100 || address == 0x840101) 
	{
	//	printf("Rd 2612 st 0A\n");
		return YM2612_status_port_0_A_r(0);
	}

	if (address == 0x840102 || address == 0x840103) 
	{
	//	printf("Rd 2612 rp 0\n");
		return YM2612_read_port_0_r(0);
	}

	if (address == 0x840104 || address == 0x840105) 
	{
	//	printf("Rd 2612 st 0B\n");
		return YM2612_status_port_0_B_r(0);
	}

	if (address == 0xc00004) return 0;
	if (address == 0xc00005) 
	{
		printf("rd vdp stat\n");
		vdp_status ^= 8;
		return vdp_status;
	}

	if (address >= 0xff0000 && address <= 0xffffff)
	{
		return workram[address-0xff0000];
	}

	printf("Unk read 8 at %x\n", address);
	return 0;
}

static unsigned int c2_read_memory_16(unsigned int address)
{
	if (trace) printf("RD16 @ %x\n", address);
	if (address < 0x200000)
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if ((address >= 0x7f0000) && (address <= 0x7fffff))
	{
		address &= 0xffff;
		address += 0x10000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	if (address == 0xc00004)
	{
		printf("rd vdp stat W\n");
		vdp_status ^= 8;

		return vdp_status;
	}

	if ((address >= 0xff0000) && (address <= 0xffffff))
	{
		address -= 0xff0000;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	printf("Unk read 16 at %x\n", address);
	return 0;
}

static unsigned int c2_read_memory_32(unsigned int address)
{
	if (trace) printf("RD32 @ %x\n", address);
	if (address < 0x200000)
	{
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}

	if ((address >= 0x7f0000) && (address <= 0x7fffff))
	{
//		printf("Read32 at %x\n", address);
		address &= 0xffff;
		address += 0x10000;
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

	if ((address >= 0xff0000) && (address <= 0xffffff))
	{
		address -= 0xff0000;
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

	printf("Unk read 32 at %x\n", address);
	return 0;
}

static void c2_write_memory_8(unsigned int address, unsigned int data)
{
	if (address == 0x800201)
	{
		control = data;
		return;
	}
	if (address == 0x840100 || address == 0x840101)
	{
	       //	printf("2612 ctl 0A %x\n", data);
		YM2612_control_port_0_A_w(0, data&0xff);
		return;
	}
	if (address == 0x840102 || address == 0x840103)
	{
	       //	printf("2612 data 0A %x\n", data);
		YM2612_data_port_0_A_w(0, data&0xff);
		return;
	}
	if (address == 0x840104 || address == 0x840105)
	{
	       //	printf("2612 ctl 0B %x\n", data);
		YM2612_control_port_0_B_w(0, data&0xff);
		return;
	}
	if (address == 0x840106 || address == 0x840107)
	{
	      //	printf("2612 data 0B %x\n", data);
		YM2612_data_port_0_B_w(0, data&0xff);
		return;
	}

	if (address >= 0xc00010 && address <= 0xc00017)
	{
		SN76496_0_w(0, data&0xff);
		return;
	}

	if (address == 0xf00000)
	{
		timer_yield();
		return;
	}

	if (address >= 0xff0000 && address < 0xffffff)
	{
		address -= 0xff0000;
		workram[address] = data;
		return;
	}
	printf("Unk write 8 %x to %x\n", data, address);
}

static void c2_write_memory_16(unsigned int address, unsigned int data)
{
	if (address >= 0xc00000 && address <= 0xc0000f)
	{
		return;	// trap vdp writes
	}

	if (address >= 0xff0000 && address <= 0xffffff)
	{
		address -= 0xff0000;
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}
	printf("Unk write 16 %x to %x\n", data, address);
}

static void c2_write_memory_32(unsigned int address, unsigned int data)
{
	if (address >= 0x8c0000 && address <= 0x8c0fff) return;
	if (address >= 0xc00000 && address <= 0xc0000f)
	{
		return;	// trap vdp writes
	}

	if (address >= 0xff0000 && address <= 0xffffff)
	{
		address -= 0xff0000;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}
	printf("Unk write 32 %x to %x\n", data, address);
}

static void WriteWord(void* mem,short dt)
{
	mem_writeword_swap((unsigned short *)mem, dt);
}

static void MakeCodes(void* mem,long *p)
{
	short *pOut=(short*)mem;
	while(1)
	{
		if(*p==-1)	break;
		WriteWord(pOut,(*p)&0xffff);
		pOut++;
		p++;
	}
}

static void MakeSpinoutLoop(void *mem)
{
	static long p[]={
			0x13fc,0x0000,0x00f0,0x0000,	//move.b	#$00,$f00000
			0x60f6,				//bra.s *
			-1};
	MakeCodes(mem,p);
}

// vblank
static void gx_timer(int refcon)
{
	if (enable_vbl)
	{
		if (ym_int)
		{
			vbl_int = 1;
		}
		else
		{
			m68k_set_irq(M68K_IRQ_6);
		}
	}

	timer_set(1.0/60.0, 0, gx_timer);
}

static void C2_Init(long srate)
{
//	FILE *f;

	vdp_status = 0;

	switch (Machine->refcon)
	{
		case 0:
		// thunder force AC
		MakeSpinoutLoop(&prgrom[0x05c0]);
		break;

		case 1:
		// puyo puyo 2
		MakeSpinoutLoop(&prgrom[0x020c]);
		break;
	}

	timer_set(1.0/60.0, 0, gx_timer);

	enable_vbl = 0;

	m1snd_addToCmdQueue(0xffff);
}

static void C2_SendCmd(int cmda, int cmdb)
{
	switch (Machine->refcon)
	{

	case 0:	// thunderforce ac
	if (cmda == 0xffff)	// init finished
	{
		printf("init finished, patching at 7f0000\n");
//		trace = 1;
		// redirect level 1 IRQ to 7f0000
		printf("old IRQV = %x %x %x %x\n", prgrom[0x64], prgrom[0x65], prgrom[0x66], prgrom[0x67]);
		prgrom[0x64] = 0x00;
		prgrom[0x65] = 0x7f;
		prgrom[0x66] = 0x00;
		prgrom[0x67] = 0x00;

		// install our handler routine at 7f0000
		#if 0
		{  	static long p[]={
					0x48E7,0xfffe,		//		movem.l		d0-d7/a0-a6,-(a7)
					0x7000,						//		moveq		#$xx,d0		//index
					0x7200,						//		moveq		#$xx,d1		//command
					0x74ff,						//		moveq 		#$ff,d2		//loop count?
					0x0c41,0x0000,				//		cmpi		#$0000,d1
					0x671e,						//		beq.s		aa
					0x0c41,0x0001,				//		cmpi		#$0001,d1
					0x672c,						//		beq.s		bb
					0x0c41,0x0002,				//		cmpi		#$0002,d1
					0x6732,						//		beq.s		cc
					0x0c41,0x0003,				//		cmpi		#$0003,d1
					0x673e,						//		beq.s		dd
					0x0c41,0x0004,				//		cmpi		#$0004,d1
					0x674a,						//		beq.s		ee
					0x4cdf,0x7fff,				//		movem.l		(a7)+,d0-d7/a0-a6
					0x4e73,						//		rte
					//
					0x4eb9,0x0000,0x5f24,		//aa:	jsr			$5f24		#0
					0x3002,						//		move		d2,d0
					0x4eb9,0x0000,0x5f10,		//		jsr			$5f10		#6
					0x4cdf,0x7fff,				//		movem.l		(a7)+,d0-d7/a0-a6
					0x4e73,						//		rte
					//
					0x4eb9,0x0000,0x5f6c,		//bb:	jsr			$5f6c		#2
					0x4cdf,0x7fff,				//		movem.l		(a7)+,d0-d7/a0-a6
					0x4e73,						//		rte
					//
					0x4eb9,0x0000,0x5fa4,		//cc:	jsr			$5fa4
					0x4eb9,0x0000,0x5fbe,		//		jsr			$5fbe		#1
					0x4cdf,0x7fff,				//		movem.l		(a7)+,d0-d7/a0-a6
					0x4e73,						//		rte
					//
					0x4eb9,0x0000,0x5f82,		//dd:	jsr			$5f82
					0x4eb9,0x0000,0x5f6c,		//		jsr			$5f6c		#1
					0x4cdf,0x7fff,				//		movem.l		(a7)+,d0-d7/a0-a6
					0x4e73,						//		rte
					//
					0x4eb9,0x0000,0x5f6c,		//ee:	jsr			$6000		#3 (noparam)
					0x4cdf,0x7fff,				//		movem.l		(a7)+,d0-d7/a0-a6
					0x4e73,						//		rte
					-1};
  		  	MakeCodes(&workram[0x10000],p);
		}
		#endif
		// nop something out in the rom
		WriteWord(&prgrom[0x5f94],0x4e71);
//		enable_vbl = 1;
	}
	else
	{
		if (cmda == 0xfffe)
		{	// stop
		 	workram[0x10007] = 4;	
		 	workram[0x10005] = 0;	
		 	workram[0x10009] = 0;	
		}
		else
		{
			return;	     
			if(cmda<0x100)
			{
				static long lp[]={0x0f,0x11,0x12,0x13,0x14,-1};
				int ldcmda=0;
				int bnLoop=0;
				//
				workram[0x10007]=0;
				workram[0x10005]=cmda&255;
				while(lp[ldcmda]!=-1)	{if(lp[ldcmda++]==(cmda&255))	{bnLoop=1; break;}}
				if(bnLoop)	workram[0x10009]=0;
				else		workram[0x10009]=0xff;
			}
			else if(cmda<0x200)
			{
				workram[0x10007]=2;
				workram[0x10005]=cmda&255;
				workram[0x10009]=0;
			}
			else if(cmda<0x300)
			{
				workram[0x10007]=3;
				workram[0x10005]=cmda&255;
				workram[0x10009]=0;
			}
			else if(cmda<0x400)
			{
				workram[0x10007]=1;
				workram[0x10005]=cmda&255;
				workram[0x10009]=0;
			}

			m68k_set_irq(M68K_IRQ_1);
			m68k_execute(5000);
		}
	}
	break;

	case 1:	// puyo puyo 2 
	if (cmda == 0xffff)
	{
	 	static long p[]={	0x4ef9,0x0000,0x05da,			//jmp.l		$05da
									-1};

		printf("puyo patch\n");
	 	MakeCodes(&prgrom[0x554],p);
		enable_vbl = 1;
	}
	else
	{
		workram[0x98c4] = cmda & 0xff;
		workram[0x98cb] = 1;
		workram[0x9820] = 1;
	}
	break;
	}
}
