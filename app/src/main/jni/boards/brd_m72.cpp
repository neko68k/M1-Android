/* Irem M72/73/82/84/85 systems */

#include "m1snd.h"

#define YM_CLOCK (3579545)
#define Z80_CLOCK (3579545)

static int cmd_latch;
static int sample_addr;

enum
{
	VECTOR_INIT,
	RST18_ASSERT,	// 1
	RST18_CLEAR,	// 2
	RST28_ASSERT,	// 3
	RST28_CLEAR	// 4
};

static void update_irq_lines(int param)
{
	static int irq1,irq2;

	switch(param)
	{
		case VECTOR_INIT:
			irq1 = irq2 = 0xff;
			return;
			break;

		case RST18_ASSERT:
			irq1 = 0xd7;
			break;

		case RST18_CLEAR:
			irq1 = 0xff;
			break;

		case RST28_ASSERT:
			irq2 = 0xdf;
			break;

		case RST28_CLEAR:
			irq2 = 0xff;
			break;
	}

	if ((irq1 & irq2) == 0xff)	/* no IRQs pending */
	{
//		printf("clear\n");
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
	else	/* IRQ pending */
	{
//		cpu_set_irq_line(0, 0, CLEAR_LINE);

		if (irq1 != 0xff)
		{
//			printf("RST18\n");
			z80_set_irqvec(Z80_RST_18);
			cpu_set_irq_line(0, 0, ASSERT_LINE);
		}
		else
		{
//			printf("RST28\n");
			z80_set_irqvec(Z80_RST_28);
			cpu_set_irq_line(0, 0, ASSERT_LINE);
		}		     
	}
}

static void YM2151_IRQ(int irq)
{
	if (irq)
	{
		update_irq_lines(RST28_ASSERT);
	}
	else
	{
		update_irq_lines(RST28_CLEAR);
	}
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(75,MIXER_PAN_LEFT,75,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ },
};

static struct DACinterface dac_interface =
{
	1,	/* 1 channel */
	{ 20 }
};

void m72_set_sample_start(int start)
{
	sample_addr = start;
}

static READ_HANDLER( z80_in_sh )
{
	unsigned char *ROM;
	int port = offset & 0xff;

	switch (port)
	{
		case 1:
		case 0x41:
			return YM2151ReadStatus(0);
			break;

		case 2:		// M72 cmd latch
		case 0x42:	// M97 cmd latch
		case 0x80:	// M84 cmd latch
			update_irq_lines(RST18_CLEAR);
			return cmd_latch&0xff;
			break;

		case 0x84:	// DAC memory read
			ROM = rom_getregion(RGN_SAMP1);
			if (!ROM) return 0;
			return ROM[sample_addr];
			break;
	}

//	printf("Unknown read from port %x (PC=%x)\n", port, _z80_get_reg(Z80_REG_PC));
	return 0;
}

WRITE_HANDLER( rtype2_sample_addr_w )
{
	sample_addr >>= 5;

	if (offset == 1)
		sample_addr = (sample_addr & 0x00ff) | ((data << 8) & 0xff00);
	else
		sample_addr = (sample_addr & 0xff00) | ((data << 0) & 0x00ff);

	sample_addr <<= 5;
}

static int a[31] = { 0x00000, 0x00020, 0x00040, 0x01360, 0x02580, 0x04f20, 0x06240, 0x076e0,
							      0x08660, 0x092a0, 0x09ba0, 0x0a560, 0x0cee0, 0x0de20, 0x0e620, 0x0f1c0,
								  0x10200, 0x10220, 0x10240, 0x11380, 0x12760, 0x12780, 0x127a0, 0x13c40,
								  0x140a0, 0x16760, 0x17e40, 0x18ee0, 0x19f60, 0x1bbc0, 0x1cee0 };
static int b[16] = { 0x00000, 0x00020, 0x03ec0, 0x05640, 0x06dc0, 0x083a0, 0x0c000, 0x0eb60, 0x112e0, 0x13dc0, 0x16520, 0x16d60, 0x18ae0, 0x1a5a0, 0x1bf00, 0x1c340 };
static int c[9] = { 0x00000, 0x00020, 0x02c40, 0x08160, 0x0c8c0, 0x0ffe0, 0x13000, 0x15820, 0x15f40 }; 
static int d[6] = { 0x0000, 0x0010, 0x2510, 0x6510, 0x8510, 0x9310 };
static int e[9] = { 0x0000, 0x0020, 0x2020, 0, 0x5720, 0, 0x7b60, 0x9b60, 0xc360 };
static int f[7] = { 0x0000, 0x0020, 0x44e0, 0x98a0, 0xc820, 0xf7a0, 0x108c0 };
static int g[7] = { 0x0000, 0x0020, 0, 0x2c40, 0x4320, 0x7120, 0xb200 };
static int h[3] = { 0x0000, 0x0020, 0x1a40 };
static int i[28] = { 0x00000, 0x00020, 0x01800, 0x02da0, 0x03be0, 0x05ae0, 0x06100, 0x06de0,
			      0x07260, 0x07a60, 0x08720, 0x0a5c0, 0x0c3c0, 0x0c7a0, 0x0e140, 0x0fb00,
				  0x10fa0, 0x10fc0, 0x10fe0, 0x11f40, 0x12b20, 0x130a0, 0x13c60, 0x14740,
				  0x153c0, 0x197e0, 0x1af40, 0x1c080 };

static WRITE_HANDLER( z80_out_sh )
{
	int port = offset & 0xff;

//	printf("Z80 out %x to port %x\n", data, port);

	switch (port)
	{
		case 0:
		case 0x40:
			YM2151_register_port_0_w(0, data);
			break;

		case 1:
		case 0x41:
			YM2151_data_port_0_w(0, data);
			break;

		case 4:	// M90 ???
			break;

		case 0x42:	// M90 IRQ ack
			break;

		case 0x46:	// M90 ???
		case 0x47:
			break;

		case 6:	// command ack
		case 0x83:	// M84 command ack
			break;

		case 0x80:	// sample address lsb
		case 0x81:
			rtype2_sample_addr_w(port-0x80, data);
			break;

		case 0x82:	// sample_w
//			printf("write %x to DAC\n", data);
			DAC_signed_data_w(0, data);
			sample_addr++;
			sample_addr &= (rom_getregionsize(RGN_SAMP1)-1);
			break;

		case 0x87:	// NMI enable
			break;

		case 0xc0:	// sample trigger
//			printf("%x to sample trigger\n", data);
			switch (Machine->refcon)
			{
				case 1:	// x multiply
					if (data < 3) { m72_set_sample_start(h[data]); }
					break;

				case 2:	// ninja spirits
					if (data < 9) { m72_set_sample_start(e[data]); }
					break;

				case 3:	// dragon breed
					if (data < 9) { m72_set_sample_start(c[data]); }
					break;

				case 4:	// image fight
					if (data < 7) { m72_set_sample_start(f[data]); }
					break;

				case 5:	// legend of hero tonma
					if (data < 7) { m72_set_sample_start(g[data]); }
					break;

				case 6:	// bchopper
					if (data < 6) { m72_set_sample_start(d[data]); }
					break;
	
				case 7:	// gallop/cosmic cop
					if (data < 31) m72_set_sample_start(a[data]);
					break;

				case 8:	// air duel
					if (data < 16) { m72_set_sample_start(b[data]); }
					break;

				case 9:	// hammerin' harry
					if (data < 28) m72_set_sample_start(i[data]);
					break;

				default:
					break;
			}
			break;

		default:
//			printf("Unknown write %x to port %x\n", data, port);
			break;
	}
}

static void nmi_timer(int refcon)
{
	int sample = z80_in_sh(0x84);
	if (sample)
		z80_out_sh(0x82, sample);

	timer_set(1.0/(60.0*128.0), 0, nmi_timer);
}

static void M72_Init(long srate)
{
	int offset = 0;

	update_irq_lines(VECTOR_INIT);
	
	switch (Machine->refcon)
	{
		case 1: //GAME_XMULTIPLY:
			offset = 0xe000;
			break;
		case 2: //GAME_NINJASPIRIT:
//		case GAME_NINJASPIRITJAPAN:
			offset = 0x11000;
			break;
		case 3: //GAME_DRAGONBREED:
//		case GAME_COSMICCOP:
		case 7: //GAME_GALLOP:
		case 8: //GAME_AIRDUEL:
		case 9:	//GAME_HHARRY:
			offset = 0x30000;
			break;
		case 4: //GAME_IMAGEFIGHT:
			offset = 0x22000;
			break;
		case 5: //GAME_LEGENDOFHEROTONMA:
			offset = 0x18000*2;
			break;
		case 6: //GAME_BATTLECHOPPER:
//		case GAME_MRHELI:
			offset = 0x400;
			break;
	}

	memcpy(workram, prgrom+offset, 0x10000);

	m1snd_addToCmdQueue(2);
	m1snd_addToCmdQueue(0);

	timer_set(1.0/(128.0), 0, nmi_timer);
}

static void M72_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	if (cmda >= 0x80)
	{
		cmd_latch = games[curgame].stopcmd;
		z80_out_sh(0xc0, cmda-0x80);
	}

	update_irq_lines(RST18_ASSERT);
}

static READ_HANDLER( readram )
{
	return workram[offset];
}

static WRITE_HANDLER( writeram )
{
	workram[offset] = data;
}

static MEMORY_READ_START( m72_read )
	{ 0x0000, 0xffff, readram },
MEMORY_END

static MEMORY_WRITE_START( m72_write )
	{ 0x0000, 0xffff, writeram },
MEMORY_END

static PORT_READ_START( m72_readport )
	{ 0x00, 0xff, z80_in_sh },
PORT_END

static PORT_WRITE_START( m72_writeport )
	{ 0x00, 0xff, z80_out_sh },
PORT_END

M1_BOARD_START( m72 )
	MDRV_NAME( "Irem M72/M81/M83/M97/M99" )
	MDRV_HWDESC( "Z80, YM2151, DAC")
	MDRV_DELAYS( 100, 60 )
	MDRV_INIT( M72_Init )
	MDRV_SEND( M72_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY( m72_read, m72_write )
	MDRV_CPU_PORTS( m72_readport, m72_writeport )

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END

