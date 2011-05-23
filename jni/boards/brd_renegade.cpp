/* Technos Renegade */

#include "m1snd.h"

static void CC_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
	return cmd_latch;
}

static unsigned int BS_Read(unsigned int address)
{
	if (address >= 0x8000)
	{
		return prgrom[address];
	}

	return memory_read8(address);
}

static void BS_Write(unsigned int address, unsigned int data)
{					
	memory_write8(address, data);
}

static M16809T m6809rwmem =
{
	BS_Read,
	BS_Write,
};

static WRITE_HANDLER( adpcm_play_w )
{
	int offs;
	int len;

	offs = (data - 0x2c) * 0x2000;
	len = 0x2000*2;

	/* kludge to avoid reading past end of ROM */
	if (offs + len > 0x20000)
		len = 0x1000;

	if (offs >= 0 && offs+len <= 0x20000)
		ADPCM_play(0,offs,len);
	else
		logerror((char *)"out of range adpcm command: 0x%02x\n",data);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, latch_r },
	{ 0x2801, 0x2801, YM3526_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1800, 0x1800, MWA_NOP }, // this gets written the same values as 0x2000
	{ 0x2000, 0x2000, adpcm_play_w },
	{ 0x2800, 0x2800, YM3526_control_port_0_w },
	{ 0x2801, 0x2801, YM3526_write_port_0_w },
	{ 0x3000, 0x3000, MWA_NOP }, /* adpcm related? stereo pan? */
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static void irqhandler(int linestate)
{
	if (linestate)
	 	cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	else
	 	cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
}

static struct YM3526interface ym3526_interface = {
	1,			/* 1 chip (no more supported) */
	3000000,	/* 3 MHz ? (hand tuned) */
	{ 100 }, 	/* volume */
	{ irqhandler },
};

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	8000,		/* 8000Hz playback */
	REGION_SOUND1,	/* memory region */
	{ 100 } 	/* volume */
};

M1_BOARD_START( renegade )
	MDRV_NAME("Renegade")
	MDRV_HWDESC("M6809, YM3526, ADPCM")
	MDRV_SEND( CC_SendCmd )

	MDRV_CPU_ADD(M6809, 1500000)
	MDRV_CPUMEMHAND(&m6809rwmem)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_SOUND_ADD(YM3526, &ym3526_interface)
	MDRV_SOUND_ADD(ADPCM, &adpcm_interface)
M1_BOARD_END

static void CC_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
 	cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
}
