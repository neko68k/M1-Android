/* Konami "Twin16" and varients sound */

#include "m1snd.h"

#define Z80_CLOCK (3579545)
#define YM_CLOCK (3579545)

static void TMNT_Init(long srate);
static void MEV_Init(long srate);
static void T16_SendCmd(int cmda, int cmdb);
static void TMNT_Update(long dsppos, long dspframes);

static int cmd_latch, bTmntSamples;

static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static void aliens_volume_callback(int v)
{
	K007232_set_volume(0,0,(v & 0x0f) * 0x11,0);
	K007232_set_volume(0,1,0,(v >> 4) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ RGN_SAMP1 },	/* memory regions */
	{ K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static struct K007232_interface mx5000_k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ RGN_SAMP1 },	/* memory regions */
	{ K007232_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static struct K007232_interface aliens_k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ RGN_SAMP1 },	/* memory regions */
	{ K007232_VOL(15,MIXER_PAN_CENTER,15,MIXER_PAN_CENTER) },	/* volume */
	{ aliens_volume_callback }	/* external port callback */
};

static struct K007232_interface mev_k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ RGN_SAMP1 },	/* memory regions */
	{ K007232_VOL(35,MIXER_PAN_CENTER,35,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
};

static struct YM2151interface mx5000_ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ 0 },
};

static WRITE_HANDLER( aliens_snd_bankswitch_w )
{
	/* b1: bank for chanel A */
	/* b0: bank for chanel B */

	int bank_A = ((data >> 1) & 0x01);
	int bank_B = ((data) & 0x01);

	K007232_set_bank( 0, bank_A, bank_B );
}

static struct YM2151interface aliens_ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
	{ aliens_snd_bankswitch_w }
};

static struct UPD7759_interface upd7759_interface =
{
	1,		/* number of chips */
	{ 60 }, /* volume */
	{ RGN_SAMP2 },		/* memory region */
	UPD7759_STANDALONE_MODE,		/* chip mode */
	{0}
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static int twin16_soundlatch;

READ_HANDLER( twin16_sres_r )
{
	return twin16_soundlatch;
}

WRITE_HANDLER( twin16_sres_w )
{
	/* bit 1 resets the UPD7795C sound chip */
	UPD7759_reset_w(0, data & 2);

	if (Machine->refcon == 1)
	{
		if (data & 0x4)
		{
			samples_play_chan(0);
		}
		else
		{
			samples_stop_chan(0);
		}
	}

	twin16_soundlatch = data;
}

WRITE_HANDLER( mx_sres_w )
{
	twin16_soundlatch = data;
}

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, twin16_sres_r },
	{ 0xa000, 0xa000, latch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
	{ 0xf000, 0xf000, UPD7759_0_busy_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, twin16_sres_w },
	{ 0xb000, 0xb00d, K007232_write_port_0_w  },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xd000, 0xd000, UPD7759_0_port_w },
	{ 0xe000, 0xe000, UPD7759_0_start_w },
MEMORY_END

M1_BOARD_START( konamitwin16 )
	MDRV_NAME("Twin16")
	MDRV_HWDESC("Z80, YM2151, K007232, uPD7759")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( T16_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K007232, &k007232_interface)
	MDRV_SOUND_ADD(UPD7759, &upd7759_interface)
M1_BOARD_END

static MEMORY_READ_START( mx_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, twin16_sres_r },
	{ 0xa000, 0xa000, latch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( mx_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, mx_sres_w },
	{ 0xb000, 0xb00d, K007232_write_port_0_w  },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
MEMORY_END

M1_BOARD_START( mx5000 )
	MDRV_NAME("MX5000")
	MDRV_HWDESC("Z80, YM2151, K007232")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( T16_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(mx_readmem_sound,mx_writemem_sound)

	MDRV_SOUND_ADD(YM2151, &mx5000_ym2151_interface)
	MDRV_SOUND_ADD(K007232, &mx5000_k007232_interface)
M1_BOARD_END

WRITE_HANDLER( mainevt_sh_irqcontrol_w )
{
	UPD7759_reset_w(0, data & 2);
	UPD7759_start_w(0, data & 1);
}

WRITE_HANDLER( mainevt_sh_bankswitch_w )
{
	int bank_A,bank_B;

//logerror("CPU #1 PC: %04x bank switch = %02x\n",activecpu_get_pc(),data);

	/* bits 0-3 select the 007232 banks */
	bank_A=(data&0x3);
	bank_B=((data>>2)&0x3);
	K007232_set_bank( 0, bank_A, bank_B );

	/* bits 4-5 select the UPD7759 bank */
	UPD7759_set_bank_base(0, ((data >> 4) & 0x03) * 0x20000);
}

static MEMORY_READ_START( mev_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa000, latch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xd000, 0xd000, UPD7759_0_busy_r },
MEMORY_END

static MEMORY_WRITE_START( mev_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0x9000, 0x9000, UPD7759_0_port_w },
	{ 0xe000, 0xe000, mainevt_sh_irqcontrol_w },
	{ 0xf000, 0xf000, mainevt_sh_bankswitch_w },
MEMORY_END

M1_BOARD_START( mainevt )
	MDRV_NAME("The Main Event")
	MDRV_HWDESC("Z80, K007232, uPD7759")
	MDRV_DELAYS( 3000, 15 )
	MDRV_INIT( MEV_Init )
	MDRV_SEND( T16_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(mev_sound_readmem, mev_sound_writemem)

	MDRV_SOUND_ADD(K007232, &mev_k007232_interface)
	MDRV_SOUND_ADD(UPD7759, &upd7759_interface)
M1_BOARD_END

WRITE_HANDLER( devstor_sh_irqcontrol_w )
{
}

WRITE_HANDLER( dv_sh_bankswitch_w )
{
	int bank_A,bank_B;

//logerror("CPU #1 PC: %04x bank switch = %02x\n",activecpu_get_pc(),data);

	/* bits 0-3 select the 007232 banks */
	bank_A=(data&0x3);
	bank_B=((data>>2)&0x3);
	K007232_set_bank( 0, bank_A, bank_B );
}

static MEMORY_READ_START( dv_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },	// gangbusters has ram to 87ff, dev only to 83ff
	{ 0xa000, 0xa000, latch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( dv_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xe000, 0xe000, devstor_sh_irqcontrol_w },
	{ 0xf000, 0xf000, dv_sh_bankswitch_w },
MEMORY_END

M1_BOARD_START( devestators )
	MDRV_NAME("Devestators")
	MDRV_HWDESC("Z80, YM2151, K007232")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( T16_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(dv_sound_readmem,dv_sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K007232, &k007232_interface)
M1_BOARD_END

static MEMORY_READ_START( aliens_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },				/* ROM g04_b03.bin */
	{ 0x8000, 0x87ff, MRA_RAM },				/* RAM */
	{ 0xa001, 0xa001, YM2151_status_port_0_r },
	{ 0xc000, 0xc000, latch_r },			/* soundlatch_r */
	{ 0xe000, 0xe00d, K007232_read_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( aliens_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },					/* ROM g04_b03.bin */
	{ 0x8000, 0x87ff, MWA_RAM },					/* RAM */
	{ 0xa000, 0xa000, YM2151_register_port_0_w },
	{ 0xa001, 0xa001, YM2151_data_port_0_w },
	{ 0xe000, 0xe00d, K007232_write_port_0_w },
MEMORY_END

M1_BOARD_START( aliens )
	MDRV_NAME("Aliens")
	MDRV_HWDESC("Z80, YM2151, K007232")
	MDRV_DELAYS( 60, 15 )
	MDRV_SEND( T16_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(aliens_readmem_sound,aliens_writemem_sound)

	MDRV_SOUND_ADD(YM2151, &aliens_ym2151_interface)
	MDRV_SOUND_ADD(K007232, &aliens_k007232_interface)
M1_BOARD_END

M1_BOARD_START( tmnt )
	MDRV_NAME("TMNT")
	MDRV_HWDESC("Z80, YM2151, K007232, uPD7759, samples")
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( TMNT_Init )
	MDRV_UPDATE( TMNT_Update )
	MDRV_SEND( T16_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K007232, &k007232_interface)
	MDRV_SOUND_ADD(UPD7759, &upd7759_interface)
M1_BOARD_END

static struct VLM5030interface vlm5030_interface =
{
    3579545,       /* master clock  */
    60,            /* volume        */
    REGION_SOUND2, /* memory region  */
    0              /* memory length */
};

static READ_HANDLER( wd_r )
{
	static int a=1;
	a^= 1;
	return a;
}

WRITE_HANDLER( salamand_speech_start_w )
{
        VLM5030_ST ( 1 );
        VLM5030_ST ( 0 );
}

static MEMORY_READ_START( sal_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, latch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
	{ 0xe000, 0xe000, wd_r }, /* watchdog?? */
MEMORY_END

static MEMORY_WRITE_START( sal_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xd000, 0xd000, VLM5030_data_w },
	{ 0xf000, 0xf000, salamand_speech_start_w },
MEMORY_END

M1_BOARD_START( salamander )
	MDRV_NAME( "Salamander" )
	MDRV_HWDESC( "Z80, YM2151, K007232, VLM5030" )
	MDRV_DELAYS( 1000, 15 )
	MDRV_SEND( T16_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sal_sound_readmem,sal_sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(K007232, &k007232_interface)
	MDRV_SOUND_ADD(VLM5030, &vlm5030_interface)
M1_BOARD_END

static void mev_timer(int refcon)
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	timer_set(1.0/(60.0*8.0), 0, mev_timer);
}

static void MEV_Init(long srate)
{
	timer_set(4.0, 0, mev_timer);
}

static void TMNT_Init(long srate)
{
	signed short *dest = (signed short *)rom_getregion(RGN_SAMP4);
	unsigned char *source = rom_getregion(RGN_SAMP3);
	int i;

	for (i = 0; i < 0x40000; i++)
	{
		int val = source[2*i] + source[2*i+1] * 256;
		int exp = val >> 13;

	  	val = (val >> 3) & (0x3ff);	/* 10 bit, Max Amplitude 0x400 */
		val -= 0x200;		 	/* Correct DC offset */

		val <<= (exp-3);

		dest[i] = val;
	}

	bTmntSamples = 0;
}

// hack can go away when we add a post-init hook for drivers
static void TMNT_Update(long dsppos, long dspframes)
{
	if (!bTmntSamples)
	{
		samples_init(1, Machine->sample_rate);
		samples_set_info(0, 20000, rom_getregion(RGN_SAMP4), 0x40000);
		bTmntSamples = 1;
	}

	StreamSys_Update(dsppos, dspframes);
}

static void T16_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	if (Machine->refcon == 1 && cmda == 0xff)
	{
		samples_stop_chan(0);
	}

	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

