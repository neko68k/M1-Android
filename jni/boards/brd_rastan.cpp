// Rastan (Z80, YM2151, MSM5205)
// Hit The Ice / Violence Fight (Z80, YM2203, MSM6295)
// Daisenpu (Z80, YM2151)

#include "m1snd.h"
#include "taitosnd.h"

static void Rst_Init(long srate);
static void Rst_SendCmd(int cmda, int cmdb);

static void irqhandler(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER( rastan_bankswitch_w )
{
	int bankofs = (((data ^1) & 0x01) * 0x4000) + 0x4000;

	cpu_setbank(5, memory_region(RGN_CPU1) + bankofs);
}

static WRITE_HANDLER( bankswitch_w )
{
	int bankofs = (data & 0x3) * 0x4000;
	cpu_setbank(5, memory_region(RGN_CPU1) + bankofs);
}

static WRITE_HANDLER( tsbankswitch_w )
{
	int bankofs = (data & 0x7) * 0x4000;
	cpu_setbank(5, memory_region(RGN_CPU1) + bankofs);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	int bankofs = (data & 3) * 0x4000;
	cpu_setbank(2, memory_region(RGN_CPU1) + bankofs);
}

static struct YM2203interface ym2203_interface =
{
	1,
	3000000,				/* ?? */
	{ YM2203_VOL(80,25) },	/* ?? */
	{ 0 },
	{ 0 },
	{ bankswitch_w },
	{ 0 },
	{ irqhandler }
};

static struct OKIM6295interface okim6295_interface =
{
	2,
	{ 8000,8000 },			/* ?? */
	{ REGION_SOUND1,REGION_SOUND1 }, /* memory regions */
	{ 50,65 }				/* ?? */
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irqhandler },
	{ rastan_bankswitch_w }
};

static struct YM2151interface riym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irqhandler },
	{ bankswitch_w }
};

static struct YM2151interface tsym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
	{ irqhandler },
	{ tsbankswitch_w }
};

static struct YM2151interface daisen_ym2151_interface =
{
	1,	/* 1 chip */
	4000000,	/* 4 MHz ?????? */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ irqhandler },
	{ 0 }
};

static struct ADPCMinterface adpcm_interface =
{
	1,		/* 1 channel */
	8000,		/* 8000Hz playback */
	RGN_SAMP1,	/* memory region */
	{ 50 }		/* volume */
};

/* Game writes here to set ADPCM ROM address */
WRITE_HANDLER( rastan_adpcm_trigger_w )
{
	UINT8 *rom = memory_region(REGION_SOUND1);
	int len = memory_region_length(REGION_SOUND1);
	int start = data << 8;
	int end;

	/* look for end of sample */
	end = (start + 3) & ~3;
	while (end < len && *((UINT32 *)(&rom[end])) != 0x08080808)
		end += 4;

	ADPCM_play(0,start,(end-start)*2);
}

/* Game writes here to START ADPCM_voice playing */
WRITE_HANDLER( rastan_c000_w )
{
}

/* Game writes here to STOP ADPCM_voice playing */
WRITE_HANDLER( rastan_d000_w )
{
}

static MEMORY_READ_START( rastan_s_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK5 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( rastan_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
	{ 0xb000, 0xb000, rastan_adpcm_trigger_w },
	{ 0xb800, 0xb800, MWA_NOP },	// irq ack?
	{ 0xc000, 0xc000, rastan_c000_w },
	{ 0xd000, 0xd000, rastan_d000_w },
MEMORY_END

M1_BOARD_START( rastan )
	MDRV_NAME("Rastan")
	MDRV_HWDESC("Z80, YM2151, MSM-5205")
	MDRV_INIT( Rst_Init )
	MDRV_SEND( Rst_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(rastan_s_readmem,rastan_s_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, &adpcm_interface)
M1_BOARD_END

M1_BOARD_START( topspeed )
	MDRV_NAME("Top Speed")
	MDRV_HWDESC("Z80, YM2151, MSM-5205")
	MDRV_INIT( Rst_Init )
	MDRV_SEND( Rst_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(rastan_s_readmem,rastan_s_writemem)

	MDRV_SOUND_ADD(YM2151, &tsym2151_interface)
	MDRV_SOUND_ADD(ADPCM, &adpcm_interface)
M1_BOARD_END

M1_BOARD_START( rainbow )
	MDRV_NAME("Rainbow Islands")
	MDRV_HWDESC("Z80, YM2151")
	MDRV_INIT( Rst_Init )
	MDRV_SEND( Rst_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(rastan_s_readmem,rastan_s_writemem)

	MDRV_SOUND_ADD(YM2151, &riym2151_interface)
M1_BOARD_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, MRA_NOP },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, MRA_NOP },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
MEMORY_END

M1_BOARD_START( exzisus )
	MDRV_NAME("Exzisus")
	MDRV_HWDESC("Z80, YM2151")
	MDRV_INIT( Rst_Init )
	MDRV_SEND( Rst_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
M1_BOARD_END

static MEMORY_READ_START( daisenpu_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe001, YM2151_status_port_0_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( daisenpu_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END

M1_BOARD_START( daisenpu )
	MDRV_NAME("Daisenpu")
	MDRV_HWDESC("Z80, YM2151")
	MDRV_INIT( Rst_Init )
	MDRV_SEND( Rst_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(daisenpu_sound_readmem,daisenpu_sound_writemem)
	
	MDRV_SOUND_ADD(YM2151, &daisen_ym2151_interface)
M1_BOARD_END

static MEMORY_READ_START( hitice_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK5 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xb000, 0xb000, OKIM6295_status_0_r },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( hitice_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xb000, 0xb001, OKIM6295_data_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
MEMORY_END

M1_BOARD_START( hitice )
	MDRV_NAME("Hit The Ice")
	MDRV_HWDESC("Z80, YM2203, MSM-6295")
	MDRV_INIT( Rst_Init )
	MDRV_SEND( Rst_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(hitice_sound_readmem,hitice_sound_writemem)
	
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

static void Rst_Init(long srate)
{
	m1snd_addToCmdQueue(0xffff);
	m1snd_addToCmdQueue(0xeef);
}

static void Rst_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xffff)
	{
		taitosound_port_w(0, 4);
		taitosound_comm_w(0, 1);
		taitosound_port_w(0, 4);
		taitosound_comm_w(0, 0);
	}
	else
	{
		taitosound_port_w(0, 4);
		taitosound_port_w(0, 0);
		taitosound_comm_w(0, cmda&0xf);
		taitosound_comm_w(0, (cmda&0xf0)>>4);
	}
}

/*
** Jumping (Rainbow Islands Bootleg)
** small patch by uf
*/

static unsigned char jumping_latch=0;

static unsigned char jumpRam[0xfff];

static struct YM2203interface jumping_ym2203_interface =
{
  2,
  3072000,                /* ?? */
  { YM2203_VOL(80,25), YM2203_VOL(80,25) },    /* ?? */
  { 0,0 },
  { 0,0 },
  { 0,0 },
  { 0,0 },
  { 0,0 }
};

static void Jumping_SendCmd(int a, int b) {
  jumping_latch = a & 0xff;
  cpu_set_irq_line(0, 0, HOLD_LINE);
}

static void Jumping_Init (long bleh ) {
  m1snd_addToCmdQueue(0xef);
  memset(jumpRam,0,0xfff);
}

static READ_HANDLER ( jumping_readlatch ) {
  cpu_set_irq_line(0,0,CLEAR_LINE);
  return jumping_latch;
}

static READ_HANDLER ( jumping_readram ) {
  return jumpRam[offset & 0xfff];
}

static WRITE_HANDLER ( jumping_writeram ) {
  jumpRam[offset & 0xfff] = data;
}

static MEMORY_READ_START( jumping_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, jumping_readram },
	{ 0xb000, 0xb001, YM2203_status_port_0_r },
	{ 0xb400, 0xb401, YM2203_status_port_1_r },
	{ 0xb800, 0xb800, jumping_readlatch },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( jumping_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, jumping_writeram },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xb400, 0xb400, YM2203_control_port_1_w },
	{ 0xb401, 0xb401, YM2203_write_port_1_w },
MEMORY_END

M1_BOARD_START( jumping )
	MDRV_NAME("Jumping")
	MDRV_HWDESC("Z80, YM2203(x2)")
	MDRV_INIT( Jumping_Init )
	MDRV_SEND( Jumping_SendCmd )

	MDRV_CPU_ADD(Z80C, 3072000) // XTAL typedefs are very bad for you
	MDRV_CPU_MEMORY(jumping_readmem,jumping_writemem )
	MDRV_SOUND_ADD(YM2203, &jumping_ym2203_interface)
M1_BOARD_END
