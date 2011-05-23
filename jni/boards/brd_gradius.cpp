/* Konami GX400 board */

#include "m1snd.h"

#define Z80_CLOCK (8000000)
#define AY_CLOCK (14318180/8)
#define K005289_CLOCK (3579545/2)

#define VBL_RATE (1.0/60.0)
#define TIMER_RATE (1.0/3500.0)

static void GX400_Init(long srate);
static void GX400Bubble_Init(long srate);
static void GX400_SendCmd(int cmda, int cmdb);

static READ_HANDLER(gx400_portA_r);
static WRITE_HANDLER(gx400_5289_control_A);
static WRITE_HANDLER(gx400_5289_control_B);

static int cmd_latch, isbubble, bInitComp, bBoot;
static unsigned char *pData;

static int gx400_portA, tick;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static unsigned char *gx400_ram;

static struct AY8910interface ay8910_interface =
{
	2,      		/* 2 chips */
	14318180/8,     /* 1.78975 MHz */
	{ 20, 15 },
	{ gx400_portA_r, 0 },
	{ 0, 0 },
	{ 0, gx400_5289_control_A },
	{ 0, gx400_5289_control_B }
};

static struct k005289_interface k005289_interface =
{
	3579545/2,	/* clock speed */
	8,		/* playback volume */
	RGN_SAMP1	/* prom memory region */
};

static struct VLM5030interface gx400_vlm5030_interface =
{
    3579545,       /* master clock  */
    40,            /* volume        */
    0,             /* memory region (RAM based) */
    0x0800         /* memory length (not sure if correct) */
};

WRITE_HANDLER( gx400_speech_start_w )
{
        /* the voice data is not in a rom but in sound RAM at $8000 */
        VLM5030_set_rom (gx400_ram + 0x4000);
        VLM5030_ST (1);
        VLM5030_ST (0);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x87ff, MRA_RAM },
	{ 0xe001, 0xe001, latch_r },
	{ 0xe086, 0xe086, AY8910_read_port_0_r },
	{ 0xe205, 0xe205, AY8910_read_port_1_r },
	{ 0xfffe, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x87ff, MWA_RAM, &gx400_ram },
	{ 0xa000, 0xafff, k005289_pitch_A_w },
	{ 0xc000, 0xcfff, k005289_pitch_B_w },
	{ 0xe000, 0xe000, VLM5030_data_w },
	{ 0xe002, 0xe002, MWA_NOP },
	{ 0xe003, 0xe003, k005289_keylatch_A_w },
	{ 0xe004, 0xe004, k005289_keylatch_B_w },
	{ 0xe005, 0xe005, AY8910_control_port_1_w },
	{ 0xe006, 0xe006, AY8910_control_port_0_w },
	{ 0xe007, 0xe007, MWA_NOP },
	{ 0xe030, 0xe030, gx400_speech_start_w },
	{ 0xe040, 0xe0ff, MWA_NOP },
	{ 0xe106, 0xe106, AY8910_write_port_0_w },
	{ 0xe405, 0xe405, AY8910_write_port_1_w },
	{ 0xfffe, 0xffff, MWA_RAM },
MEMORY_END

M1_BOARD_START( gx400 )
	MDRV_NAME("GX400 (ROM type)")
	MDRV_HWDESC("Z80, AY-3-8910(x2), K005289, VLM5030")
	MDRV_DELAYS( 60, 15 )
	MDRV_INIT( GX400_Init )
	MDRV_SEND( GX400_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
	MDRV_SOUND_ADD(K005289, &k005289_interface)
	MDRV_SOUND_ADD(VLM5030, &gx400_vlm5030_interface)
M1_BOARD_END

M1_BOARD_START( gx400_bubble )
	MDRV_NAME("GX400 (bubble memory type)")
	MDRV_HWDESC("Z80, AY-3-8910(x2), K005289, VLM5030")
	MDRV_DELAYS( 500, 15 )
	MDRV_INIT( GX400Bubble_Init )
	MDRV_SEND( GX400_SendCmd )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(AY8910, &ay8910_interface)
	MDRV_SOUND_ADD(K005289, &k005289_interface)
	MDRV_SOUND_ADD(VLM5030, &gx400_vlm5030_interface)
M1_BOARD_END

static READ_HANDLER(gx400_portA_r)
{
	int res = gx400_portA;

/*
   bit 0-3:   timer
   bit 4 6:   unused (always high)
   bit 5:     vlm5030 busy
   bit 7:     must be clear for bubble memory version to boot properly
*/

//	printf("reading portA\n");

	bInitComp = 1;

	if (VLM5030_BSY())
		res |= 0x20;

	return res;
}

static WRITE_HANDLER(gx400_5289_control_A)
{
	k005289_control_A_w(0, data);
}

static WRITE_HANDLER(gx400_5289_control_B)
{
	k005289_control_B_w(0, data);
}

static void vbl_timer(int refcon)
{
	if (isbubble)
	{
		if (bInitComp && !bBoot)
		{
			bBoot = 1;
			memcpy(gx400_ram, pData, (0x7e00 - 0x4000));
			gx400_ram[0] = 1;	// indicate xfer complete
		}
		else
		{
			timer_set(VBL_RATE, 0, vbl_timer);
		}
	}
}

static void gx400_timer(int refcon)
{
	tick++;
	tick &= 0x2f;
	gx400_portA &= 0xd0;
	gx400_portA |= tick;

	timer_set(TIMER_RATE, 0, gx400_timer);
}

static void GX400_Init(long srate)
{
	gx400_portA = 0x50;	// bits 4, 6, and 7 are always high

	timer_set(VBL_RATE, 0, vbl_timer);
	timer_set(TIMER_RATE, 0, gx400_timer);

	isbubble = 0;
}

static void GX400Bubble_Init(long srate)
{
	gx400_portA = 0x50;	// bits 4, 6, and 7 are always high

	timer_set(VBL_RATE, 0, vbl_timer);
	timer_set(TIMER_RATE, 0, gx400_timer);

	pData = rom_getregion(RGN_CPU2);
	bInitComp=0;
	bBoot=0;

	switch (Machine->refcon)
	{
		case 1: //GAME_GRADIUS:
			pData += 0x14500;
			break;

		case 2: //GAME_TWINBEE:
			pData += 0x20000;
			break;

		case 3: //GAME_GALACTICWARRIORS:
			pData += 0x21800;
			break;

		case 4: //GAME_BUBBLESYSTEM:
			bBoot = 1;	// nothing to download
			break;
	}
	isbubble = 1;
}

static void GX400_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}
