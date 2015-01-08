/* Video System YM2610 boards */
/* Z80 + YMF286K or YM2610 */

#include "m1snd.h"

static void Blaz_Init(long srate);
static void PDrm_Init(long srate);
static void Blaz_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static WRITE_HANDLER( blzntrnd_sh_bankswitch_w )
{
	unsigned char *RAM = memory_region(RGN_CPU1);
	int bankaddress;

	bankaddress = (data & 0x07) * 0x4000;
	cpu_setbank(1, &RAM[bankaddress]);
}

static WRITE_HANDLER( f1_sh_bankswitch_w )
{
	unsigned char *RAM = memory_region(RGN_CPU1);
	int bankaddress;

	bankaddress = (data & 0x01) * 0x8000;
	bankaddress += 0x8000;
	cpu_setbank(1, &RAM[bankaddress]);
}

static WRITE_HANDLER( suprslam_sh_bankswitch_w )
{
	unsigned char *RAM = memory_region(RGN_CPU1);
	int bankaddress;

	bankaddress = (data & 0x03) * 0x8000;
	cpu_setbank(1, &RAM[bankaddress]);
}

static WRITE_HANDLER( pipedrm_bankswitch_w )
{
	UINT8 *ram = memory_region(REGION_CPU1);
	cpu_setbank(1, &ram[0x10000 + (data & 0x01) * 0x8000]);
}

static void blzntrnd_irqhandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface blzntrnd_ym2610_interface =
{
	1,
	8000000,	/* 8 MHz??? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ blzntrnd_irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( blzntrnd_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( blzntrnd_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( blzntrnd_sound_readport )
	{ 0x40, 0x40, latch_r },
	{ 0x80, 0x80, YM2610_status_port_0_A_r },
	{ 0x82, 0x82, YM2610_status_port_0_B_r },
PORT_END

static PORT_WRITE_START( blzntrnd_sound_writeport )
	{ 0x00, 0x00, blzntrnd_sh_bankswitch_w },
	{ 0x40, 0x40, IOWP_NOP },
	{ 0x80, 0x80, YM2610_control_port_0_A_w },
	{ 0x81, 0x81, YM2610_data_port_0_A_w },
	{ 0x82, 0x82, YM2610_control_port_0_B_w },
	{ 0x83, 0x83, YM2610_data_port_0_B_w },
PORT_END

static MEMORY_READ_START( sw_sound_readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( sw_sound_writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static PORT_WRITE_START( tao_sound_writeport )
	{ 0x00, 0x00, YM2610_control_port_0_A_w },
	{ 0x01, 0x01, YM2610_data_port_0_A_w },
	{ 0x02, 0x02, YM2610_control_port_0_B_w },
	{ 0x03, 0x03, YM2610_data_port_0_B_w },
	{ 0x04, 0x04, suprslam_sh_bankswitch_w },
	{ 0x08, 0x08, IOWP_NOP },
PORT_END

static PORT_READ_START( sw_sound_readport )
	{ 0x00, 0x00, YM2610_status_port_0_A_r },
	{ 0x02, 0x02, YM2610_status_port_0_B_r },
	{ 0x0c, 0x0c, latch_r },
PORT_END

static PORT_WRITE_START( sw_sound_writeport )
	{ 0x00, 0x00, YM2610_control_port_0_A_w },
	{ 0x01, 0x01, YM2610_data_port_0_A_w },
	{ 0x02, 0x02, YM2610_control_port_0_B_w },
	{ 0x03, 0x03, YM2610_data_port_0_B_w },
	{ 0x04, 0x04, suprslam_sh_bankswitch_w },
	{ 0x08, 0x08, IOWP_NOP },
PORT_END

static PORT_READ_START( f1_sound_readport )
	{ 0x14, 0x14, latch_r },
	{ 0x18, 0x18, YM2610_status_port_0_A_r },
	{ 0x1a, 0x1a, YM2610_status_port_0_B_r },
PORT_END

static PORT_WRITE_START( f1_sound_writeport )
	{ 0x00, 0x00, f1_sh_bankswitch_w },
	{ 0x0c, 0x0c, IOWP_NOP },
	{ 0x14, 0x14, IOWP_NOP },
	{ 0x18, 0x18, YM2610_control_port_0_A_w },
	{ 0x19, 0x19, YM2610_data_port_0_A_w },
	{ 0x1a, 0x1a, YM2610_control_port_0_B_w },
	{ 0x1b, 0x1b, YM2610_data_port_0_B_w },
PORT_END

static PORT_READ_START( sslam_sound_readport )
	{ 0x04, 0x04, latch_r },
	{ 0x08, 0x08, YM2610_status_port_0_A_r },
	{ 0x09, 0x09, YM2610_read_port_0_r },
	{ 0x0a, 0x0a, YM2610_status_port_0_B_r },
	{ 0x10, 0x10, latch_r },
PORT_END

static PORT_WRITE_START( sslam_sound_writeport )
	{ 0x00, 0x00, suprslam_sh_bankswitch_w },
	{ 0x04, 0x04, IOWP_NOP },
	{ 0x08, 0x08, YM2610_control_port_0_A_w },
	{ 0x09, 0x09, YM2610_data_port_0_A_w },
	{ 0x0a, 0x0a, YM2610_control_port_0_B_w },
	{ 0x0b, 0x0b, YM2610_data_port_0_B_w },
	{ 0x18, 0x18, IOWP_NOP },
PORT_END

static PORT_READ_START( pipedrm_readport )
	{ 0x16, 0x16, latch_r },
	{ 0x18, 0x18, YM2610_status_port_0_A_r },
	{ 0x1a, 0x1a, YM2610_status_port_0_B_r },
PORT_END


static PORT_WRITE_START( pipedrm_writeport )
	{ 0x04, 0x04, pipedrm_bankswitch_w },
	{ 0x17, 0x17, IOWP_NOP },
	{ 0x18, 0x18, YM2610_control_port_0_A_w },
	{ 0x19, 0x19, YM2610_data_port_0_A_w },
	{ 0x1a, 0x1a, YM2610_control_port_0_B_w },
	{ 0x1b, 0x1b, YM2610_data_port_0_B_w },
PORT_END

M1_BOARD_START( blzntrnd )
	MDRV_NAME( "Blazing Tornado" )
	MDRV_HWDESC( "Z80, YM2610" )
	MDRV_DELAYS( 300, 100 )
	MDRV_INIT( Blaz_Init )
	MDRV_SEND( Blaz_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(blzntrnd_sound_readmem, blzntrnd_sound_writemem)
	MDRV_CPU_PORTS(blzntrnd_sound_readport, blzntrnd_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &blzntrnd_ym2610_interface)
M1_BOARD_END

M1_BOARD_START( sonicwings )
	MDRV_NAME( "Sonic Wings" )
	MDRV_HWDESC( "Z80, YM2610" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( Blaz_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(sw_sound_readmem, sw_sound_writemem)
	MDRV_CPU_PORTS(sw_sound_readport, sw_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &blzntrnd_ym2610_interface)
M1_BOARD_END

M1_BOARD_START( taotaido )
	MDRV_NAME( "Tao Taido" )
	MDRV_HWDESC( "Z80, YM2610" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( Blaz_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(sw_sound_readmem, sw_sound_writemem)
	MDRV_CPU_PORTS(sw_sound_readport, tao_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &blzntrnd_ym2610_interface)
M1_BOARD_END

static void F1_Init(long srate)
{
	f1_sh_bankswitch_w(0, 0);
}

M1_BOARD_START( f1gp )
	MDRV_NAME( "F-1 Grand Prix" )
	MDRV_HWDESC( "Z80, YM2610" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( F1_Init )
	MDRV_SEND( Blaz_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(sw_sound_readmem, sw_sound_writemem)
	MDRV_CPU_PORTS(f1_sound_readport, f1_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &blzntrnd_ym2610_interface)
M1_BOARD_END

M1_BOARD_START( superslam )
	MDRV_NAME( "Super Slams" )
	MDRV_HWDESC( "Z80, YM2610" )
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( Blaz_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(sw_sound_readmem, sw_sound_writemem)
	MDRV_CPU_PORTS(sslam_sound_readport, sslam_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &blzntrnd_ym2610_interface)
M1_BOARD_END

M1_BOARD_START( pipedrm )
	MDRV_NAME( "Pipe Dream" )
	MDRV_HWDESC( "Z80, YM2610" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( PDrm_Init )
	MDRV_SEND( Blaz_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(sw_sound_readmem, sw_sound_writemem)
	MDRV_CPU_PORTS(pipedrm_readport, pipedrm_writeport)

	MDRV_SOUND_ADD(YM2610, &blzntrnd_ym2610_interface)
M1_BOARD_END

static READ8_HANDLER( from_acknmi )
{
	return 0xff;
}

static READ8_HANDLER( fromlatchaux_r )
{
	return cmd_latch;
}

static MEMORY_READ_START( from_sound_readmem )
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( from_sound_writemem )
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( from_sound_readport )
	{ 0x00, 0x00, latch_r },
	{ 0x04, 0x04, fromlatchaux_r },
	{ 0x08, 0x08, YM2610_status_port_0_A_r },
	{ 0x0a, 0x0a, YM2610_status_port_0_B_r },
	{ 0x0c, 0x0c, from_acknmi },
PORT_END

static PORT_WRITE_START( from_sound_writeport )
	{ 0x08, 0x08, YM2610_control_port_0_A_w },
	{ 0x09, 0x09, YM2610_data_port_0_A_w },
	{ 0x0a, 0x0a, YM2610_control_port_0_B_w },
	{ 0x0b, 0x0b, YM2610_data_port_0_B_w },
PORT_END

M1_BOARD_START( fromance )
	MDRV_NAME( "Final Romance" )
	MDRV_HWDESC( "Z80, YM2610" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( PDrm_Init )
	MDRV_SEND( Blaz_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(from_sound_readmem, from_sound_writemem)
	MDRV_CPU_PORTS(from_sound_readport, from_sound_writeport)

	MDRV_SOUND_ADD(YM2610, &blzntrnd_ym2610_interface)
M1_BOARD_END

static void PDrm_Init(long srate)
{
	pipedrm_bankswitch_w(0, 0);	
}

static void Blaz_Init(long srate)
{
	cmd_latch = 0;
	m1snd_setCmdPrefix(0xff);
}

static void Blaz_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}
