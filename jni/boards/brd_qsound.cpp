/* Capcom CPS2/ZN QSound board */

// NOTE: both the timer frequency and z80 clock are total guesses
//       an OST would be nice to get the tempo right, but it's close :)

#include "m1snd.h"

#define Z80_CLOCK (8000000)

static void QS_Init(long srate);
static void QSCPS2_Init(long srate);
static void QS_SendCmd(int cmda, int cmdb);
static void QS_StopCmd(int cmda);
static void QSCPS2_SendCmd(int cmda, int cmdb);

static int kabuki_offset;

static unsigned char qs_latch;
static unsigned int enabler_adr[2], enabler_data[2];

static unsigned char *qsound_sharedram1;

// Kabuki decryption routines
void wof_decode(void);
void dino_decode(void);
void punisher_decode(void);
void slammast_decode(void);

struct QSound_interface qsound_interface =
{
	QSOUND_CLOCK,
	RGN_SAMP1,
	{ 100,100 }
};

static READ_HANDLER( qs_readop )
{
	return prgrom[offset + kabuki_offset];
}

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
//	printf("%x from latch\n", qs_latch);
	return qs_latch;
}

int lastbnk = -1;

static WRITE_HANDLER( qsound_banksw_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress= (0x8000 + (data & 0xf) * 0x4000);
	if (bankaddress >= memory_region_length(REGION_CPU1))
	{
		bankaddress=0;
	}

	if (data != lastbnk)
	{
//		printf("%x to bank => %x\n", data, bankaddress);
		lastbnk = data;
	}
	cpu_setbank(1, &RAM[bankaddress]);
}

static READ_HANDLER( Read_Shared )
{
	return qsound_sharedram1[offset];
}

static WRITE_HANDLER( Write_Shared )
{
	if (offset == enabler_adr[0])
	{
		qsound_sharedram1[enabler_adr[0]] = enabler_data[0];
		if (enabler_adr[1] != 0)
		{
			qsound_sharedram1[enabler_adr[1]] = enabler_data[1];
		}
		return;
	}

	qsound_sharedram1[offset] = data;
}

MEMORY_READ_START( qsound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },  /* banked (contains music data) */
	{ 0xc000, 0xcfff, Read_Shared },
	{ 0xd007, 0xd007, qsound_status_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

MEMORY_READ_START( qsound_readop )
	{ 0x0000, 0x7fff, qs_readop },
	{ 0x8000, 0xbfff, MRA_BANK1 },  /* banked (contains music data) */
	{ 0xc000, 0xcfff, Read_Shared },
MEMORY_END

MEMORY_WRITE_START( qsound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, Write_Shared, &qsound_sharedram1 },
	{ 0xd000, 0xd000, qsound_data_h_w },
	{ 0xd001, 0xd001, qsound_data_l_w },
	{ 0xd002, 0xd002, qsound_cmd_w },
	{ 0xd003, 0xd003, qsound_banksw_w },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( qsound_readport )
	{ 0x00, 0x00, latch_r },
PORT_END

static PORT_WRITE_START( qsound_writeport )
PORT_END

M1_BOARD_START( qsoundzn )
	MDRV_NAME("Capcom QSound (ZN version)")
	MDRV_HWDESC("Z80, QSound")
	MDRV_DELAYS(250, 50)
	MDRV_INIT( QS_Init )
	MDRV_SEND( QS_SendCmd )
	MDRV_STOP( QS_StopCmd )
	
	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(qsound_readmem, qsound_writemem)
	MDRV_CPU_PORTS(qsound_readport, qsound_writeport)

	MDRV_SOUND_ADD(QSOUND, &qsound_interface)
M1_BOARD_END

M1_BOARD_START( qsoundcps2 )
	MDRV_NAME("Capcom QSound (CPS version)")
	MDRV_HWDESC("Z80, QSound")
	MDRV_DELAYS(250, 200)
	MDRV_INIT( QSCPS2_Init )
	MDRV_SEND( QSCPS2_SendCmd )

	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY(qsound_readmem,qsound_writemem)
	MDRV_CPU_READOP(qsound_readop)

	MDRV_SOUND_ADD(QSOUND, &qsound_interface)
M1_BOARD_END

void qs_timer(int refcon)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	timer_set(1.0/250.0, 0, qs_timer);
}

static void QS_Init(long srate)
{
	timer_set(1.0/250.0, 0, qs_timer);

	kabuki_offset = 0;

	// set the appropriate music command prefix
	switch (Machine->refcon)
	{
		case 1: //GAME_STRIDER2:
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(5);
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(0);

			m1snd_setCmdPrefix(0);
			break;

		case 2: //GAME_TOSHINDEN2:
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(0xa);
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(0);

			m1snd_setCmdPrefix(0);
			break;

		case 3: //GAME_KIKIAOH:
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(5);
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(0);

			m1snd_setCmdPrefix(0x80);
			break;

		case 4: //GAME_SFEX:
		case 5: //GAME_STARGLADIATOR2:
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(5);

			m1snd_setCmdPrefix(0);
			break;

		case 6: //GAME_RVSCHOOL:
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(5);
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0x3d);
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0x3e);

			m1snd_setCmdPrefix(0);
			break;

		case 7: //GAME_SFEX2P:
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0);

			m1snd_setCmdPrefix(4);
			break;

		case 8: //GAME_TETRISGRANDMASTER:
			m1snd_addToCmdQueue(0x30);	// force STEREO mode
			m1snd_addToCmdQueue(1);

			m1snd_setCmdPrefix(4);
			break;


		case 9: //GAME_SFEX2:
			m1snd_addToCmdQueueRaw(0x30);	// force STEREO mode
			m1snd_addToCmdQueueRaw(1);
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0);

			m1snd_setCmdPrefix(4);
			break;
	}
	

	// disable "enabler" stuff
	enabler_adr[0] = 0;
	enabler_adr[1] = 0;
}

static void QSCPS2_Init(long srate)
{
	timer_set(1.0/250.0, 0, qs_timer);

	kabuki_offset = 0;

	// cps2 has the workram shared with the 68000,
	// so we must pretend to be the 68k here...
	qsound_sharedram1[0] = 0;
	qsound_sharedram1[1] = 0;
	qsound_sharedram1[7] = 0x10;	// panpot center
	qsound_sharedram1[0xf] = 0xff;	// command clear
	qsound_sharedram1[0xffe] = 0x01;	// stereo mode

	// setup the enablers
	enabler_adr[0] = 0xfff;
	enabler_data[0] = 0xff;
	enabler_adr[1] = 0xffd;
	enabler_data[1] = 0x88;

	switch (Machine->refcon)
	{
		case 10: //GAME_SLAMMASTERS:
			slammast_decode();
			kabuki_offset = 0x20000;
			break;

		case 11: //GAME_THEPUNISHER:
			punisher_decode();
			kabuki_offset = 0x20000;
			break;

		case 12: //GAME_CADILLACSANDDINOSAURS:
			dino_decode();
			kabuki_offset = 0x20000;
			break;

		case 13: //GAME_WARRIORSOFFATE:
			wof_decode();
			kabuki_offset = 0x20000;
			break;
	}
}

static void QS_SendCmd(int cmda, int cmdb)
{
	qs_latch = cmda;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static void QS_StopCmd(int cmda)
{
	switch (Machine->refcon)
	{
		case 4: //GAME_SFEX:
		case 1: //GAME_STRIDER2:
		case 3: //GAME_KIKIAOH:
		case 2: //GAME_TOSHINDEN2:
		case 8: //GAME_TETRISGRANDMASTER:
		case 5: //GAME_STARGLADIATOR2:
			m1snd_addToCmdQueueRaw(0xff);
			m1snd_addToCmdQueueRaw(0);
			break;

		case 6: //GAME_RVSCHOOL:
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0x3d);
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0x3e);
			break;

		case 9: //GAME_SFEX2:
		case 7: //GAME_SFEX2P:
			m1snd_addToCmdQueueRaw(0);
			m1snd_addToCmdQueueRaw(0);
			break;

		default:
			m1snd_addToCmdQueue(cmda);
			break;
	}
}

static void QSCPS2_SendCmd(int cmda, int cmdb)
{
	qsound_sharedram1[0] = (cmda>>8)&0xff;
	qsound_sharedram1[1] = cmda&0xff;
	qsound_sharedram1[0xf] = 0;
}

