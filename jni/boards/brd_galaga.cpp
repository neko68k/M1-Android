/* Namco Galaga / Dig Dug / Xevious */

#include "m1snd.h"

#define Z80_CLOCK (3125000)
#define YM_CLOCK (3579580)

#define Gal_TIMER_PERIOD (1.0/120.0)

WRITE_HANDLER( pengo_sound_w );
extern unsigned char *pengo_soundregs;

static int irqmask = 0, bInit0 = 0;

static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	100,		/* playback volume */
	RGN_SAMP1	/* memory region */
};

static READ_HANDLER( Gal_Read )
{
	if (offset <= 0x1fff)
	{
		return prgrom[offset];
	}

	if (offset >= 0x8000 && offset <= 0x9fff)
	{
//		if (address >= 0x9a80 && address <= 0x9aac && workram[address] != 0)
//			printf("read %x at %x (PC=%x)\n", workram[address], address, z80c_get_reg(REG_PC));

//		printf("read @ %x (PC=%x)\n", offset, z80c_get_reg(REG_PC));

		if (Machine->refcon == 1)	// dig dug
		{
			if ((!bInit0) && (offset == 0x8a01))
			{
				workram[0x8a01] = 0;
				bInit0 = 1;
			}
		}

		return workram[offset];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static WRITE_HANDLER( Gal_Write )
{		
	if (offset >= 0x6800 && offset <= 0x681f)
	{
		pengo_sound_w(offset-0x6800, data);
		return;
	}

	if (offset == 0x6822)
	{
//		printf("NMI ack/lock: %x (PC=%x)\n", data, z80_get_reg(REG_PC));
		if (data == 1)
		{
			cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
		}
		irqmask = (data & 1) ^ 0x01;
		return;
	}

	if (offset >= 0x8000 && offset <= 0x9fff)
	{
		workram[offset] = data;
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void gal_timer(int refcon)
{
//	printf("------------------------VBL\n");
	if (irqmask)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, ASSERT_LINE);
	}

	// set up for next time
	timer_set(Gal_TIMER_PERIOD, 0, gal_timer);
}

static void Gal_Init(long srate)
{
	pengo_soundregs = &workram[0x6800];

	bInit0 = 0;

	timer_set(Gal_TIMER_PERIOD, 0, gal_timer);

	m1snd_addToCmdQueue(0xffff);	// prime the pump
}

// see digdug CPU #0 routine at 0x3C15
static void Gal_SendCmd(int cmda, int cmdb)
{
	int i;

	pengo_sound_enable_w(0, 1);
	irqmask = 1;

	if (Machine->refcon == 2)	// galaga
	{
		if (cmda != 0xffff)
		{
			for (i = 0x9aa0; i < 0x9aaa; i++)
			{
				workram[i] = 0;
			}

			workram[0x9aa0 + cmda] = 1;
		}
		else
		{
			for (i = 0x9aa0; i < 0x9aaa; i++)
			{
				workram[i] = 0;
			}
		}
	}
	else if (Machine->refcon == 1)	// dig dug
	{
		if (cmda != 0xffff)
		{
			for (i = 0x9a80; i < 0x9a80+0x2c; i++)
			{
				workram[i] = 0;
			}

			workram[0x9a80 + cmda] = 1;
		}
		else
		{
			for (i = 0x9a80; i < 0x9a80+0x2c; i++)
			{
				workram[i] = 0;
			}
		}
	}
	else if (Machine->refcon == 3)	// xevious
	{
		if (cmda != 0xffff)
		{
			for (i = 0x2800; i < 0x2800+0x10; i++)
			{
				workram[i] = 0;
			}

			workram[0x2800 + cmda] = 1;
		}
		else
		{
			for (i = 0x2800; i < 0x2800+0x10; i++)
			{
				workram[i] = 0;
			}
		}
	}
	else if (Machine->refcon == 4)	// bosconian
	{
		if (cmda != 0xffff)
		{
			for (i = 0x1208; i < 0x1216; i++)
			{
				workram[i] = 0;
			}

			workram[0x1208 + cmda] = 1;
		}
		else
		{
			for (i = 0x1208; i < 0x1216; i++)
			{
				workram[i] = 0;
			}
		}
	}
}

static MEMORY_READ_START( readmem_cpu3 )
	{ 0x0000, 0xffff, Gal_Read },
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu3 )
	{ 0x0000, 0xffff, Gal_Write },
MEMORY_END

M1_BOARD_START( galaga )
	MDRV_NAME("Galaga")
	MDRV_HWDESC("Z80, Namco WSG")
	MDRV_INIT( Gal_Init )
	MDRV_SEND( Gal_SendCmd )
	MDRV_DELAYS( 600, 15 )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(readmem_cpu3,writemem_cpu3)
	
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
M1_BOARD_END

static READ_HANDLER( xevious_sharedram_r )
{
//	printf("RS: [%04x]\n", offset);

	if (offset == 0x1401)
	{
		workram[offset] = 0;
	}

	return workram[offset];
}

static WRITE_HANDLER( xevious_sharedram_w )
{
	workram[offset] = data;
}

static WRITE_HANDLER( xevious_interrupt_enable_3_w )
{
	if (data == 1)
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, CLEAR_LINE);
	}

	irqmask = (data & 1) ^ 0x01;
}

static MEMORY_READ_START( xreadmem_cpu3 )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x7800, 0xcfff, xevious_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( xwritemem_cpu3 )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x6800, 0x681f, pengo_sound_w, &pengo_soundregs },
	{ 0x6822, 0x6822, xevious_interrupt_enable_3_w },
	{ 0x7800, 0xcfff, xevious_sharedram_w },
MEMORY_END

M1_BOARD_START( xevious )
	MDRV_NAME("Xevious")
	MDRV_HWDESC("Z80, Namco WSG")
	MDRV_INIT( Gal_Init )
	MDRV_SEND( Gal_SendCmd )
	MDRV_DELAYS( 600, 15 )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(xreadmem_cpu3,xwritemem_cpu3)
	
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
M1_BOARD_END

static MEMORY_READ_START( breadmem_cpu3 )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x7800, 0x8fff, xevious_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( bwritemem_cpu3 )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x6800, 0x681f, pengo_sound_w, &pengo_soundregs },
	{ 0x6822, 0x6822, xevious_interrupt_enable_3_w },
	{ 0x7800, 0x8fff, xevious_sharedram_w },
MEMORY_END

M1_BOARD_START( bosconian )
	MDRV_NAME("Bosconian")
	MDRV_HWDESC("Z80, Namco WSG")
	MDRV_INIT( Gal_Init )
	MDRV_SEND( Gal_SendCmd )
	MDRV_DELAYS( 600, 15 )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(breadmem_cpu3,bwritemem_cpu3)
	
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
M1_BOARD_END

