/* Taito YM2203 boards */

#include "m1snd.h"

static void TNZS_Init(long srate);
static void TNZS_SendCmd(int cmda, int cmdb);

static int latch;

static int iostatus[2][3]={
	{0x55,0xaa,0x5a},
	{0x5a,0xa5,0x55},
};

static int *pstatus, mc_cnt;

static READ_HANDLER( input_port_0_r )
{
	return 0;
}

static void tnzs_timer(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	if (Machine->refcon != 1)
		cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(1.0/60.0, 0, tnzs_timer);
}

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz ??? */
	{ YM2203_VOL(30,30) },
	{ input_port_0_r },		/* DSW1 connected to port A */
	{ input_port_0_r },		/* DSW2 connected to port B */
	{ 0 },
	{ 0 },
};

static struct DACinterface dac_interface =
{
	1,
	{ MIXER(20,MIXER_PAN_CENTER) }
};

static void YM_IRQ(int irq)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, irq ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return latch;
}

static WRITE_HANDLER( kabukiz_sound_bank_w )
{
	// to avoid the write when the sound chip is initialized
	if(data != 0xff)
	{
		unsigned char *ROM = memory_region(REGION_CPU1);
		cpu_setbank(2, &ROM[0x4000 * (data & 0x07)]);
	}
}

static WRITE_HANDLER( kabukiz_sample_w )
{
	// to avoid the write when the sound chip is initialized
	if(data != 0xff)
	{
		DAC_data_w( 0, data );
	}
}

static struct YM2203interface ym2203_kz_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz ??? */
	{ YM2203_VOL(100,100) },
	{ input_port_0_r },		/* DSW1 connected to port A */
	{ input_port_0_r },		/* DSW2 connected to port B */
	{ kabukiz_sound_bank_w },
	{ kabukiz_sample_w },
	{ YM_IRQ }
};

WRITE_HANDLER( tnzs_bankswitch1_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

//	printf("PC %04x: writing %02x to bankswitch 1\n", activecpu_get_pc(),data);

	/* bit 2 resets the mcu */
//	if (data & 0x04) 
//	{
//		mcu_reset();
//	}

	/* bits 0-1 select ROM bank */
	cpu_setbank (2, &RAM[0x8000 + 0x2000 * (data & 3)]);
}

READ_HANDLER( tnzs_workram_r )
{
	return workram[offset];
}

WRITE_HANDLER( tnzs_workram_w )
{
	workram[offset] = data;
}

READ_HANDLER( tnzs_workram2_r )
{
	return workram[offset+0x1000];
}

WRITE_HANDLER( tnzs_workram2_w )
{
//	printf("w %x @ %x\n", data, offset);
	if (Machine->refcon == 1)
	{
		if (offset == 0 && data == 1)
		{
			cpu_set_irq_line(0, 0, CLEAR_LINE);
		}
	}
	workram[offset+0x1000] = data;
}

READ_HANDLER( tnzs_mcu_r )
{
	int rv;

	if (offset == 1) return 1;

	if (mc_cnt <= 2)
	{
		rv = pstatus[mc_cnt];
		mc_cnt++;
	}
	else
	{
		rv = 0;
	}

	return rv;
}

static MEMORY_READ_START( sub_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK2 },
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb001, 0xb001, YM2203_read_port_0_r },
	{ 0xc000, 0xc001, tnzs_mcu_r },	/* plain input ports in insectx (memory handler */
									/* changed in insectx_init() ) */
	{ 0xd000, 0xdfff, tnzs_workram2_r },
	{ 0xe000, 0xefff, tnzs_workram_r },
	{ 0xf000, 0xf003, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xc000, 0xc001, MWA_NOP },
	{ 0xd000, 0xdfff, tnzs_workram2_w },
	{ 0xe000, 0xefff, tnzs_workram_w },
MEMORY_END

static MEMORY_READ_START( kz_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( kz_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( kz_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x02, 0x02, latch_r },
PORT_END

static PORT_WRITE_START( kz_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END

M1_BOARD_START( tnzs )
	MDRV_NAME("The New Zealand Story")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_INIT( TNZS_Init )
	MDRV_SEND( TNZS_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static void KZ_Init(long srate)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpu_setbank (2, &RAM[0x4000]);
}

static void KZ_SendCmd(int cmda, int cmdb)
{
	latch = cmda;
	z80_set_irqvec(Z80_RST_38);
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

M1_BOARD_START( kabukiz )
	MDRV_NAME("Kabuki-Z")
	MDRV_HWDESC("Z80, YM2203, DAC")
	MDRV_INIT( KZ_Init )
	MDRV_SEND( KZ_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(kz_readmem,kz_writemem)
	MDRV_CPU_PORTS(kz_readport,kz_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_kz_interface)
	MDRV_SOUND_ADD(DAC, &dac_interface)
M1_BOARD_END

static void TNZS_Init(long srate)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpu_setbank (2, &RAM[0x8000]);

	mc_cnt = 0;
	switch (Machine->refcon)
	{
		case 3:	// arkanoid 2
			pstatus = iostatus[0];
			break;

		case 1: // TNZS
		case 2: // Plump Pop
			pstatus = iostatus[1];
			workram[0xf11]=1;
			break;
	}

	m1snd_addToCmdQueue(0xef);

	timer_set(1.0/60.0, 0, tnzs_timer);
}

static void TNZS_SendCmd(int cmda, int cmdb)
{
	switch (Machine->refcon)
	{
		case 3: // arkanoid 2
			workram[0x733] = cmda;
			break;

		case 2: // Plump Pop
		case 1: // TNZS
			workram[0xf10] = cmda;
			break;

		default:
			logerror((char *)"brd_tnzs: unknown game!\n");
			break;
	}
}


/*
	Kuri Kinton
*/

static void *vblank_timer;

static READ_HANDLER( ram_r )
{
	return workram[offset];
}

static WRITE_HANDLER( ram_w )
{
	workram[offset] = data;
}

static MEMORY_READ_START( kurikint_snd_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, ram_r },
	{ 0xe800, 0xe800, YM2203_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( kurikint_snd_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, ram_w },
	{ 0xe800, 0xe800, YM2203_control_port_0_w },
	{ 0xe801, 0xe801, YM2203_write_port_0_w },
MEMORY_END

static void YM_IRQHandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface2 =
{
	1,
	3000000,
	{ YM2203_VOL(80, 20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ YM_IRQHandler }
};

static void irq_off(int ref)
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void timer_callback(int ref)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	timer_set(TIME_IN_NSEC(300), 0, irq_off);
}

static void KuriKinton_Init(long foo)
{
	vblank_timer = timer_pulse(TIME_IN_HZ(60), 0, timer_callback);
	workram[0x7f0] = 0xef;
}

static void KuriKinton_SendCmd(int cmda, int cmdb)
{
	workram[0x7f0] = cmda;
}


M1_BOARD_START( kurikint )
	MDRV_NAME("Kuri Kinton")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_INIT( KuriKinton_Init )
	MDRV_SEND( KuriKinton_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(kurikint_snd_readmem, kurikint_snd_writemem)
	MDRV_SOUND_ADD(YM2203, &ym2203_interface2)
M1_BOARD_END



/*
	KiKi KaiKai
*/

static void Kikikai_Init(long foo)
{
	vblank_timer = timer_pulse(TIME_IN_HZ(60), 0, timer_callback);
	workram[0x1fff] = 0xff;
}

static void delayed_cmd(int ref)
{
	workram[0x1fff] = ref;
}

static void Kikikai_SendCmd(int cmda, int cmdb)
{
	workram[0x1fff] = 0xef;
	timer_set(TIME_IN_USEC(1000), cmda, delayed_cmd);
}


static READ_HANDLER( kiki_2203_r )
{
	return YM2203_status_port_0_r(0) & 0x7f;
}

static MEMORY_READ_START( kikikai_snd_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xa7ff, ram_r },
	{ 0xa800, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc000, kiki_2203_r },
	{ 0xc001, 0xc001, YM2203_read_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( kikikai_snd_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xa7ff, ram_w },
	{ 0xa800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc000, YM2203_control_port_0_w },
	{ 0xc001, 0xc001, YM2203_write_port_0_w },
MEMORY_END

M1_BOARD_START( kikikai )
	MDRV_NAME("KiKi KaiKai")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_INIT( Kikikai_Init )
	MDRV_SEND( Kikikai_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(kikikai_snd_readmem, kikikai_snd_writemem)
	MDRV_SOUND_ADD(YM2203, &ym2203_interface2)
M1_BOARD_END
