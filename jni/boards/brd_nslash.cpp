/* Night Slashers */
/* Z80 + YM2151 + MSM6295(x2) */

#include "m1snd.h"

static void NSL_SendCmd(int cmda, int cmdb);

static int cmd_latch;
static int nslasher_sound_irq;

static READ_HANDLER( latch_r )
{
	nslasher_sound_irq &= ~0x02;
	cpu_set_irq_line(0, 0, (nslasher_sound_irq != 0) ? ASSERT_LINE : CLEAR_LINE);
	return cmd_latch;
}

static void sound_irq(int irq)
{
	if (irq)
		nslasher_sound_irq |= 0x01;
	else
		nslasher_sound_irq &= ~0x01;
	cpu_set_irq_line(0, 0, (nslasher_sound_irq != 0) ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	OKIM6295_set_bank_base(0, ((data >> 0)& 1) * 0x40000);
	OKIM6295_set_bank_base(1, ((data >> 1)& 1) * 0x40000);
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ RGN_SAMP1, RGN_SAMP2 },
	{ 80, 10 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },			/* ROM F6 */
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa001, 0xa001, YM2151_status_port_0_r },
	{ 0xb000, 0xb000, OKIM6295_status_0_r },
	{ 0xc000, 0xc000, OKIM6295_status_1_r },
	{ 0xd000, 0xd000, latch_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, YM2151_register_port_0_w },
	{ 0xa001, 0xa001, YM2151_data_port_0_w },
	{ 0xb000, 0xb000, OKIM6295_data_0_w },
	{ 0xc000, 0xc000, OKIM6295_data_1_w },
MEMORY_END

static READ8_HANDLER( ns_read_rom )
{
	return prgrom[offset];
}

static PORT_READ_START( readio_nslash )
	{ 0x0000, 0xffff, ns_read_rom },
PORT_END

static PORT_WRITE_START( writeio_nslash )
PORT_END

M1_BOARD_START( nslash )
	MDRV_NAME( "Night Slashers" )
	MDRV_HWDESC( "Z80, YM2151, MSM-6295(x2)" )
	MDRV_SEND( NSL_SendCmd )

	MDRV_CPU_ADD(Z80C, 32220000/9)
	MDRV_CPU_MEMORY(readmem_sound, writemem_sound)
	MDRV_CPU_PORTS(readio_nslash, writeio_nslash)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static void NSL_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	nslasher_sound_irq |= 0x02;
	cpu_set_irq_line(0, 0, (nslasher_sound_irq != 0) ? ASSERT_LINE : CLEAR_LINE);
}
