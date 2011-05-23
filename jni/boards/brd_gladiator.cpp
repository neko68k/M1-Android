// Taito Gladiator

#include "m1snd.h"
#include "taitosnd.h"

static void gx_timer(int refcon);
static int cmd_latch, rdstate = 0, pcm_latch, pcm_bank = 0;

static void gx_timer2(int refcon)
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	timer_set(1.0/120.0, 0, gx_timer);
}

static void gx_timer(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);

	timer_set(1.0/120.0, 0, gx_timer2);
}

static void GLD_Init(long srate)
{
	cmd_latch = 0;
	rdstate = 0;
	pcm_bank = 0;
	pcm_latch = 0;

	timer_set(1.0/60.0, 0, gx_timer);
}

void GLD_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;	
	rdstate = 0;
}

static READ8_HANDLER( fake_8741_comm_r )
{
	int rv;
	if (offset&1) 
	{
		rv = 1;
	}
	else
	{
		if (rdstate < 32)
		{
			rv = cmd_latch;
			rdstate++;
		}
		else
		{
			rv = 0;
		}
	}

//	printf("8741_1_r: ofs %d => %02x\n", offset&1, rv);

	return rv;
}

static READ8_HANDLER( fake_8741_0_r )
{
	// see code at 0x332: 0x61 must have bit 1 set, and 0x60 must be 0x48
	if (offset) return 1;

	return 0x48;
}

static READ8_HANDLER( fake_8741_1_r )
{
	// see code at 0x2fc: 0x81 must have bit 1 set, and 0x80 must be 0x66
	if (offset) return 1;

	return 0x66;
}

static WRITE8_HANDLER( glad_pcm_latch_w )
{
	pcm_latch = data;
}

static READ8_HANDLER( glad_pcm_latch_r )
{
	return pcm_latch;
}

static READ8_HANDLER( glad_pcm_rom_r )
{
	data8_t *ROM = rom_getregion(RGN_CPU2);

	return ROM[offset + pcm_bank];
}

static WRITE8_HANDLER( glad_adpcm_w )
{
	/* bit6 = bank offset */
	pcm_bank = (data & 0x40) ? 0xc000 : 0;

	MSM5205_data_w(0,data);         /* bit0..3  */
	MSM5205_reset_w(0,(data>>5)&1); /* bit 5    */
	MSM5205_vclk_w (0,(data>>4)&1); /* bit4     */
}

static PORT_READ_START( glad_sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
	{ 0x20, 0x21, fake_8741_comm_r },
	{ 0x60, 0x61, fake_8741_0_r },
	{ 0x80, 0x81, fake_8741_1_r },
PORT_END

static PORT_WRITE_START( glad_sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0xe0, 0xe0, glad_pcm_latch_w },
PORT_END

static MEMORY_READ_START( glad_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( glad_sound_writemem )
	{ 0x0000, 0x7fff, MWA_NOP },
	{ 0x8000, 0x83ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( glad_readmem_pcm )
	{ 0x2000, 0x2fff, glad_pcm_latch_r },
	{ 0x4000, 0xffff, glad_pcm_rom_r },
MEMORY_END

static MEMORY_WRITE_START( glad_writemem_pcm )
	{ 0x1000, 0x1fff, glad_adpcm_w },
MEMORY_END

static void GYM_IRQHandler(int irq)
{
	if (irq)
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	else
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
}

static struct YM2203interface gym2203_interface =
{
	1,			/* 2 chips */
	12000000/8,
	{ YM2203_VOL(60,20), YM2203_VOL(60,20) },
	{ 0, 0 },		/* portA read */
	{ 0, 0 },
	{ NULL, NULL },	/* portA write */
	{ NULL, NULL },	/* portB write */
	{ GYM_IRQHandler, 0 }
};

static struct MSM5205interface msm5205_interface =
{
	1,				/* 1 chip */
	384000, 			/* 384KHz */
	{ 0 },				/* interrupt function */
	{ MSM5205_SEX_4B },		/* vclk input mode    */
	{ 50 }				/* volume */
};

M1_BOARD_START( gladiatr )
	MDRV_NAME("Gladiator")
	MDRV_HWDESC("Z80, M6809, YM2203, MSM-5205(x2)")
	MDRV_INIT( GLD_Init )
	MDRV_SEND( GLD_SendCmd )
	MDRV_DELAYS(1000, 150)
	
	MDRV_CPU_ADD(Z80C, 12000000/4)
	MDRV_CPU_MEMORY(glad_sound_readmem,glad_sound_writemem)
	MDRV_CPU_PORTS(glad_sound_readport,glad_sound_writeport)

//	MDRV_CPU_ADD(M6809B, 12000000/16)
//	MDRV_CPU_MEMORY(glad_readmem_pcm, glad_writemem_pcm)

	MDRV_SOUND_ADD(YM2203, &gym2203_interface)
	MDRV_SOUND_ADD(MSM5205, &msm5205_interface)
M1_BOARD_END
