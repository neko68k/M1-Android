/* Tatsumi Round Up 5, Cycle Warriors and Big Fight */
#include "m1snd.h"

static void YM2151_IRQ(int irq);
static void Tatsumi_SendCmd(int cmda, int cmdb);

static struct YM2151interface ym2151_interface =
{
	1,
	16000000/4,
	{ YM3012_VOL(65, MIXER_PAN_LEFT, 65, MIXER_PAN_RIGHT) },
	{ YM2151_IRQ }
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 2000000/132 },
	{ RGN_SAMP1 },
	{ 60 }
};

/*
	"Current YM2151 doesn't seem to raise the busy flag quickly enough for the
	self-test in Tatsumi games.  Needs fixed, but hack it here for now."
*/
READ8_HANDLER(tatsumi_hack_ym2151_r)
{
	int PC = z80_get_reg(REG_PC);

	if (Machine->refcon == 0)
	{
		/* Round Up 5 */
		if (PC == 0x29fe)
			return 0x80;
	}
	else if (Machine->refcon == 1)
	{
		/* Cycle Warriors */
		if (PC == 0x2aca)
			return 0x80;
	}
	else if (Machine->refcon == 2)
	{
		/* Big Fight */
		if (PC == 0x1c65)
			return 0x80;
	}

	return YM2151_status_port_0_r(0);
}

READ8_HANDLER(tatsumi_hack_oki_r)
{
	int PC = z80_get_reg(REG_PC);

	if (Machine->refcon == 0)
	{
		/* Round Up 5 */
		if (PC == 0x2acc)
			return 0xf;
		else if ((PC == 0x2a9b) || (PC == 0x2adc))
			return 0;
	}
	else if (Machine->refcon == 1)
	{
		/* Cycle Warriors */
		if ( (PC == 0x2b70) || (PC == 0x2bb5) )
			return 0xf;
		else if (PC == 0x2ba3)
			return 0;
	}
	else if (Machine->refcon == 2)
	{
		/* Big Fight */
		if (PC == 0x1c79)
			return 0xf;
		else if (PC == 0x1cac)
			return 0;
	}

	return OKIM6295_status_0_r(0);
}

static READ_HANDLER( ram_r )
{
	return workram[offset];
}

static WRITE_HANDLER( ram_w )
{
	workram[offset] = data;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffef, MRA_RAM },
	{ 0xe000, 0xffef, ram_r },
	{ 0xfff1, 0xfff1, tatsumi_hack_ym2151_r },
	{ 0xfff4, 0xfff4, tatsumi_hack_oki_r },
	{ 0xfff8, 0xfff8, MRA_NOP },
	{ 0xfff9, 0xfff9, MRA_NOP },
	{ 0xfffc, 0xfffc, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffef, ram_w },
	{ 0xfff0, 0xfff0, YM2151_register_port_0_w },
	{ 0xfff1, 0xfff1, YM2151_data_port_0_w },
	{ 0xfff4, 0xfff4, OKIM6295_data_0_w },
	{ 0xfff9, 0xfff9, MWA_NOP },
	{ 0xfffa, 0xfffa, MWA_NOP },
MEMORY_END


M1_BOARD_START( roundup5 )
	MDRV_NAME("Round Up 5")
	MDRV_HWDESC("Z80, YM2151, MSM6295")
	MDRV_SEND( Tatsumi_SendCmd )
	MDRV_DELAYS( 500, 15 )
	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END


static void YM2151_IRQ(int irq)
{
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static void Tatsumi_SendCmd(int cmda, int cmdb)
{	
	if (Machine->refcon == 0)
	{
		/* Round Up 5 */
		workram[0xf58c - 0xe000] = cmda;
	}
	else if (Machine->refcon == 1)
	{
		/* Cycle Warriors */
		workram[0xf2b6 - 0xe000] = cmda;
	}	
	else if (Machine->refcon == 2)
	{
		/* Big Fight */
		OKIM6295_set_frequency(0, 2000000 / 2 / 132);
		workram[0xf2b6 - 0xe000] = cmda;
	}
}
