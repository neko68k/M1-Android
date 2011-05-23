/* CPS1 YM2151 board */

#include "m1snd.h"

#define Z80_CLOCK (3579545)

static void CPS1_Init(long srate);
static void CPS1_SendCmd(int cmda, int cmdb);
static void YM2151_IRQ(int irq);

static int cmd_latch, cmd_latch2;

static READ_HANDLER( bclatch_r )
{
	int old_latch = cmd_latch;

	cmd_latch = 0xff;

	return old_latch;
}

static READ_HANDLER( latch_r )
{
	return cmd_latch;
}

static READ_HANDLER( latch2_r )
{
	int ret = cmd_latch2;

	if (cmd_latch2 != 0xff) cmd_latch2 = 0xff;

	return ret;
}

static struct YM2151interface ym2151_interface =
{
	1,  /* 1 chip */
	3579580,    /* 3.579580 MHz ? */
	{ YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ }
};

static struct YM2151interface ym2151_interface_bionic =
{
	1,  /* 1 chip */
	3579545,
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
};

static struct OKIM6295interface okim6295_interface_6061 =
{
	1,  /* 1 chip */
	{ 6061 },
	{ REGION_SOUND1 },
	{ 30 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,  /* 1 chip */
	{ 7576 },
	{ RGN_SAMP1 },
	{ 15 }
};

static WRITE_HANDLER( cps1_snd_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
//	int length = memory_region_length(REGION_CPU1);
	int bankaddr;

	bankaddr = (data * 0x4000);
//	printf("bank to %x\n", bankaddr);
	cpu_setbank(1, &RAM[bankaddr + 0x8000]);

	if (data & 0xfe) logerror((char *)"%04x: write %02x to bankswitch\n",activecpu_get_pc(),data);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xf000, 0xf001, YM2151_status_port_0_r },
	{ 0xf002, 0xf002, OKIM6295_status_0_r },
	{ 0xf008, 0xf008, latch_r },
	{ 0xf00a, 0xf00a, latch2_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2151_register_port_0_w },
	{ 0xf001, 0xf001, YM2151_data_port_0_w },
	{ 0xf002, 0xf002, OKIM6295_data_0_w },
	{ 0xf004, 0xf004, cps1_snd_bankswitch_w },
//	{ 0xf006, 0xf006, MWA_NOP }, /* ???? Unknown ???? */
MEMORY_END

static MEMORY_READ_START( bcsound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8001, 0x8001, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, bclatch_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( bcsound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8000, YM2151_register_port_0_w },
	{ 0x8001, 0x8001, YM2151_data_port_0_w },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

M1_BOARD_START( capcomcps1 )
	MDRV_NAME("CPS-1")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_INIT( CPS1_Init )
	MDRV_SEND( CPS1_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

M1_BOARD_START( capcomcps1fw )
	MDRV_NAME("CPS-1 (alt. PCM rate)")
	MDRV_HWDESC("Z80, YM2151, MSM-6295")
	MDRV_INIT( CPS1_Init )
	MDRV_SEND( CPS1_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface_6061)
M1_BOARD_END

static void YM2151_IRQ(int irq)
{
//	printf("2151 IRQ: %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void CPS1_Init(long srate)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpu_setbank(1,&RAM[0x8000]);

	cmd_latch = 0xff;
	cmd_latch2 = 0; //xff;
}

static void CPS1_SendCmd(int cmda, int cmdb)
{
//	printf("Write %x to latch\n", cmda);
	cmd_latch = cmda;

	if (cmda >= 0xf0)
	{
		cmd_latch2 = cmda;
	}
	else
	{
		cmd_latch2 = 0;
	}
}

static void BC_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
}

static void bc_timer(int refcon);

static void bc_timer2(int refcon)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	timer_set(1.0/480.0, 0, bc_timer);
}

static void bc_timer(int refcon)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	timer_set(1.0/480.0, 0, bc_timer2);
}

static void BC_Init(long srate)
{
	timer_set(1.0/480.0, 0, bc_timer);

	m1snd_addToCmdQueueRaw(0x55);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0xff);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0xaa);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
}

M1_BOARD_START( bionicc )
	MDRV_NAME("Bionic Commando")
	MDRV_HWDESC("Z80, YM2151")
	MDRV_INIT( BC_Init )
	MDRV_SEND( BC_SendCmd )
	MDRV_DELAYS( 2000, 15 )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(bcsound_readmem,bcsound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface_bionic)
M1_BOARD_END

