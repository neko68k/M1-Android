/* Konami Ajax */
/* Z80 + YM2151 + K007232(x2) */

#include "m1snd.h"

static void AJX_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}
#if 0
static READ_HANDLER( latch2_r )
{
	return 1;
}
#endif
static struct YM2151interface ym2151_interface =
{
	1,
	3579545,	/* 3.58 MHz */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ 0 },
};

/*	sound_bank_w:
	Handled by the LS273 Octal +ve edge trigger D-type Flip-flop with Reset at B11:

	Bit	Description
	---	-----------
	7	CONT1 (???) \
	6	CONT2 (???) / One or both bits are set to 1 when you kill a enemy
	5	\
	3	/ 4MBANKH
	4	\
	2	/ 4MBANKL
	1	\
	0	/ 2MBANK
*/

static WRITE_HANDLER( sound_bank_w )
{
	int bank_A, bank_B;

	/* banks # for the 007232 (chip 1) */
	bank_A = ((data >> 1) & 0x01);
	bank_B = ((data >> 0) & 0x01);
	K007232_set_bank(0,bank_A,bank_B);

	/* banks # for the 007232 (chip 2) */
	bank_A = ((data >> 4) & 0x03);
	bank_B = ((data >> 2) & 0x03);
	K007232_set_bank(1,bank_A,bank_B);
}

static void volume_callback0(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static WRITE_HANDLER( k007232_extvol_w )
{
	/* channel A volume (mono) */
	K007232_set_volume(1,0,(data & 0x0f) * 0x11/2,(data & 0x0f) * 0x11/2);
}

static void volume_callback1(int v)
{
	/* channel B volume/pan */
	K007232_set_volume(1,1,(v & 0x0f) * 0x11/2,(v >> 4) * 0x11/2);
}

static struct K007232_interface k007232_interface =
{
	2,			/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1, REGION_SOUND2 },	/* memory regions */
	{ K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER),
		K007232_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },/* volume */
	{ volume_callback0,  volume_callback1 }	/* external port callback */
};

static MEMORY_READ_START( ajax_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },			/* ROM F6 */
	{ 0x8000, 0x87ff, MRA_RAM },			/* RAM 2128SL at D16 */
	{ 0xa000, 0xa00d, K007232_read_port_0_r },	/* 007232 registers (chip 1) */
	{ 0xb000, 0xb00d, K007232_read_port_1_r },	/* 007232 registers (chip 2) */
	{ 0xc001, 0xc001, YM2151_status_port_0_r },	/* YM2151 */
	{ 0xe000, 0xe000, latch_r },			/* soundlatch_r */
MEMORY_END

static MEMORY_WRITE_START( ajax_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },			/* ROM F6 */
	{ 0x8000, 0x87ff, MWA_RAM },			/* RAM 2128SL at D16 */
	{ 0x9000, 0x9000, sound_bank_w },		/* 007232 bankswitch */
	{ 0xa000, 0xa00d, K007232_write_port_0_w },	/* 007232 registers (chip 1) */
	{ 0xb000, 0xb00d, K007232_write_port_1_w },	/* 007232 registers (chip 2) */
	{ 0xb80c, 0xb80c, k007232_extvol_w },	/* extra volume, goes to the 007232 w/ A11 */
	{ 0xc000, 0xc000, YM2151_register_port_0_w },	/* YM2151 */
	{ 0xc001, 0xc001, YM2151_data_port_0_w },	/* YM2151 */
MEMORY_END

M1_BOARD_START( ajax )
	MDRV_NAME( "Ajax" )
	MDRV_HWDESC( "Z80, YM2151, K007232(x2)" )
	MDRV_DELAYS( 60, 20 )
	MDRV_SEND( AJX_SendCmd )

	MDRV_CPU_ADD(Z80C, 3579545)
	MDRV_CPU_MEMORY(ajax_readmem_sound,ajax_writemem_sound)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K007232, &k007232_interface)
M1_BOARD_END

static void AJX_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
