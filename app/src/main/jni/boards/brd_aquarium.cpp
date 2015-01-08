/* Excellent Systems "Aquarium" */

#include "m1snd.h"

static int cmd_latch;

static void Aqua_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( aquarium_z80_bank_w )
{
	int soundbank = ((data & 0x7) + 1) * 0x8000;
	data8_t *Z80 = (data8_t *)memory_region(REGION_CPU1);

	cpu_setbank(1, &Z80[soundbank]);
}

static UINT8 aquarium_snd_bitswap(UINT8 scrambled_data)
{
	UINT8 data = 0;

	data |= ((scrambled_data & 0x01) << 7);
	data |= ((scrambled_data & 0x02) << 5);
	data |= ((scrambled_data & 0x04) << 3);
	data |= ((scrambled_data & 0x08) << 1);
	data |= ((scrambled_data & 0x10) >> 1);
	data |= ((scrambled_data & 0x20) >> 3);
	data |= ((scrambled_data & 0x40) >> 5);
	data |= ((scrambled_data & 0x80) >> 7);

	return data;
}

static READ_HANDLER( aquarium_oki_r )
{
	return (aquarium_snd_bitswap(OKIM6295_status_0_r(0)) );
}

static WRITE_HANDLER( aquarium_oki_w )
{
	logerror((char *)"Z80-PC:%04x Writing %04x to the OKI M6295\n",activecpu_get_previouspc(),aquarium_snd_bitswap(data));
	OKIM6295_data_0_w( 0, (aquarium_snd_bitswap(data)) );
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
MEMORY_END

static PORT_READ_START( snd_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x02, 0x02, aquarium_oki_r },
	{ 0x04, 0x04, latch_r },
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x02, 0x02, aquarium_oki_w },
	{ 0x06, 0x06, IOWP_NOP },	// command ack?
	{ 0x08, 0x08, aquarium_z80_bank_w },
PORT_END

static void YM_IRQHandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,     		/* 1 chip */
	3579545,	/* Guess */
	{ YM3012_VOL(75,MIXER_PAN_LEFT,75,MIXER_PAN_RIGHT) },
	{ YM_IRQHandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,		/* 1 chip */
	{ 8500 },	/* frequency (Hz) */
	{ RGN_SAMP1 },	/* memory region */
	{ 47 }
};

M1_BOARD_START( aquarium )
	MDRV_NAME("Aquarium")			// hardware name
	MDRV_HWDESC("Z80, YM2151, MSM-6295")	// hardware description
	MDRV_SEND( Aqua_SendCmd )		// routine to send a command to the hardware

	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

