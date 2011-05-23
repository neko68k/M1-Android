/* Darius: 2xZ80, 2xYM2203, MSM-5205 */

#include "m1snd.h"
#include "taitosnd.h"

#define DAR_Z80_CLOCK (4000000)	// both z80s at the same clock
#define DAR_YM_CLOCK (4000000)
#define MSM_CLOCK (384000)

static void DAR_Init(long srate);
static void DAR_SendCmd(int cmda, int cmdb);
static void YM_IRQHandler(int irq);
static void darius_adpcm_int(int irq);

static int adpcm_latch, nmi_enable;

static WRITE_HANDLER( darius_write_portA0 )
{
	// volume control FM #0 PSG #0 A
	//usrintf_showmessage(" pan %02x %02x %02x %02x %02x", darius_pan[0], darius_pan[1], darius_pan[2], darius_pan[3], darius_pan[4] );
	//usrintf_showmessage(" A0 %02x A1 %02x B0 %02x B1 %02x", port[0], port[1], port[2], port[3] );
//	darius_vol[0] = darius_def_vol[(data>>4)&0x0f];
//	darius_vol[6] = darius_def_vol[(data>>0)&0x0f];
//	update_fm0();
//	update_psg0( 0 );
}

static WRITE_HANDLER( darius_write_portA1 )
{
	// volume control FM #1 PSG #1 A
	//usrintf_showmessage(" pan %02x %02x %02x %02x %02x", darius_pan[0], darius_pan[1], darius_pan[2], darius_pan[3], darius_pan[4] );
//	darius_vol[3] = darius_def_vol[(data>>4)&0x0f];
//	darius_vol[7] = darius_def_vol[(data>>0)&0x0f];
//	update_fm1();
//	update_psg1( 0 );
}

static WRITE_HANDLER( darius_write_portB0 )
{
	// volume control PSG #0 B/C
	//usrintf_showmessage(" pan %02x %02x %02x %02x %02x", darius_pan[0], darius_pan[1], darius_pan[2], darius_pan[3], darius_pan[4] );
//	darius_vol[1] = darius_def_vol[(data>>4)&0x0f];
//	darius_vol[2] = darius_def_vol[(data>>0)&0x0f];
//	update_psg0( 1 );
//	update_psg0( 2 );
}

static WRITE_HANDLER( darius_write_portB1 )
{
	// volume control PSG #1 B/C
	//usrintf_showmessage(" pan %02x %02x %02x %02x %02x", darius_pan[0], darius_pan[1], darius_pan[2], darius_pan[3], darius_pan[4] );
//	darius_vol[4] = darius_def_vol[(data>>4)&0x0f];
//	darius_vol[5] = darius_def_vol[(data>>0)&0x0f];
//	update_psg1( 1 );
//	update_psg1( 2 );
}

static WRITE_HANDLER( darius_fm0_pan )
{
}

static WRITE_HANDLER( darius_fm1_pan )
{
}

static WRITE_HANDLER( darius_psg0_pan )
{
}

static WRITE_HANDLER( darius_psg1_pan )
{
}

static WRITE_HANDLER( darius_da_pan )
{
}

struct MSM5205interface msm5205_interface =
{
	1,				/* 1 chip */
	384000, 			/* 384KHz */
	{ darius_adpcm_int },	/* interrupt function */
	{ MSM5205_S48_4B },	/* 8KHz   */
	{ 50 }			/* volume */
};

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz ??? */
	{ YM2203_VOL(60,20), YM2203_VOL(60,20) },
	{ 0, 0 },		/* portA read */
	{ 0, 0 },
	{ darius_write_portA0, darius_write_portA1 },	/* portA write */
	{ darius_write_portB0, darius_write_portB1 },	/* portB write */
	{ YM_IRQHandler, 0 }
};

static WRITE_HANDLER( sound_bankswitch_w )
{
	int banknum = data &0x03;

//	printf("bankswitch to %x\n", banknum);
	cpu_setbank( 1, memory_region(REGION_CPU1) + (banknum * 0x8000) + 0x10000 );
}

static WRITE_HANDLER( adpcm_command_w )
{
	adpcm_latch = data;
}

static MEMORY_READ_START( darius_sound_readmem )
	{ 0x0000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0x9001, 0x9001, YM2203_read_port_0_r },
	{ 0xa000, 0xa000, YM2203_status_port_1_r },
	{ 0xa001, 0xa001, YM2203_read_port_1_r },
	{ 0xb000, 0xb000, MRA_NOP },
	{ 0xb001, 0xb001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( darius_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM2203_control_port_1_w },
	{ 0xa001, 0xa001, YM2203_write_port_1_w },
	{ 0xb000, 0xb000, taitosound_slave_port_w },
	{ 0xb001, 0xb001, taitosound_slave_comm_w },
	{ 0xc000, 0xc000, darius_fm0_pan },
	{ 0xc400, 0xc400, darius_fm1_pan },
	{ 0xc800, 0xc800, darius_psg0_pan },
	{ 0xcc00, 0xcc00, darius_psg1_pan },
	{ 0xd000, 0xd000, darius_da_pan },
	{ 0xd400, 0xd400, adpcm_command_w },	/* ADPCM command for second Z80 to read from port 0x00 */
//	{ 0xd800, 0xd800, display_value },	/* ??? */
	{ 0xdc00, 0xdc00, sound_bankswitch_w },
MEMORY_END

static READ_HANDLER( adpcm_command_read )
{
	return adpcm_latch;
}

static READ_HANDLER( readport2 )
{
	return 0;
}

static READ_HANDLER( readport3 )
{
	return 0;
}

static WRITE_HANDLER ( adpcm_nmi_disable )
{
	nmi_enable = 0;
}

static WRITE_HANDLER ( adpcm_nmi_enable )
{
	nmi_enable = 1;
}

static WRITE_HANDLER( adpcm_data_w )
{
	MSM5205_data_w (0,   data         );
	MSM5205_reset_w(0, !(data & 0x20) );	/* my best guess, but it could be output enable as well */
}

static PORT_READ_START( darius_sound2_readport )
	{ 0x00, 0x00, adpcm_command_read },
	{ 0x02, 0x02, readport2 },	/* ??? */
	{ 0x03, 0x03, readport3 },	/* ??? */
PORT_END

static PORT_WRITE_START( darius_sound2_writeport )
	{ 0x00, 0x00, adpcm_nmi_disable },
	{ 0x01, 0x01, adpcm_nmi_enable },
	{ 0x02, 0x02, adpcm_data_w },
PORT_END

static MEMORY_READ_START( darius_sound2_readmem )
	{ 0x0000, 0xffff, MRA_ROM },
	/* yes, no RAM */
MEMORY_END

static MEMORY_WRITE_START( darius_sound2_writemem )
	{ 0x0000, 0xffff, MWA_NOP },	/* writes rom whenever interrupt occurs - as no stack */
	/* yes, no RAM */
MEMORY_END

M1_BOARD_START( darius )
	MDRV_NAME("Darius")
	MDRV_HWDESC("Z80(x2), YM2203(x2), MSM-5205")
	MDRV_INIT( DAR_Init )
	MDRV_SEND( DAR_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000/2)	// master
	MDRV_CPU_MEMORY(darius_sound_readmem,darius_sound_writemem)

	MDRV_CPU_ADD(Z80C, 8000000/2)	// ADPCM slave
	MDRV_CPU_MEMORY(darius_sound2_readmem,darius_sound2_writemem)
	MDRV_CPU_PORTS(darius_sound2_readport,darius_sound2_writeport)

	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
	MDRV_SOUND_ADD(MSM5205, &msm5205_interface)
M1_BOARD_END

static void YM_IRQHandler(int irq)
{
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void darius_adpcm_int(int ieq)
{
	if (nmi_enable)
	{
		cpu_set_irq_line( 1, IRQ_LINE_NMI, ASSERT_LINE );
		cpu_set_irq_line( 1, IRQ_LINE_NMI, CLEAR_LINE );
	}
}

static void DAR_Init(long srate)
{
	int i;
	
	prgrom = memory_region(REGION_CPU1);

	timer_setnoturbo();

	// set up the bank image for the main z80
	for( i = 3; i >= 0; i-- )
	{
		memcpy( prgrom + 0x8000*i + 0x10000, prgrom,            0x4000 );
		memcpy( prgrom + 0x8000*i + 0x14000, prgrom + 0x4000*i, 0x4000 );
	}

	cpu_setbank(1, prgrom);

	m1snd_addToCmdQueue(0xffff);
	m1snd_addToCmdQueue(0xeef);
}

static void DAR_SendCmd(int cmda, int cmdb)
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
