/* various segapcm based boards */

#define TIMER_RATE (1.0/670.0)

#include "m1snd.h"

#define TOR_Z80_CLOCK (4000000)
#define TOR_YM_CLOCK (4000000)

static void OR_Init(long srate);
static void SP_SendCmd(int cmda, int cmdb);

static int cmd_latch;
static int or_suffix[8] = { 0, 0, 0x3f, 0, 0, 0, 0, -1 };

struct YM2151interface ym2151_interface = {
	1,			/* 1 chip */
	4000000,
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};

struct SEGAPCMinterface segapcm_interface_15k = {
	SEGAPCM_SAMPLE15K,
	BANK_256,
	RGN_SAMP1,		// memory region
	70
};
struct SEGAPCMinterface segapcm_interface_15k_512 = {
	SEGAPCM_SAMPLE15K,
	BANK_512,
	RGN_SAMP1,		// memory region
	70
};
struct SEGAPCMinterface segapcm_interface_15k_gforce = {
	SEGAPCM_SAMPLE15K,
	BANK_12M|BANK_MASKF8,
	RGN_SAMP1,		// memory region
	70
};
struct SEGAPCMinterface segapcm_interface_32k = {
	SEGAPCM_SAMPLE32K,
	BANK_256,
	RGN_SAMP1,
	70
};

static int er_suffix[] = { 0, 0, 0, 0, 0, -1 };

static void ER_Init(long srate)
{
	m1snd_setCmdSuffixStr(er_suffix);
}

static void ER_StopCmd(int cmda)
{
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0x81);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
}

static void OR_Init(long srate)
{
	m1snd_setCmdSuffixStr(or_suffix);
}

static void SP_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void SMGP_SendCmd(int cmda, int cmdb)
{
	workram[0x10] = cmda;
	workram[0x1e] = 0x80;
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	return cmd_latch;
}

static READ_HANDLER( workram_r )
{
	return workram[offset];
}

static WRITE_HANDLER( workram_w )
{
	workram[offset] = data;
}

static MEMORY_READ_START( snd_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, SegaPCM_r },
	{ 0xf800, 0xffff, workram_r },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, SegaPCM_w },
	{ 0xf800, 0xffff, workram_w },
MEMORY_END

static MEMORY_READ_START( snd_readmem2 )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_r },
	{ 0xf100, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( snd_writemem2 )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_w },
	{ 0xf100, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( snd_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x40, 0x40, latch_r },
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
PORT_END

M1_BOARD_START( erver1 )
	MDRV_NAME( "Enduro Racer" )
	MDRV_HWDESC( "Z80, YM2151, Sega PCM (BANK_256)" )
	MDRV_DELAYS( 60, 5 )
	MDRV_INIT( ER_Init )
	MDRV_SEND( SP_SendCmd )
	MDRV_STOP( ER_StopCmd )

	MDRV_CPU_ADD(Z80C, TOR_Z80_CLOCK)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, &segapcm_interface_15k)
M1_BOARD_END

M1_BOARD_START( segashang )
	MDRV_NAME( "Super Hang-On" )
	MDRV_HWDESC( "Z80, YM2151, Sega PCM (BANK_512)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( OR_Init )
	MDRV_SEND( SP_SendCmd )

	MDRV_CPU_ADD(Z80C, TOR_Z80_CLOCK)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, &segapcm_interface_15k_512)
M1_BOARD_END

M1_BOARD_START( segaorun )
	MDRV_NAME( "OutRun" )
	MDRV_HWDESC( "Z80, YM2151, Sega PCM (BANK_256)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( OR_Init )
	MDRV_SEND( SP_SendCmd )

	MDRV_CPU_ADD(Z80C, TOR_Z80_CLOCK)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, &segapcm_interface_15k)
M1_BOARD_END

static int tr_suffix[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 };

static void TOR_Stop(int cmda)
{
	if (Machine->refcon == 1)
	{
		// don't add anything for racing hero
	}
	else
	{
		m1snd_addToCmdQueueRaw(games[curgame].stopcmd);
	}
}

static int rachero_prefix[] = { 0xff, 0x00, -1 };

static void TR_Init(long srate)
{
	if (Machine->refcon == 1)
	{
		// Racing Hero: prefix is ff 00, no stop command
		m1snd_setCmdPrefixStr(rachero_prefix);
	}
	else
	{
		m1snd_setCmdSuffixStr(tr_suffix);
	}
}

M1_BOARD_START( segatoutrun )
	MDRV_NAME( "Turbo OutRun + X Board" )
	MDRV_HWDESC( "Z80, YM2151, Sega PCM (BANK_512)" )
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( TR_Init )
	MDRV_SEND( SP_SendCmd )
	MDRV_STOP( TOR_Stop )

	MDRV_CPU_ADD(Z80C, TOR_Z80_CLOCK)
	MDRV_CPU_MEMORY(snd_readmem2,snd_writemem2)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, &segapcm_interface_15k_512)
M1_BOARD_END

M1_BOARD_START( segagforce )
	MDRV_NAME( "Y Board" )
	MDRV_HWDESC( "Z80, YM2151, Sega PCM (BANK_12M|BANK_MASKF8)" )
	MDRV_DELAYS( 60, 55 )
	MDRV_SEND( SP_SendCmd )

	MDRV_CPU_ADD(Z80C, TOR_Z80_CLOCK)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, &segapcm_interface_15k_gforce)
M1_BOARD_END

static void smgp_timer(int refcon)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);

	timer_set(TIMER_RATE, 0, smgp_timer);
}

static void SMGP_Init(long srate)
{
	timer_set(TIMER_RATE, 0, smgp_timer);
}

M1_BOARD_START( segasmgp )
	MDRV_NAME( "Super Monaco GP" )
	MDRV_HWDESC( "Z80, YM2151, Sega PCM (BANK_512)" )
	MDRV_DELAYS( 60, 55 )
	MDRV_INIT( SMGP_Init )
	MDRV_SEND( SMGP_SendCmd )

	MDRV_CPU_ADD(Z80C, TOR_Z80_CLOCK)
	MDRV_CPU_MEMORY(snd_readmem,snd_writemem)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, &segapcm_interface_15k_512)
M1_BOARD_END

