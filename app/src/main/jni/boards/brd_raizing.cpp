/* Raizing 68k+Z80 games */
/* Z80 + YM2151 + MSM6295(x2) */
/* Z80 + YMZ280B */

#include "m1snd.h"

#define Z80_CLOCK (4000000)
// this is *way* off from MAME.  something weird here.
#define TIMER_RATE (1.0/450.0)

static void Raiz_Init(long srate);
static void Raiz2_Init(long srate);
static void Raiz3_Init(long srate);
static void Raiz4_Init(long srate);
static void Raiz_SendCmd(int cmda, int cmdb);
static void Raiz2_SendCmd(int cmda, int cmdb);
static void Raiz3_SendCmd(int cmda, int cmdb);
static void Raiz4_SendCmd(int cmda, int cmdb);

static int bInit, cmd_latch, port48, port4a;
static unsigned char *raizing1_shared_ram;
static int bankno[512];

static struct YMZ280Binterface ymz280b_interface =
{
	1,
	{ 16934400 },
	{ RGN_SAMP1 },
	{ YM3012_VOL(0,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) },
};

static struct YM2151interface raiz1_ym2151_interface =
{
	1,				/* 1 chip */
	27000000/8,		/* 3.375MHz , 27MHz Oscillator */
	{ YM3012_VOL(25,MIXER_PAN_LEFT,25,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct YM2151interface ym2151_interface =
{
	1,				/* 1 chip */
	32000000/8,		/* 4.00MHz , 32MHz Oscillator */
	{ YM3012_VOL(65,MIXER_PAN_LEFT,65,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,						/* 1 chip */
	{ 32000000/32/132 },	/* frequency (Hz) 1MHz to 6295 (using B mode) */
	{ RGN_SAMP1 },		/* memory region */
	{ 45 }
};

static struct OKIM6295interface battleg_okim6295_interface =
{
	1,						/* 1 chip */
	{ 32000000/16/132 },	/* frequency (Hz). 2MHz to 6295 (using B mode) */
	{ RGN_SAMP1 },		/* memory region */
	{ 45 }
};

static struct OKIM6295interface batrider_okim6295_interface =
{
	2,										/* 2 chips */
	{ 32000000/10/132, 32000000/10/165 },	/* frequency (Hz). 3.2MHz to two 6295 (using B mode / A mode) */
	{ RGN_SAMP1, RGN_SAMP2 },		/* memory region */
	{ 35, 35 }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static void raizing_oki6295_set_bankbase( int chip, int channel, int base )
{
	/* The OKI6295 ROM space is divided in four banks, each one independantly */
	/* controlled. The sample table at the beginning of the addressing space  */
	/* is divided in four pages as well, banked together with the sample data */

	data8_t *rom = (data8_t *)rom_getregion(RGN_SAMP1 + chip);

	/* copy the samples */
	memcpy(rom + channel * 0x10000, rom + 0x40000 + base, 0x10000);

	/* and also copy the samples address table */
	rom += channel * 0x100;
	memcpy(rom, rom + 0x40000 + base, 0x100);
}

static WRITE_HANDLER( battleg_bankswitch_w )
{
	data8_t *RAM = (data8_t *)memory_region(REGION_CPU1);
	int bankaddress;
	int bank;

	bank = (data & 0x0f) - 10;

	bankaddress = 0x8000 + 0x4000 * bank;
	cpu_setbank(1, &RAM[bankaddress]);
}

static WRITE_HANDLER( batrider_bankswitch_w )
{
	data8_t *RAM = (data8_t *)memory_region(REGION_CPU1);
	int bankaddress;
	int bank;

	bank = data & 0x0f;
	bankaddress = 0x4000 * bank;
	cpu_setbank(1, &RAM[bankaddress]);
}

static READ_HANDLER( raiz_readport )
{
	switch (offset & 0xff)
	{
		case 0x48:
//			printf("read 48\n");
			return port48;
			break;

		case 0x4a:
//			printf("read 4a\n");
			return port4a;
			break;

		case 0x81:
			return YM2151_status_port_0_r(0);
			break;

		case 0x82:
			return OKIM6295_status_0_r(0);
			break;

		case 0x84:
			return OKIM6295_status_1_r(0);
			break;
	}
	return 0;
}

static WRITE_HANDLER( raiz_writeport )
{
	switch (offset & 0xff)
	{
		case 0x40:
		case 0x42:
		case 0x44:
			break;

		case 0x46:	// clear NMI
			cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
			break;

		case 0x80:
			YM2151_register_port_0_w(0, data);
			break;

		case 0x81:
			YM2151_data_port_0_w(0, data);
			break;

		case 0x82:
			OKIM6295_data_0_w(0, data);
			break;

		case 0x84:
			OKIM6295_data_1_w(0, data);
			break;

		case 0x88:	// z80 bankswitch
			batrider_bankswitch_w(0, data);
			break;
							      
		case 0xc0:	// 6295 bankswitches
			raizing_oki6295_set_bankbase( 0, 0,  (data       & 0x0f) * 0x10000);
			raizing_oki6295_set_bankbase( 0, 1, ((data >> 4) & 0x0f) * 0x10000);
			break;

		case 0xc2:
			raizing_oki6295_set_bankbase( 0, 2,  (data       & 0x0f) * 0x10000);
			raizing_oki6295_set_bankbase( 0, 3, ((data >> 4) & 0x0f) * 0x10000);
			break;

		case 0xc4:
			raizing_oki6295_set_bankbase( 1, 0,  (data       & 0x0f) * 0x10000);
			raizing_oki6295_set_bankbase( 1, 1, ((data >> 4) & 0x0f) * 0x10000);
			break;

		case 0xc6:
			raizing_oki6295_set_bankbase( 1, 2,  (data       & 0x0f) * 0x10000);
			raizing_oki6295_set_bankbase( 1, 3, ((data >> 4) & 0x0f) * 0x10000);
			break;
	}
}

static WRITE_HANDLER( raizing_okim6295_bankselect_0 )
{
	raizing_oki6295_set_bankbase( 0, 0,  (data       & 0x0f) * 0x10000);
	raizing_oki6295_set_bankbase( 0, 1, ((data >> 4) & 0x0f) * 0x10000);
}

static WRITE_HANDLER( raizing_okim6295_bankselect_1 )
{
	raizing_oki6295_set_bankbase( 0, 2,  (data       & 0x0f) * 0x10000);
	raizing_oki6295_set_bankbase( 0, 3, ((data >> 4) & 0x0f) * 0x10000);
}

#if 0
static WRITE_HANDLER( raizing_okim6295_bankselect_2 )
{
	raizing_oki6295_set_bankbase( 1, 0,  (data       & 0x0f) * 0x10000);
	raizing_oki6295_set_bankbase( 1, 1, ((data >> 4) & 0x0f) * 0x10000);
}

static WRITE_HANDLER( raizing_okim6295_bankselect_3 )
{
	raizing_oki6295_set_bankbase( 1, 2,  (data       & 0x0f) * 0x10000);
	raizing_oki6295_set_bankbase( 1, 3, ((data >> 4) & 0x0f) * 0x10000);
}
#endif

static READ_HANDLER( read_share )
{
	// handle boot protocol
	if ((!bInit) && (offset == 1))
	{

		bInit = 1;
		raizing1_shared_ram[0] = raizing1_shared_ram[1] = 0xfe;
	}

	return raizing1_shared_ram[offset];
}

static MEMORY_READ_START( raizing_sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, read_share },
	{ 0xe001, 0xe001, YM2151_status_port_0_r },
	{ 0xe004, 0xe004, OKIM6295_status_0_r },
MEMORY_END

static MEMORY_WRITE_START( raizing_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM, &raizing1_shared_ram },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
	{ 0xe004, 0xe004, OKIM6295_data_0_w },
	{ 0xe00e, 0xe00e, MWA_NOP },
MEMORY_END

static MEMORY_READ_START( battleg_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe001, 0xe001, YM2151_status_port_0_r },
	{ 0xe004, 0xe004, OKIM6295_status_0_r },
	{ 0xe01c, 0xe01c, latch_r },
	{ 0xe01d, 0xe01d, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( battleg_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM, &raizing1_shared_ram },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
	{ 0xe004, 0xe004, OKIM6295_data_0_w },
	{ 0xe006, 0xe006, raizing_okim6295_bankselect_0 },
	{ 0xe008, 0xe008, raizing_okim6295_bankselect_1 },
	{ 0xe00a, 0xe00a, battleg_bankswitch_w },
	{ 0xe00c, 0xe00c, MWA_NOP },
MEMORY_END

static MEMORY_READ_START( batrider_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( batrider_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x0000, 0xffff, raiz_readport },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x0000, 0xffff, raiz_writeport },
PORT_END

M1_BOARD_START( raizing )
	MDRV_NAME("Raizing (type 1)")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_DELAYS( 300, 100 )
	MDRV_INIT( Raiz_Init )
	MDRV_SEND( Raiz_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(raizing_sound_readmem,raizing_sound_writemem)
	MDRV_CPU_PORTS(readport,writeport)

	MDRV_SOUND_ADD(YM2151, &raiz1_ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

M1_BOARD_START( raizing2 )
	MDRV_NAME("Raizing (type 2)")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_DELAYS( 300, 100 )
	MDRV_INIT( Raiz2_Init )
	MDRV_SEND( Raiz2_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(battleg_sound_readmem,battleg_sound_writemem)
	MDRV_CPU_PORTS(readport,writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &battleg_okim6295_interface)
M1_BOARD_END

M1_BOARD_START( raizing3 )
	MDRV_NAME("Raizing (type 3)")
	MDRV_HWDESC("Z80, YM2151, MSM-6295(x2)")
	MDRV_DELAYS( 300, 100 )
	MDRV_INIT( Raiz3_Init )
	MDRV_SEND( Raiz3_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(batrider_sound_readmem,batrider_sound_writemem)
	MDRV_CPU_PORTS(readport,writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &batrider_okim6295_interface)
M1_BOARD_END

static READ_HANDLER( raiz4_readport )
{
	switch (offset & 0xff)
	{
		case 0x48:
			return port48;
			break;

		case 0x4a:
			return port4a;
			break;

		case 0x81:	// ymz280 status
//			printf("read YMZ status\n");
			return YMZ280B_status_0_r(0);
			break;

		default:
//			printf("Unknown port read at %x\n", port);
			break;
	}
	return 0;
}

static WRITE_HANDLER( raiz4_writeport )
{
	switch (offset & 0xff)
	{
		case 0x40:
		case 0x42:
		case 0x44:
			break;

		case 0x46:	// clear NMI
			cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
			break;

		case 0x80:
			YMZ280B_register_0_w(0, data);
			break;

		case 0x81:
			YMZ280B_data_0_w(0, data);
			break;

		default:
//			printf("Unknown port write %x to %x (PC=%x)\n", val, port&0xff, _z80_get_reg(Z80_REG_PC));
			break;
	}
}

// sorcerer striker & kingdom grand prix
static void Raiz_Init(long srate)
{
	static int ship_reqbankdef[]=
	{
                0x06,0x07,0x08,0x0e,0x60,0x61,0x62,0x63,0x64,-1,
        };
	int i;

        for(i=0;i<0x200;i++)    
	{
		bankno[i]=0;
	}
        if (Machine->refcon == 1)
        {
                i=0;
                while (ship_reqbankdef[i] != -1)   
		{
			bankno[ship_reqbankdef[i]]=1;  
			i++;
		}

                for(i=0x100;i<0x200;i++)                
		{
			bankno[i]=1;    //0x100 - 1ff : B bank
		}
        }

	bInit = 0;
}

// battle garegga
static void Raiz2_Init(long srate)
{
	bInit = 0;
	cmd_latch = 0x55;

	m1snd_addToCmdQueue(0x55);
}

// armed police batrider
static void Raiz3_Init(long srate)
{
	port48 = 0x55;
	port4a = 0;

	m1snd_addToCmdQueue(0x5500);
}

static void gx_timer(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, gx_timer);
}

static MEMORY_READ_START( bbakraid_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( bbakraid_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },	/* Only 2FFFh valid code */
	{ 0xc000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( bbakraid_sound_readport )
	{ 0x0000, 0xffff, raiz4_readport },
PORT_END

static PORT_WRITE_START( bbakraid_sound_writeport )
	{ 0x0000, 0xffff, raiz4_writeport },
PORT_END

M1_BOARD_START( raizing4 )
	MDRV_NAME("Raizing (type 4)")
	MDRV_HWDESC("Z80, YMZ280B")
	MDRV_DELAYS( 600, 100 )
	MDRV_INIT( Raiz4_Init )
	MDRV_SEND( Raiz4_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(bbakraid_sound_readmem, bbakraid_sound_writemem)
	MDRV_CPU_PORTS(bbakraid_sound_readport, bbakraid_sound_writeport)

	MDRV_SOUND_ADD(YMZ280B, &ymz280b_interface)
M1_BOARD_END

// battle bakraid
static void Raiz4_Init(long srate)
{
	timer_set(TIMER_RATE, 0, gx_timer);
}

static void Raiz_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xff00)
	{
		// stop
		raizing1_shared_ram[0] = 0;
		raizing1_shared_ram[1] = 1;
	}
	else
	{
		raizing1_shared_ram[0] = cmda;
		raizing1_shared_ram[1] = 6;

		if (Machine->refcon == 1)
		{
			OKIM6295_set_bank_base(0, bankno[cmda] * 0x40000);
		}
	}
}

static void Raiz2_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static void Raiz3_SendCmd(int cmda, int cmdb)
{
	switch (cmda)
	{
		case 0x5500:	// enabler
			port48 = 0x55;
			port4a = 0;
			break;

		case 0x9900:	// stop
			port48 = 0x1;
			port4a = 0;
			break;

		default:
			port48 = 0;
			port4a = cmda;
			break;
	}

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void Raiz4_SendCmd(int cmda, int cmdb)
{
	if (cmda > 0x100)
	{
		port48 = cmda>>8;
		port4a = cmda&0xff;
	}
	else
	{
		port48 = 1;
		port4a = cmda-1;
	}

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
