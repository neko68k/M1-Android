/*
	Seibu/Taito games with Z80 (Toshiba T5182 module) and YM-2151
	- Panic Road
	- The Lost Castle in Dark Mist
	- Mustache Boy
*/

#include "m1snd.h"

enum
{
	VECTOR_INIT,
	YM2151_ASSERT,
	YM2151_CLEAR,
	YM2151_ACK,
	CPU_ASSERT,
	CPU_CLEAR
};

static int irqstate;

static void setirq_callback(int param)
{
	switch (param)
	{
		case YM2151_ASSERT:
			irqstate |= 1|4;
			break;

		case YM2151_CLEAR:
			irqstate &= ~1;
			break;

		case YM2151_ACK:
			irqstate &= ~4;
			break;

		case CPU_ASSERT:
			irqstate |= 2;	// also used by t5182_sharedram_semaphore_main_r
			break;

		case CPU_CLEAR:
			irqstate &= ~2;
			break;
	}

	if (irqstate == 0)	/* no IRQs pending */
		cpu_set_irq_line(0 ,0,CLEAR_LINE);
	else	/* IRQ pending */
		cpu_set_irq_line(0, 0,ASSERT_LINE);
}



static WRITE_HANDLER( t5182_sound_irq_w )
{
	setirq_callback(CPU_ASSERT);
}

static WRITE_HANDLER( t5182_ym2151_irq_ack_w )
{
	setirq_callback(YM2151_ACK);
}

static WRITE_HANDLER( t5182_cpu_irq_ack_w )
{
	setirq_callback(CPU_CLEAR);
}

static void t5182_ym2151_irq_handler(int irq)
{
	if (irq)
		setirq_callback(YM2151_ASSERT);
	else
		setirq_callback(YM2151_CLEAR);
}



static int semaphore_main, semaphore_snd;

READ_HANDLER(t5182_sharedram_semaphore_snd_r)
{
	return semaphore_snd;
}

WRITE_HANDLER(t5182_sharedram_semaphore_main_acquire_w)
{
	semaphore_main = 1;
}

WRITE_HANDLER(t5182_sharedram_semaphore_main_release_w)
{
	semaphore_main = 0;
}

static WRITE_HANDLER(t5182_sharedram_semaphore_snd_acquire_w)
{
	semaphore_snd = 1;
}

static WRITE_HANDLER(t5182_sharedram_semaphore_snd_release_w)
{
	semaphore_snd = 0;
}

static READ_HANDLER(t5182_sharedram_semaphore_main_r)
{
	return semaphore_main | (irqstate & 2);
}


static READ_HANDLER( ram_r )
{
	return workram[offset];
}

static WRITE_HANDLER( ram_w )
{
	workram[offset] = data;
}

static READ_HANDLER( coin_r )
{
	return 0;
}


// 4000-407F    RAM shared with main CPU
// 4000 output queue length
// 4001-4020 output queue
// answers:
//  80XX finished playing sound XX
//  A0XX short contact on coin slot XX (coin error)
//  A1XX inserted coin in slot XX
// 4021 input queue length
// 4022-4041 input queue
// commands:
//  80XX play sound XX
//  81XX stop sound XX
//  82XX stop all voices associated with timer A/B/both where XX = 01/02/03
//  84XX play sound XX if it isn't already playing
//  90XX reset
//  A0XX
// rest unused
static MEMORY_READ_START( panicr_snd_readmem )
	{ 0x0000, 0x1f9e, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4000, 0x40ff, ram_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( panicr_snd_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x40ff, ram_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


// 00  W YM2151 address
// 01 RW YM2151 data
// 10  W semaphore for shared RAM: set as in use
// 11  W semaphore for shared RAM: set as not in use
// 12  W clear IRQ from YM2151
// 13  W clear IRQ from main CPU
// 20 R  flags bit 0 = main CPU is accessing shared RAM????  bit 1 = main CPU generated IRQ
// 30 R  coin inputs (bits 0 and 1, active high)
// 40  W external ROM banking? (the only 0 bit enables a ROM)
// 50  W test mode status flags (bit 0 = ROM test fail, bit 1 = RAM test fail, bit 2 = YM2151 IRQ not received)
static PORT_READ_START( panicr_snd_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x20, 0x20, t5182_sharedram_semaphore_main_r },
	{ 0x30, 0x30, coin_r },
PORT_END

static PORT_WRITE_START( panicr_snd_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x10, 0x10, t5182_sharedram_semaphore_snd_acquire_w },
	{ 0x11, 0x11, t5182_sharedram_semaphore_snd_release_w },
	{ 0x12, 0x12, t5182_ym2151_irq_ack_w },
	{ 0x13, 0x13, t5182_cpu_irq_ack_w },
PORT_END


static struct YM2151interface ym2151_interface =
{
	1,
	14318180 / 4,
	{ YM3012_VOL(40, MIXER_PAN_LEFT, 40, MIXER_PAN_RIGHT) },
	{ t5182_ym2151_irq_handler }
};



static void PanicRoad_Init(long foo)
{
	/* darkmist */
	if (Machine->refcon == 1)
	{
		int i;

		for (i = 0x8000; i < 0x10000; ++i)
			prgrom[i] = BITSWAP8(prgrom[i], 7, 1, 2, 3, 4, 5, 6, 0);
	}
}


static void PanicRoad_SendCmd(int cmda, int cmdb)
{
	/* Stop */
	if (cmda == 0xff)
	{
		workram[0x21] = 0x1;
		workram[0x22] = 0x90;
	}
	else
	{
		workram[0x21] = 0x1;
		workram[0x22] = 0x80;
		workram[0x23] = cmda;
	}

	t5182_sound_irq_w(0, 0);
}


M1_BOARD_START( panicr )
	MDRV_NAME("Panic Road")
	MDRV_HWDESC("Z80, YM-2151")
	MDRV_INIT( PanicRoad_Init )
	MDRV_SEND( PanicRoad_SendCmd )

	MDRV_CPU_ADD(Z80C, 14318180 / 4)
	MDRV_CPU_MEMORY(panicr_snd_readmem, panicr_snd_writemem)
	MDRV_CPU_PORTS(panicr_snd_readport, panicr_snd_writeport)
	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
M1_BOARD_END
