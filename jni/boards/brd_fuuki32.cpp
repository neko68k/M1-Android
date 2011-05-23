/* Fuuki FG-3 system */

#include "m1snd.h"

static data8_t fuuki32_shared_ram[16];

static void irqhandler(int irq)
{
//	printf("YM IRQ %d\n", irq);
	if (irq)
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 0, CLEAR_LINE);

}

static void FG3_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xffff)
	{
		fuuki32_shared_ram[0] = 0xae;
		return;
	}

	if (cmda < 0x100)
	{
		fuuki32_shared_ram[1] = cmda & 0xff;
		fuuki32_shared_ram[0] = 0xa0 | (cmda >> 8);
	}
	else
	{
		fuuki32_shared_ram[0xb] = cmda & 0xff;
		fuuki32_shared_ram[0xa] = 0xa0 | (cmda >> 8);
	}
}

static WRITE_HANDLER ( fuuki32_sound_bw_w )
{
	data8_t *rom = memory_region(REGION_CPU1);

	cpu_setbank(1, rom + (data * 0x8000));
}

int last_read = 0;
static READ_HANDLER( snd_z80_r )
{
	if (fuuki32_shared_ram[offset] != last_read)
	{
		last_read = fuuki32_shared_ram[offset];
	}
	return fuuki32_shared_ram[offset];
}

static WRITE_HANDLER( snd_z80_w )
{
	// boot protocol
	if (offset == 0 && data == 0xcd)
	{
		memset(fuuki32_shared_ram, 0, 16*sizeof(data8_t));
		return;
	}

	fuuki32_shared_ram[offset] = data;	
}

static MEMORY_READ_START( fuuki32_sound_readmem )
	{ 0x0000, 0x5fff, MRA_ROM		},	// ROM
	{ 0x6000, 0x6fff, MRA_RAM		},	// RAM
	{ 0x7ff0, 0x7fff, snd_z80_r  },
	{ 0x8000, 0xffff, MRA_BANK1		},	// ROM
MEMORY_END

static MEMORY_WRITE_START( fuuki32_sound_writemem )
	{ 0x0000, 0x5fff, MWA_ROM		},	// ROM
	{ 0x6000, 0x6fff, MWA_RAM		},	// RAM
	{ 0x7ff0, 0x7fff, snd_z80_w  },
	{ 0x8000, 0xffff, MWA_ROM		},	// ROM
MEMORY_END

static PORT_READ_START( fuuki32_sound_readport )
	{ 0x40, 0x40, YMF262_status_0_r },
PORT_END

static PORT_WRITE_START( fuuki32_sound_writeport )
	{ 0x00, 0x00, fuuki32_sound_bw_w },
	{ 0x30, 0x30, MWA_NOP },
	{ 0x40, 0x40, YMF262_register_A_0_w },
	{ 0x41, 0x41, YMF262_data_A_0_w },
	{ 0x42, 0x42, YMF262_register_B_0_w },
	{ 0x43, 0x43, YMF262_data_B_0_w },
	{ 0x44, 0x44, YMF278B_control_port_0_C_w },
	{ 0x45, 0x45, YMF278B_data_port_0_C_w },
PORT_END

static struct YMF278B_interface ymf278b_interface =
{
	1,
	{ YMF278B_STD_CLOCK },
	{ REGION_SOUND1 },
	{ YM3012_VOL(50, MIXER_PAN_LEFT, 50, MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct YMF262interface ymf262_interface =
{
	1,					/* 1 chip */
	14318180,			/* X1 ? */
	{ YAC512_VOL(25,MIXER_PAN_LEFT,25,MIXER_PAN_RIGHT) },	/* channels A and B */
	{ YAC512_VOL(25,MIXER_PAN_LEFT,25,MIXER_PAN_RIGHT) },	/* channels C and D */
	{ irqhandler },		/* irq */
};

static void FG3_Init(long srate)
{
	m1snd_addToCmdQueue(0xffff);
	fuuki32_sound_bw_w(0, 0);
}

M1_BOARD_START( fuukifg3 )
	MDRV_NAME("Fuuki FG-3")
	MDRV_HWDESC("Z80, YMF278B (OPL4)")
	MDRV_DELAYS( 1000, 100 )
	MDRV_INIT( FG3_Init )
	MDRV_SEND( FG3_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY(fuuki32_sound_readmem,fuuki32_sound_writemem)
	MDRV_CPU_PORTS(fuuki32_sound_readport,fuuki32_sound_writeport)

	MDRV_SOUND_ADD(YMF262, &ymf262_interface)
	MDRV_SOUND_ADD(YMF278B, &ymf278b_interface)
M1_BOARD_END

