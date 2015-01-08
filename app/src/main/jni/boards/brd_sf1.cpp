/* Street Fighter 1 plus Tiger Road / Tora-he no Michi and F-1 Dream */

/* Darius (Taito) has a similar setup with master/slave 
   z80s where the master drives 2 YM2203s and the
   slave MSM-5205s.  Like this, the slave has no RAM... */

#include "m1snd.h"

#define SF1_Z80_CLOCK (3579545)	// both z80s at the same clock

static void SF1_Init(long srate);
static void SF1_SendCmd(int cmda, int cmdb);
static void YM2151_IRQ(int irq);

static int cmd_latch, cmd_latch2;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);

	// most values above 0x40 crash the primary CPU
	if (Machine->refcon == 0) 
	{
		if (cmd_latch > 0x3f) return 0xff;
	}

	return cmd_latch;	
}

static READ_HANDLER( latch2_r )
{
	return cmd_latch;
}

static struct YM2151interface ym2151_interface =
{
	1,	/* 1 chip */
	3579545,	/* ? xtal is 3.579545MHz */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ YM2151_IRQ }
};

static struct MSM5205interface msm5205_interface =
{
	2,		/* 2 chips */
	384000,				/* 384KHz ?           */
	{ 0, 0 },/* interrupt function */
	{ MSM5205_SEX_4B,MSM5205_SEX_4B},	/* 8KHz playback ?    */
	{ 100, 100 }
};

static WRITE_HANDLER( sound2_bank_w )
{
	cpu_setbank(1,memory_region(REGION_CPU2)+0x8000*(data+1));
}

static WRITE_HANDLER( msm5205_w )
{
	MSM5205_reset_w(offset,(data>>7)&1);
	/* ?? bit 6?? */
	MSM5205_data_w(offset,data);
	MSM5205_vclk_w(offset,1);
	MSM5205_vclk_w(offset,0);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, latch_r },
	{ 0xe001, 0xe001, YM2151_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
MEMORY_END


static MEMORY_READ_START( sound2_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

/* Yes, _no_ ram */
static MEMORY_WRITE_START( sound2_writemem )
	{ 0x0000, 0xffff, MWA_NOP },
MEMORY_END

static PORT_READ_START( sound2_readport )
	{ 0x01, 0x01, latch2_r },
PORT_END


static PORT_WRITE_START( sound2_writeport )
	{ 0x00, 0x01, msm5205_w },
	{ 0x02, 0x02, sound2_bank_w },
PORT_END

M1_BOARD_START( sf1 )
	MDRV_NAME("Street Fighter")
	MDRV_HWDESC("Z80(x2), YM2151, MSM-5205(x2)")
	MDRV_DELAYS( 800, 100 )
	MDRV_INIT( SF1_Init )
	MDRV_SEND( SF1_SendCmd )

	MDRV_CPU_ADD(Z80C, SF1_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_CPU_ADD(Z80C, SF1_Z80_CLOCK)
	MDRV_CPU_MEMORY(sound2_readmem,sound2_writemem)
	MDRV_CPU_PORTS(sound2_readport,sound2_writeport)
		
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, &msm5205_interface)
M1_BOARD_END

static void YM2151_IRQ(int irq)
{
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void vbl_timer(int refcon)
{
	cpu_set_irq_line(1, 0, ASSERT_LINE);
	cpu_set_irq_line(1, 0, CLEAR_LINE);

	timer_set(1.0/8000.0, 0, vbl_timer);
}

static void vbl_timer4k(int refcon)
{
	cpu_set_irq_line(1, 0, ASSERT_LINE);
	cpu_set_irq_line(1, 0, CLEAR_LINE);

	timer_set(1.0/4000.0, 0, vbl_timer);
}

static void SF1_Init(long srate)
{
	cmd_latch = 0;

	timer_set(1.0/8000.0, 0, vbl_timer);
}

static void TM_Init(long srate)
{
	cmd_latch = 0;
	cmd_latch2 = 0;

	timer_set(1.0/4000.0, 0, vbl_timer4k);
}

static void SF1_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	// kludge the samples for Tora-he no Michi, the main Z80 won't 
	// pass on the commands for some reason
	if (Machine->refcon == 1)
	{
		if (cmda >= 0x80) cmd_latch2 = cmda & 0x7f;
	}

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static WRITE_HANDLER( latch2_w )
{
	cmd_latch2 = data;
}

static READ_HANDLER( trlatch2_r )
{
	return cmd_latch2;
}

static MEMORY_READ_START( trsound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8000, YM2203_status_port_0_r },
	{ 0xa000, 0xa000, YM2203_status_port_1_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( trsound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM2203_control_port_1_w },
	{ 0xa001, 0xa001, YM2203_write_port_1_w },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( trsound_readport )
PORT_END

static PORT_WRITE_START( trsound_writeport )
	{ 0x7f, 0x7f, latch2_w },
PORT_END

static void irqhandler(int irq)
{
	cpu_set_irq_line(0,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,          /* 2 chips */
	3579545,    /* 3.579 MHz ? */
	{ YM2203_VOL(25,25), YM2203_VOL(25,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

M1_BOARD_START( tigerroad )
	MDRV_NAME("Tiger Road")
	MDRV_HWDESC("Z80, YM2203(x2)")
	MDRV_DELAYS( 800, 100 )
//	MDRV_INIT( SF1_Init )
	MDRV_SEND( SF1_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(trsound_readmem,trsound_writemem)
	MDRV_CPU_PORTS(trsound_readport,trsound_writeport)
		
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static struct MSM5205interface trmsm5205_interface =
{
	1,		/* 1 chip */
	384000,	/* 384KHz ? */
	{ 0 },	/* interrupt function */
	{ MSM5205_SEX_4B },	/* 4KHz playback ?  */
	{ 100 }
};

static MEMORY_READ_START( sample_readmem )
	{ 0x0000, 0xffff, MRA_ROM },
MEMORY_END

/* yes, no RAM */
static MEMORY_WRITE_START( sample_writemem )
	{ 0x0000, 0xffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( sample_readport )
	{ 0x00, 0x00, trlatch2_r },
PORT_END

static PORT_WRITE_START( sample_writeport )
	{ 0x01, 0x01, msm5205_w },
PORT_END

M1_BOARD_START( toramich )
	MDRV_NAME("Tora-he no Michi")
	MDRV_HWDESC("Z80(x2), YM2203(x2), MSM-5205")
	MDRV_DELAYS( 800, 100 )
	MDRV_INIT( TM_Init )
	MDRV_SEND( SF1_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(trsound_readmem,trsound_writemem)
	MDRV_CPU_PORTS(trsound_readport,trsound_writeport)
		
	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(sample_readmem,sample_writemem)
	MDRV_CPU_PORTS(sample_readport,sample_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(MSM5205, &trmsm5205_interface)
M1_BOARD_END

