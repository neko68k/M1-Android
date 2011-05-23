// BattleToads: Z80 + BSMT2000
// Song data offset info from MooglyGuy.

#include "m1snd.h"

#define TIMER_RATE (1.0/183.0)

static void BT_Init(long srate);
static void BT_SendCmd(int cmda, int cmdb);

static int cmd_latch, sound_int_state, data_ready, bytes_left;
static unsigned char *dlp, sequence[0x8000];
static UINT8 main_to_sound_ready;

static int songdata[9*2] =
{
	0x9b5, 0x7708f8,
	0x1464, 0x7712b4,
	0x76c, 0x77271a,
	0x100d, 0x772e88,
	0x11d1, 0x773e9c,
	0x1072, 0x775070,
	0x1072, 0x775070,
	0x1072, 0x775070,
	0x1072, 0x775070,
};

static void BT_WriteLatch(int cmda)
{
	cmd_latch = cmda;
	main_to_sound_ready = 1;
	cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
}

static struct BSMT2000interface bsmt2000_interface =
{
	1,
	{ 24000000 },
	{ 12 },
	{ RGN_SAMP1 },
	{ 100 }
};

// download sequencer
static void _timer(int refcon)
{
	if (!bytes_left) return;
//	printf("pushing %x\n", *dlp);
	BT_WriteLatch(*dlp);
	bytes_left--;
	dlp++;
}

static READ_HANDLER( sound_data_r )
{
	int ret = cmd_latch;

	main_to_sound_ready = 0;
	cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);

	_timer(0);

//	printf("RL: 0x%x,\n", ret);
	return ret;
}

static READ_HANDLER( bsmt_ready_r )
{
	return 0x80;
}

static READ_HANDLER( sound_ready_to_send_r )
{
	return 0x80;
}

static WRITE_HANDLER( bsmt2000_port_w )
{
	UINT16 reg = offset >> 8;
	UINT16 val = ((offset & 0xff) << 8) | data;
	BSMT2000_data_0_w(reg, val, 0);
}

static WRITE_HANDLER( sound_int_state_w )
{
	if (!(sound_int_state & 0x80) && (data & 0x80))
		cpu_set_irq_line(0, 0, CLEAR_LINE);

	sound_int_state = data;
}

static READ_HANDLER( sound_data_ready_r )
{
	return main_to_sound_ready ? 0x00 : 0x80;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_RAM },
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x8000, 0x8000, sound_data_r },
	{ 0x8004, 0x8004, sound_data_ready_r },
	{ 0x8005, 0x8005, sound_ready_to_send_r },
	{ 0x8006, 0x8006, bsmt_ready_r },
MEMORY_END

static PORT_WRITE_START( sound_writeport )
	{ 0x0000, 0x7fff, bsmt2000_port_w },
	{ 0x8000, 0x8000, IOWP_NOP },
	{ 0x8002, 0x8002, sound_int_state_w },
MEMORY_END

M1_BOARD_START( btoads )
	MDRV_NAME("BattleToads")
	MDRV_HWDESC("Z80, BSMT2000")
	MDRV_DELAYS(250, 250)
	MDRV_INIT( BT_Init )
	MDRV_SEND( BT_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_SOUND_ADD(BSMT2000, &bsmt2000_interface)
M1_BOARD_END

static void bt_timer(int refcon)
{
	if (sound_int_state & 0x80)
	{
		cpu_set_irq_line(0, 0, ASSERT_LINE);
		sound_int_state = 0x00;
	}

	timer_set(TIMER_RATE, refcon, bt_timer);
}

static void BT_Init(long srate)
{
	unsigned char *ROM = memory_region(RGN_CPU2);
	unsigned char *s1, *s2, *d, *od;
	int i;

	s1 = ROM;
	s2 = ROM + 0x400000;
	od = (unsigned char *)malloc(0x800000);
	d = od;

	for (i = 0; i < 0x400000/2; i++)
	{
		*d++ = *s1++;
		*d++ = *s1++;
		*d++ = *s2++;
		*d++ = *s2++;
	}

	memcpy(ROM, od, 0x800000);

	timer_set(TIMER_RATE, 0xbeef, bt_timer);

	sound_int_state = 0x80;
	data_ready = 0;

	bytes_left = 8;
	sequence[0] = 0x52;
	sequence[1] = 0x61;
	sequence[2] = 0x72;
	sequence[3] = 0x65;
	sequence[4] = 0x4;
	sequence[5] = 0x5;
	sequence[6] = 0xa;
	sequence[7] = 0x12;
}

static void BT_SendCmd(int cmda, int cmdb)
{
	unsigned char *ROM = memory_region(RGN_CPU2);
	int songptr, length;

	if (cmda == 0)
	{
		cpu_set_reset_line(0, ASSERT_LINE);
		cpu_set_reset_line(0, CLEAR_LINE);
		return;
	}

	if (cmda < 10)
	{
		songptr = songdata[((cmda-1)*2)+1];
		length = songdata[((cmda-1)*2)]; 

		bytes_left = length + 4 + 8 + 8;
		sequence[0] = 0x52;
		sequence[1] = 0x61;
		sequence[2] = 0x72;
		sequence[3] = 0x65;
		sequence[4] = 0x4;
		sequence[5] = 0x5;
		sequence[6] = 0xa;
		sequence[7] = 0x12;
		sequence[8] = 0xd;
		sequence[9] = 0x0;
		memcpy(&sequence[10], &ROM[songptr], length + 2);
		sequence[10+length+2] = 1;
		sequence[10+length+3] = 0;
		sequence[10+length+4] = 3;
		sequence[10+length+5] = 0;
		sequence[10+length+6] = 1;
		sequence[10+length+7] = 1;
		sequence[10+length+8] = 1;
		sequence[10+length+9] = 1;
		dlp = sequence;
		_timer(0);

		if (cmda > 6)
		{
			sequence[10+length+10] = 1;
			switch (cmda)
			{
				case 7:
					sequence[10+length+11] = 7;
					break;
				case 8:
					sequence[10+length+11] = 0x10;
					break;
				case 9:
					sequence[10+length+11] = 0xb;
					break;
			}

			bytes_left += 2;
		}
	}
}
