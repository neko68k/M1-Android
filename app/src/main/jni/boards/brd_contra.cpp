/* Konami "Contra" and Thunder Cross and Jackal and Block Hole/Quarth - lone 2151 warriors */
/* Plus Super Contra */

#include "m1snd.h"

#define M6809_CLOCK (3579580)
#define YM_CLOCK (3579580)
#define Z80_CLOCK (3579580)

static void J_Init(long srate);
static void J_SendCmd(int cmda, int cmdb);
static void C_SendCmd(int cmda, int cmdb);
static void TX_SendCmd(int cmda, int cmdb);

static unsigned int C_Read(unsigned int address);
static void C_Write(unsigned int address, unsigned int data);
static unsigned int J_Read(unsigned int address);
static void J_Write(unsigned int address, unsigned int data);

static int cmd_latch, pstage;

static M16809T contrarwmem =
{
	C_Read,
	C_Write,
};

static M16809T jackalrwmem =
{
	J_Read,
	J_Write,
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580, 		
	{ YM3012_VOL(50,MIXER_PAN_RIGHT,50,MIXER_PAN_LEFT) },
	{ 0 }
};

static struct YM2151interface scym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
};

static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1 },	/* memory regions */
	{ K007232_VOL(4,MIXER_PAN_CENTER,4,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static void C_Init(long srate)
{
	// set stereo
	m1snd_addToCmdQueue(0x00);
	m1snd_addToCmdQueue(0x70);
	m1snd_addToCmdQueue(0x00);
}

M1_BOARD_START( contra )
	MDRV_NAME( "Contra" )
	MDRV_HWDESC( "6809, YM2151")
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( C_Init )
	MDRV_SEND( C_SendCmd )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&contrarwmem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
M1_BOARD_END

M1_BOARD_START( jackal )
	MDRV_NAME( "Jackal" )
	MDRV_HWDESC( "6809, YM2151")
	MDRV_DELAYS( 500, 15 )
	MDRV_INIT( J_Init )
	MDRV_SEND( J_SendCmd )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&jackalrwmem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
M1_BOARD_END

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( thunderx_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, latch_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( thunderx_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xe00c, 0xe00d, MWA_NOP },		/* leftover from missing 007232? */
MEMORY_END

M1_BOARD_START( thundercross )
	MDRV_NAME( "Thunder Cross" )
	MDRV_HWDESC( "Z80, YM2151")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( TX_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(thunderx_readmem_sound,thunderx_writemem_sound)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
M1_BOARD_END

static WRITE_HANDLER( scontra_snd_bankswitch_w )
{
	/* b3-b2: bank for chanel B */
	/* b1-b0: bank for chanel A */

	int bank_A = (data & 0x03);
	int bank_B = ((data >> 2) & 0x03);
	K007232_set_bank( 0, bank_A, bank_B );
}

static MEMORY_READ_START( scontra_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },				/* ROM */
	{ 0x8000, 0x87ff, MRA_RAM },				/* RAM */
	{ 0xa000, 0xa000, latch_r },			/* soundlatch_r */
	{ 0xb000, 0xb00d, K007232_read_port_0_r },	/* 007232 registers */
	{ 0xc001, 0xc001, YM2151_status_port_0_r },	/* YM2151 */
MEMORY_END

static MEMORY_WRITE_START( scontra_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },					/* ROM */
	{ 0x8000, 0x87ff, MWA_RAM },					/* RAM */
	{ 0xb000, 0xb00d, K007232_write_port_0_w },		/* 007232 registers */
	{ 0xc000, 0xc000, YM2151_register_port_0_w },	/* YM2151 */
	{ 0xc001, 0xc001, YM2151_data_port_0_w },		/* YM2151 */
	{ 0xf000, 0xf000, scontra_snd_bankswitch_w },	/* 007232 bank select */
MEMORY_END

M1_BOARD_START( scontra )
	MDRV_NAME( "Super Contra" )
	MDRV_HWDESC( "Z80, YM2151, K007232")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( TX_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(scontra_readmem_sound,scontra_writemem_sound)

	MDRV_SOUND_ADD(YM2151, &scym2151_interface)
	MDRV_SOUND_ADD(K007232, &k007232_interface)
M1_BOARD_END

static unsigned int C_Read(unsigned int address)
{
	if (address == 0)
	{
		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
		return cmd_latch;
	}
	
	if (address == 0x2001)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0x6000 && address <= 0x67ff)
	{
		return workram[address];
	}

	if (address >= 0x8000)
	{
		return prgrom[address-0x8000];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void C_Write(unsigned int address, unsigned int data)
{					
	if (address == 0x2000)
	{
		YM2151_register_port_0_w(0, data);
		return;
	}

	if (address == 0x2001)
	{
		YM2151_data_port_0_w(0, data);
		return;
	}

	if (address == 0x4000)
	{	
		return;
	}

	if (address >= 0x6000 && address <= 0x67ff)
	{
		workram[address] = data;
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static unsigned int J_Read(unsigned int address)
{
	if (address == 0x2001)
	{
		return YM2151ReadStatus(0);
	}

	if (address >= 0x4000 && address <= 0x7fff)
	{
		if ((address == 0x7c07) || (address == 0x7c06))
		{
			cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
		}
		return workram[address];
	}

	if (address >= 0x8000)
	{
		return prgrom[address];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void J_Write(unsigned int address, unsigned int data)
{					
	if (address == 0x2000)
	{
		YM2151_register_port_0_w(0, data);
		return;
	}

	if (address == 0x2001)
	{
		YM2151_data_port_0_w(0, data);
		return;
	}

	if (address >= 0x4000 && address <= 0x7fff)
	{
		// simulate bootup handshake with main CPU
		if (!pstage && address == 0x7c01)
		{
			pstage = 1;
			workram[0x7c00] = 0x06;
			workram[0x7c01] = 0x31;
			return;
		}
		else if ((pstage == 1) && address == 0x7c20)
		{
			if (data == 0)
			{
				pstage = 2;
				workram[0x7c1a] = 0xc0;
			}
		}
		
		workram[address] = data;
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void J_Init(long srate)
{
	pstage = 0;
}

static void C_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, M6809_IRQ_LINE, HOLD_LINE);
}

static void J_SendCmd(int cmda, int cmdb)
{
	workram[0x7c07] = 1;
	workram[0x7c06] = cmda;

	cpu_set_irq_line(0, M6809_IRQ_LINE, HOLD_LINE);
}

static void TX_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

