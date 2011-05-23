// Sega Genesis

// sonic 1 : music runs in subroutine @ 71b4c, commands at ffff0a/b/c, z80 program decompress/download @ 134a

#include "m1snd.h"

static unsigned char z80_ram[0x2000];

// sound driver include handlers
#include "gen_inc_gems.cpp"
#include "gen_inc_soundimages.cpp"
#include "gen_inc_smpsz80.cpp"

static int busy_68k = 0;

static void Gen_Init(long srate);
static void Gen_SendCmd(int cmda, int cmdb);

static unsigned int gen_read_memory_8(unsigned int address);
static unsigned int gen_read_memory_16(unsigned int address);
static unsigned int gen_read_memory_32(unsigned int address);
static void gen_write_memory_8(unsigned int address, unsigned int data);
static void gen_write_memory_16(unsigned int address, unsigned int data);
static void gen_write_memory_32(unsigned int address, unsigned int data);

static int vdp_status = 0, busreq = 0;

#define MASTER_CLOCK		(53693100)

static M168KT gen_readwritemem =
{
	gen_read_memory_8,
	gen_read_memory_16,
	gen_read_memory_32,
	gen_write_memory_8,
	gen_write_memory_16,
	gen_write_memory_32,
};

static void ym2612_interrupt(int state)
{
}

static struct YM2612interface ym3438_intf =
{
	1,								/* One chip */
	MASTER_CLOCK/7,					/* Clock: 7.67 MHz */
	{ MIXER(40,MIXER_PAN_LEFT), MIXER(40,MIXER_PAN_RIGHT) },	/* Volume */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ ym2612_interrupt }			/* IRQ handler */
};				 

static struct SN76496interface sn76489_intf =
{
	1,								/* One chip */
	{ MASTER_CLOCK/15 },			/* Clock: 3.58 MHz */
	{ 10 }							/* Volume */
};

static unsigned int gen_read_memory_8(unsigned int address)
{
	if (address < 0x400000)
	{
		return prgrom[address];
	}

	if (address >= 0x880000 && address <= 0x9FFFFF) 
	{
		address -= 0x880000;
		return prgrom[address];
	}

	if ((address >= 0xa00000) && (address <= 0xa03fff))
	{
		return z80_ram[address&0x1fff];
	}

	if (address >= 0xa04000 && address <= 0xa04003)
	{
		return YM2612_status_port_0_A_r(0);
	}

	if (address >= 0xa10000 && address <= 0xa1000f) 
	{
		return 0;
	}

	if (address == 0xa11100)
	{
//		printf("read busreq = %x (PC=%x)\n", busreq, m68k_get_reg(NULL, M68K_REG_PC));
		return busreq ^ 1;
	}

	if (address == 0xc00004) return 0;
	if (address == 0xc00005) 
	{
//		printf("rd vdp stat\n");
		return vdp_status;
	}

	if (address >= 0xe00000)
	{
		address &= 0xffff;
		return workram[(address&0xffff)];
	}

	printf("Unk read 8 at %x\n", address);
	return 0;
}

static unsigned int gen_read_memory_16(unsigned int address)
{
	if (address < 0x400000)
	{
		return mem_readword_swap((unsigned short *)(prgrom+address));
	}

	if (address >= 0x880000 && address <= 0x9FFFFF) 
	{
		return mem_readword_swap((unsigned short *)(prgrom+(address - 0x880000)));
	}

	if ((address >= 0xa00000) && (address <= 0xa03fff))
	{
		address &= 0x1fff;
		return mem_readword_swap((unsigned short *)(z80_ram+address));
	}

	if (address == 0xc00004)
	{
//		printf("rd vdp stat W\n");

		return vdp_status;
	}

	if (address >= 0xe00000)
	{
		address &= 0xffff;
		return mem_readword_swap((unsigned short *)(workram+address));
	}

	printf("Unk read 16 at %x\n", address);
	return 0;
}

static unsigned int gen_read_memory_32(unsigned int address)
{
	if (address < 0x400000)
	{
		return mem_readlong_swap((unsigned int *)(prgrom+address));
	}

	if (address >= 0x880000 && address <= 0x9FFFFF) 
	{
		return mem_readlong_swap((unsigned int *)(prgrom+(address - 0x880000)));
	}

	if ((address >= 0xa00000) && (address <= 0xa03fff))
	{
		address &= 0x1fff;
		return mem_readlong_swap((unsigned int *)(z80_ram+address));
	}

	if (address >= 0xe00000)
	{
		address &= 0xffff;
		return mem_readlong_swap((unsigned int *)(workram+address));
	}

	printf("Unk read 32 at %x\n", address);
	return 0;
}

static void gen_write_memory_8(unsigned int address, unsigned int data)
{
	if ((address >= 0xa00000) && (address <= 0xa03fff))
	{
//		printf("W8 %02x to Z80 @ %x\n", data, address&0x1fff);
		z80_ram[address&0x1fff] = data;
		return;
	}

	if (address >= 0xa04000 && address <= 0xa04003)
	{
		switch (address & 7)
		{
			case 0:
				YM2612_control_port_0_A_w(0, data);
				break;
			case 1:
			  	YM2612_data_port_0_A_w(0, data);
				break;
			case 2:
				YM2612_control_port_0_B_w(0, data);
				break;
			case 3:
			  	YM2612_data_port_0_B_w(0, data);
				break;
		}
		return;
	}

	// I/O
	if (address >= 0xa10000 && address <= 0xa1000f) 
	{
		return;
	}

	if (address >= 0xa11200 && address <= 0xa11201)
	{
//		printf("Z80 reset W8: %x\n", data);
		// z80 reset

		if (data)
		{
//			printf("booting Z80\n");
			timer_unspin_cpu(1);
			cpu_set_reset_line(1, ASSERT_LINE);
			cpu_set_reset_line(1, CLEAR_LINE);
		}
		else
		{
//			printf("Stopping Z80\n");
			timer_spin_cpu(1);
		}
		return;
	}

	if (address >= 0xA15100 && address <= 0xA1513F) 
	{
		printf("Program wrote B in 32X space: %08X\n", address);
		return;
	}

	if (address >= 0xc00011 && address <= 0xc00017)
	{
		SN76496_0_w(0, data&0xff);
		return;
	}

	if (address >= 0xe00000)
	{
		address &= 0xffff;
		workram[address] = data;
		return;
	}
	printf("Unk write 8 %x to %x\n", data, address);
}

static void gen_write_memory_16(unsigned int address, unsigned int data)
{
	if (address >= 0xc00000 && address <= 0xc0000f)
	{
		return;	// trap vdp writes
	}
							     
	if (address == 0xc10000)
	{
		printf("68k trap, disabling 68k @ %x\n", m68k_get_reg(NULL, M68K_REG_PC));
		busy_68k = 0;
		timer_spin_cpu(0);
		timer_yield();
		return;
	}

	if ((address >= 0xa00000) && (address <= 0xa03fff))
	{
		address &= 0x1fff;
		mem_writeword_swap((unsigned short *)(z80_ram+address), data);
		return;
	}

	if (address == 0xa11100)
	{
//		printf("68k: busreq %x (PC=%x)\n", data, m68k_get_reg(NULL, M68K_REG_PC));
		if (data)
		{
			busreq = 1;
		}
		else
		{
			busreq = 0;
		}
		return;
	}

	if (address == 0xa11200)
	{
//		printf("Z80 reset W16: %x\n", data);

		if (data)
		{
//			printf("booting Z80\n");
			timer_unspin_cpu(1);
			cpu_set_reset_line(1, ASSERT_LINE);
			cpu_set_reset_line(1, CLEAR_LINE);
		}
		else
		{
//			printf("Stopping Z80\n");
			timer_spin_cpu(1);
		}
		return;
	}

	if (address >= 0xA15100 && address <= 0xA1513F) 
	{
		printf("Program wrote W in 32X space: %08X\n", address);
		return;
	}

	if (address >= 0xe00000)
	{
		address &= 0xffff;
		mem_writeword_swap((unsigned short *)(workram+address), data);
		return;
	}
	printf("Unk write 16 %x to %x\n", data, address);
}

static void gen_write_memory_32(unsigned int address, unsigned int data)
{
	if ((address >= 0xa00000) && (address <= 0xa03fff))
	{
		address &= 0x1fff;
		mem_writelong_swap((unsigned int *)(z80_ram+address), data);
		return;
	}

	if (address >= 0xA15100 && address <= 0xA1513F) 
	{
		printf("Program wrote L in 32X space: %08X\n", address);
		return;
	}

	if (address >= 0xc00000 && address <= 0xc0000f)
	{
		return;	// trap vdp writes
	}

	if (address >= 0xe00000)
	{
		address &= 0xffff;
		mem_writelong_swap((unsigned int *)(workram+address), data);
		return;
	}
	printf("Unk write 32 %x to %x\n", data, address);
}

static int z80_latch = 0;

static READ_HANDLER( z80_read_shared )
{
	return z80_ram[offset];
}

static WRITE_HANDLER( z80_write_shared )
{
	z80_ram[offset] = data;
}

static READ_HANDLER( z80_read_vdp )
{
//	printf("Z80 R VDP: ofs %x\n", offset);
	return 0;
}

static WRITE_HANDLER( z80_write_vdp )
{
	switch (offset)
	{
		case 0x11:
		case 0x13:
		case 0x15:
		case 0x17:
			SN76496_0_w(0, data);
			break;

		default:
			printf("unhandled z80 vdp write %02x %02x\n",offset,data);
			break;
	}
}

static UINT32 z80_bank = 0, z80_bankpartial = 0, z80_bank_pos = 0;

static READ_HANDLER( z80_read_bank )
{
	UINT32 addr = offset + z80_bank;

	// program ROM
	if (addr <= 0x3fffff)
	{
		return prgrom[addr];
	}
	else if (addr <= 0x9FFFFF) 
	{
		printf("Z80 read in 32X space: %08X\n", addr);
		addr -= 0x880000;
		return prgrom[addr];
	}

	else if (addr <= 0xA1513F) 
	{
		printf("Program wrote in 32X space: %08X\n", addr);
		return 0;
	}

	else if (addr >= 0xe00000)	// work RAM
	{
		printf("Z80 reading workram @ %x\n", addr);
		return 0; //workram[addr&0xffff];
	}						     
	else
	{
//		printf("Z80 reading unknown banked data @ %x\n", addr);
	}

	return 0;
}

static WRITE_HANDLER( z80_write_bank )
{
	UINT32 addr = offset + z80_bank;

	if (addr <= 0xA1513F) 
	{
		printf("Program wrote in 32X space: %08X\n", addr);
	}
	// work RAM?
	else if (addr >= 0xe00000)
	{
		workram[addr&0xffff] = data;
	}
	else if (addr == 0xc00011)
	{
		SN76496_0_w(0, data);
	}
	else
	{
//		printf("Z80 writing %x to unknown banked data @ %x\n", data, addr);
	}
}

static WRITE_HANDLER( z80_write_bankswitch )
{
	z80_bankpartial |= (data & 0x01) << 23;
	z80_bank_pos++;

	if (z80_bank_pos < 9)
	{
		z80_bankpartial >>= 1;
	}
	else
	{
		z80_bank_pos = 0;
		z80_bank = z80_bankpartial;
		z80_bankpartial = 0;
	}

//	printf("z80: change bank to %x\n", z80_bank);
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0x1fff, z80_read_shared },
	{ 0x2000, 0x3fff, z80_read_shared },
	{ 0x4000, 0x4000, YM2612_status_port_0_A_r },
	{ 0x4001, 0x4001, YM2612_status_port_0_A_r },
	{ 0x4002, 0x4002, YM2612_status_port_0_A_r },
	{ 0x4003, 0x4003, YM2612_status_port_0_A_r },
	{ 0x7f00, 0x7fff, z80_read_vdp },
	{ 0x8000, 0xffff, z80_read_bank },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0x1fff, z80_write_shared },
	{ 0x2000, 0x3fff, z80_write_shared },
	{ 0x4000, 0x4000, YM2612_control_port_0_A_w },
	{ 0x4001, 0x4001, YM2612_data_port_0_A_w },
	{ 0x4002, 0x4002, YM2612_control_port_0_B_w },
	{ 0x4003, 0x4003, YM2612_data_port_0_B_w },
	{ 0x6000, 0x6001, z80_write_bankswitch },
	{ 0x7f00, 0x7fff, z80_write_vdp },
	{ 0x8000, 0xffff, z80_write_bank },
MEMORY_END

static void write_rom_16(UINT32 address, UINT16 data)
{
	mem_writeword_swap((unsigned short *)(prgrom+address), data);
}

static void write_rom_32(UINT32 address, UINT32 data)
{
	mem_writelong_swap((unsigned int *)(prgrom+address), data);
}

static void setup_gems(UINT32 gemsinit, UINT32 initcall, UINT32 startsong, UINT32 stopsong)
{
	UINT32 start, data0, data1, data2, data3;

	write_rom_32(0, 0xfffff0);	// initial stack
	write_rom_32(4, 0x000200);	// address to run

	start = initcall - 0x16;

	data0 = gen_read_memory_32(start);
	data1 = gen_read_memory_32(start+6);
	data2 = gen_read_memory_32(start+12);
	data3 = gen_read_memory_32(start+18);

//	printf("%x %x %x %x\n", data0, data1, data2, data3);

	// generate addresses

	write_rom_16(0x200, 0x4879);	// PEA
	write_rom_32(0x202, data0);
	write_rom_16(0x206, 0x4879);	// PEA
	write_rom_32(0x208, data1);
	write_rom_16(0x20c, 0x4879);	// PEA
	write_rom_32(0x20e, data2);
	write_rom_16(0x212, 0x4879);	// PEA
	write_rom_32(0x214, data3);

//	printf("GEMS data @ %x %x %x %x\n", data0, data1, data2, data3);

	// startup
	write_rom_16(0x218, 0x4eb9);	// JSR
	write_rom_32(0x21a, gemsinit);	// gemsinit
	write_rom_16(0x21e, 0xdffc);	// adda.l
	write_rom_32(0x220, 0x10);	// #0x10, sp
	write_rom_16(0x224, 0x227c);	// movea.l
	write_rom_32(0x226, 0xc10000);	// #$c10000, a1
	write_rom_16(0x22a, 0x32bc);	// move.w 
	write_rom_16(0x22c, 0x5555);	// #$5555, (a1)	(causes halt of 68k)

	// command handler: startsong
	write_rom_16(0x400, 0x4879);	// PEA
	write_rom_32(0x402, 0);		// command #
	write_rom_16(0x406, 0x4eb9);	// JSR
	write_rom_32(0x408, startsong);	// startsong
	write_rom_16(0x40c, 0xdffc);	// adda.l
	write_rom_32(0x40e, 0x4);	// #0x4, sp
	write_rom_16(0x412, 0x227c);	// movea.l
	write_rom_32(0x414, 0xc10000);	// #$c10000, a1
	write_rom_16(0x418, 0x32bc);	// move.w 
	write_rom_16(0x41a, 0x5555);	// #$5555, (a1)	(causes halt of 68k)
		
	// command handler: stopsong
	write_rom_16(0x600, 0x4879);	// PEA
	write_rom_32(0x602, 0);		// command #
	write_rom_16(0x606, 0x4eb9);	// JSR
	write_rom_32(0x608, stopsong);	// stopsong
	write_rom_16(0x60c, 0xdffc);	// adda.l
	write_rom_32(0x60e, 0x4);	// #0x4, sp
	write_rom_16(0x612, 0x227c);	// movea.l
	write_rom_32(0x614, 0xc10000);	// #$c10000, a1
	write_rom_16(0x618, 0x32bc);	// move.w 
	write_rom_16(0x61a, 0x5555);	// #$5555, (a1)	(causes halt of 68k)
}

static void vbl_timer(int refcon);

static void vbl_out_timer(int refcon)
{
	vdp_status &= ~8;
	cpu_set_irq_line(1, 0, CLEAR_LINE);
	timer_set(1.0/60.0, 0, vbl_timer);
}

static void vbl_timer(int refcon)
{
	vdp_status |= 8;

	if (m1snd_get_custom_tag_value("ftrigadr") != 0xffffffff)
	{
		z80_ram[m1snd_get_custom_tag_value("ftrigadr")] = m1snd_get_custom_tag_value("ftrigval");
	}

	cpu_set_irq_line(1, 0, ASSERT_LINE);
	timer_set((1.0/60.0)/262.0, 0, vbl_out_timer);
}

void Gen_Frame(void)
{
	if (Machine->refcon == 6) 
	{
		if (!busy_68k) 
		{
			// run the VBL service routine
			printf("start 68k VBL\n");
			m68k_set_reg(M68K_REG_PC, 0x300);
			timer_unspin_cpu(0);	// restart the 68000
			busy_68k = 1;
		}
		else
		{
			printf("Refusing rekick of 68k - busy\n");
		}
	}
}


// makes the 68k die
static void install_68k_killer(void)
{
	write_rom_32(0, 0xfffff0);	// initial stack
	write_rom_32(4, 0x000200);	// address to run
	
	write_rom_16(0x200, 0x227c);	// movea.l
	write_rom_32(0x202, 0xc10000);	// #$c10000, a1
	write_rom_16(0x206, 0x32bc);	// move.w 
	write_rom_16(0x208, 0x5555);	// #$5555, (a1)	(causes halt of 68k)
}

static void Gen_Init(long srate)
{
	INT32 gemsinit, initcall, startsong, stopsong;
	UINT32 haltadr;
		
	vdp_status = 0;

	// stop the Z80 until we need it
	timer_spin_cpu(1);

	switch (Machine->refcon)
	{
		case 0:	// GEMS
			if (gems_init(&gemsinit, &initcall, &startsong, &stopsong) != -1)
			{
//				printf("GEMS AI: init %x initcall %x start %x stop %x\n", gemsinit, initcall, startsong, stopsong);
				setup_gems(gemsinit, initcall, startsong, stopsong);

				timer_set(1.0/60.0, 0, vbl_timer);
			}
			else
			{
				install_68k_killer();
				timer_spin_cpu(0);	// also disable the 68000
			}
			break;

		case 1: // generic driver using init and cmdadr custom tags
			write_rom_32(0, 0xfffff0);	// initial stack
			write_rom_32(4, 0x000200);	// address to run

			write_rom_16(0x200, 0x4eb9);	// JSR
			write_rom_32(0x202, m1snd_get_custom_tag_value("init"));	// Z80 decompress / init
			write_rom_16(0x206, 0x227c);	// movea.l
			write_rom_32(0x208, 0xc10000);	// #$c10000, a1
			write_rom_16(0x20c, 0x32bc);	// move.w 
			write_rom_16(0x20e, 0x5555);	// #$5555, (a1)	(causes halt of 68k)

			haltadr = m1snd_get_custom_tag_value("halt68k");
			if (haltadr != 0xffffffff) 
			{
				write_rom_16(haltadr, 0x227c);	// movea.l
				write_rom_32(haltadr+2, 0xc10000);	// #$c10000, a1
				write_rom_16(haltadr+6, 0x32bc);	// move.w 
				write_rom_16(haltadr+8, 0x5555);	// #$5555, (a1)	(causes halt of 68k)
			}

		      	timer_set(1.0/60.0, 0, vbl_timer);
			break;

		case 2: // Sound Images
			initcall = SI_GetZ80();
			if (!initcall) printf("WARNING: SoundImages Z80 block not found!\n");
			memcpy(z80_ram, prgrom+initcall, 0x1fff);
			break;				  
							  
	  	case 3: // SMPS/Z80
			write_rom_32(0, 0xfffff0);	// initial stack
			write_rom_32(4, 0x000200);	// address to run
			if (smpsz80_init(&initcall) == 0) 
			{
				// Patch the code
				write_rom_16(0x200, 0x4eb9);
				write_rom_32(0x202, initcall);
				write_rom_16(0x206, 0x4e71);
				write_rom_16(0x208, 0x60fc); // cpu halt
				timer_unspin_cpu(0); // let the 68000 run
			      	timer_set(1.0/60.0, 0, vbl_timer);
			} 
			else 
			{
				printf("patch FAILED!\n");
			}
			break;	

	  	case 4:	// generic with Z80 image
	  	case 5:	// Test Drive II (DSI custom)
			install_68k_killer();

			// copy the z80 program to z80 ram			
//			printf("Z80 image @ %x\n", m1snd_get_custom_tag_value("z80prg"));
			memcpy(z80_ram, &prgrom[m1snd_get_custom_tag_value("z80prg")], 0x2000);

			// boot the Z80
			timer_unspin_cpu(1);

		      	timer_set(1.0/60.0, 0, vbl_timer);
			break;	

	  	case 6: // SMPS/68k
			write_rom_32(0, 0xfffff0);	// initial stack
			write_rom_32(4, 0x000200);	// address to run

			write_rom_16(0x200, 0x4eb9);	// JSR
			write_rom_32(0x202, m1snd_get_custom_tag_value("boot"));	// Z80 decompress / init
			write_rom_16(0x206, 0x227c);	// movea.l
			write_rom_32(0x208, 0xc10000);	// #$c10000, a1
			write_rom_16(0x20c, 0x32bc);	// move.w 
			write_rom_16(0x20e, 0x5555);	// #$5555, (a1)	(causes halt of 68k)
			
			haltadr = m1snd_get_custom_tag_value("halt68k");
			if (haltadr != 0xffffffff) 
			{
				write_rom_16(haltadr, 0x227c);	// movea.l
				write_rom_32(haltadr+2, 0xc10000);	// #$c10000, a1
				write_rom_16(haltadr+6, 0x32bc);	// move.w 
				write_rom_16(haltadr+8, 0x5555);	// #$5555, (a1)	(causes halt of 68k)
			}

			write_rom_16(0x300, 0x4eb9);	// JSR
			write_rom_32(0x302, m1snd_get_custom_tag_value("run"));
			write_rom_16(0x306, 0x227c);	// movea.l
			write_rom_32(0x308, 0xc10000);	// #$c10000, a1
			write_rom_16(0x30c, 0x32bc);	// move.w 
			write_rom_16(0x30e, 0x5555);	// #$5555, (a1)	(causes halt of 68k)

			busy_68k = 0;

			memset(workram, 0, 64*1024);

			timer_spin_cpu(1);
		      	timer_set(1.0/60.0, 0, vbl_timer);
			break;	
	}

	if (m1snd_get_custom_tag_value("cmdsubd0") != 0xffffffff)
	{
		write_rom_16(0x400, 0x7000);	// moveq #0, D0
		write_rom_16(0x402, 0x4eb9);	// JSR
		write_rom_32(0x404, m1snd_get_custom_tag_value("cmdsubd0"));	// startsong
		write_rom_16(0x408, 0x227c);	// movea.l
		write_rom_32(0x40a, 0xc10000);	// #$c10000, a1
		write_rom_16(0x40e, 0x32bc);	// move.w 
		write_rom_16(0x410, 0x5555);	// #$5555, (a1)	(causes halt of 68k)
	}
}

static int lastcmd = -1;

static void Gen_SendCmd(int cmda, int cmdb)
{
	if (cmda == games[curgame].stopcmd)
	{
//		printf("Stopping song %d\n", lastcmd);

		if (Machine->refcon == 0)
		{
			if (lastcmd == -1) return;

			// patch the code
			prgrom[0x605] = lastcmd;
			// point the CPU there
			m68k_set_reg(M68K_REG_SP, 0xfffe00);
			m68k_set_reg(M68K_REG_PC, 0x600);

			timer_unspin_cpu(0);	// restart the 68000
			return;
		}
		else if (Machine->refcon == 5)
		{
			z80_ram[0x71] = 0;
			z80_ram[0x70] = 0;
		}
	}

//	printf("Sending %x to Genesis\n", cmda);

	switch (Machine->refcon)
	{
		case 0:	// GEMS
			// patch the code
			prgrom[0x405] = cmda;
			lastcmd = cmda;

			// point the CPU there
			m68k_set_reg(M68K_REG_PC, 0x400);

			timer_unspin_cpu(0);	// restart the 68000
			break;

		case 4:	// generic driver w/Z80 image
		case 1: // generic driver w/JSR setup
//			printf("CMD %x to generic @ %x\n", cmda, m1snd_get_custom_tag_value("cmdadr"));
			timer_spin_cpu(0);

			if (m1snd_get_custom_tag_value("cmdadr") != 0xffffffff)
			{
				z80_ram[m1snd_get_custom_tag_value("cmdadr")] = cmda&0xff;
			}

			if (m1snd_get_custom_tag_value("cmdhiadr") != 0xffffffff)
			{
				z80_ram[m1snd_get_custom_tag_value("cmdhiadr")] = (cmda>>8)&0xff;
			}

			if (m1snd_get_custom_tag_value("cmd2adr") != 0xffffffff)
			{
				int base = m1snd_get_custom_tag_value("cmd2adr");

				z80_ram[base] = (cmda>>8)&0xff;
				z80_ram[base+1] = (cmda&0xff);

				printf("%02x @ %x, %02x @ %x\n", z80_ram[base], base, z80_ram[base+1], base+1);
			}

			if (m1snd_get_custom_tag_value("cmd3adr") != 0xffffffff)
			{
				int base = m1snd_get_custom_tag_value("cmd3adr");

				z80_ram[base] = (cmda>>16)&0xff;
				z80_ram[base+1] = (cmda>>8)&0xff;
				z80_ram[base+2] = (cmda&0xff);
			}

			if (m1snd_get_custom_tag_value("cmdmirror") != 0xffffffff)
			{
				z80_ram[m1snd_get_custom_tag_value("cmdmirror")] = cmda;
			}

			if (m1snd_get_custom_tag_value("cmdsubd0") != 0xffffffff)
			{
				write_rom_16(0x400, 0x7000 | (cmda & 0xff));	// moveq #cmda, D0

				m68k_set_reg(M68K_REG_PC, 0x400);
				timer_unspin_cpu(0);	// restart the 68000
			}

			if (m1snd_get_custom_tag_value("trigadr") != 0xffffffff)
			{
					z80_ram[m1snd_get_custom_tag_value("trigadr")] = m1snd_get_custom_tag_value("trigval");
			}

			if (m1snd_get_custom_tag_value("trig2adr") != 0xffffffff)
			{
					z80_ram[m1snd_get_custom_tag_value("trig2adr")] = (m1snd_get_custom_tag_value("trigval")>>8)&0xff;
					z80_ram[m1snd_get_custom_tag_value("trig2adr")+1] = (m1snd_get_custom_tag_value("trigval")&0xff);
			}
			break;

		case 2: // Sound Images
			timer_spin_cpu(0);	// kill the 68k now
			SI_PlaySound(cmda & 0xff);
			break;

	  	case 3:	// SMPS/Z80 (special)
			timer_spin_cpu(0);	// kill the 68k now

			z80_ram[0x1c0a] = (cmda & 0xFF);
			break;	

	  	case 5: // Test Drive II (DSI custom) - need to find how game looks up command addresses
			if (cmda != 0) 
			{
				// title song  = 78de0
				// ingame song = 7a624
				if (cmda == 1)
				{
					z80_ram[0x77] = 0xe0;
					z80_ram[0x76] = 0x8d;
					z80_ram[0x75] = 0x07;
					z80_ram[0x74] = 0x00;
				}
				else if (cmda == 2)  
				{
					z80_ram[0x77] = 0x24;
					z80_ram[0x76] = 0xa6;
					z80_ram[0x75] = 0x07;
					z80_ram[0x74] = 0x00;
				}
				z80_ram[0x71] = 1;
				z80_ram[0x70] = 1;
			}
			break;	

	  	case 6: // SMPS/68k
			if (m1snd_get_custom_tag_value("cmdadr") != 0xffffffff)
			{
			 	workram[m1snd_get_custom_tag_value("cmdadr")] = cmda & 0xff;
			}
			break;	
	}
}


// TD2 : z80 @ 812cc
// init: 91 = 1 70 = 1 90 = 6b
// ball: 91 = 1 7b = 72 7a = 2e 79 = 8 78 = 0 72 = 1 70 = 1
// mus : 70 = 0 77 = e0 76 = 8d 75 = 7 74 = 0 71 = 1 70 = 1


M1_BOARD_START( genesis )
	MDRV_NAME("Sega Genesis/MegaDrive")
	MDRV_HWDESC("68000, Z80, YM2612, SN76496")
	MDRV_INIT( Gen_Init )
	MDRV_SEND( Gen_SendCmd )
	MDRV_RUN( Gen_Frame )
	MDRV_DELAYS(1500, 350)

	MDRV_CPU_ADD(MC68000, MASTER_CLOCK / 7)
	MDRV_CPUMEMHAND(&gen_readwritemem)

	MDRV_CPU_ADD(Z80C, MASTER_CLOCK / 15)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)

	MDRV_SOUND_ADD(YM2612, &ym3438_intf)
	MDRV_SOUND_ADD(SN76496, &sn76489_intf)
M1_BOARD_END
