/* Konami's Blades of Steel and Double Dribble (6809 + YM2203 + speech) */
/* (and Iron Horse) */

#include "m1snd.h"

static void CC_SendCmd(int cmda, int cmdb);
static void DD_SendCmd(int cmda, int cmdb);
static void IH_SendCmd(int cmda, int cmdb);

static int cmd_latch;
static unsigned char *ddrible_snd_sharedram;

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

static void DD_Write(unsigned int address, unsigned int data)
{	
	// handle inter-CPU comms protocol
	if ((address == 0x0002) && (data == 0x01))
	{
//		printf("Handling comms\n");
		data = 0;
	}
				
	memory_write8(address, data);
}

static M16809T dd6809rwmem =
{
	BS_Read,
	DD_Write,
};

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
	return cmd_latch;
}

static WRITE_HANDLER( bladestl_port_B_w ){
	/* bit 1, 2 unknown */
	UPD7759_set_bank_base(0, ((data & 0x38) >> 3)*0x20000);
}

static WRITE_HANDLER( bladestl_speech_ctrl_w ){
	UPD7759_reset_w(0, data & 1);
	UPD7759_start_w(0, data & 2);
}

static MEMORY_READ_START( bladestl_readmem_sound )
	{ 0x0000, 0x07ff, MRA_RAM },				/* RAM */
	{ 0x1000, 0x1000, YM2203_status_port_0_r },	/* YM2203 */
	{ 0x1001, 0x1001, YM2203_read_port_0_r },	/* YM2203 */
	{ 0x4000, 0x4000, UPD7759_0_busy_r },		/* UPD7759 */
	{ 0x6000, 0x6000, latch_r },			/* soundlatch_r */
	{ 0x8000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END

static MEMORY_WRITE_START( bladestl_writemem_sound )
	{ 0x0000, 0x07ff, MWA_RAM },				/* RAM */
	{ 0x1000, 0x1000, YM2203_control_port_0_w },/* YM2203 */
	{ 0x1001, 0x1001, YM2203_write_port_0_w },	/* YM2203 */
	{ 0x3000, 0x3000, bladestl_speech_ctrl_w },	/* UPD7759 */
	{ 0x5000, 0x5000, MWA_NOP },				/* ??? */
	{ 0x8000, 0xffff, MWA_ROM },				/* ROM */
MEMORY_END

static struct YM2203interface ym2203_interface =
{
	1,						/* 1 chip */
	3579545,				/* 3.579545 MHz? */
	{ YM2203_VOL(80,10) },
	{ 0 },
	{ 0 },
	{ UPD7759_0_port_w },
	{ bladestl_port_B_w }
};

static struct UPD7759_interface upd7759_interface =
{
	1,							/* number of chips */
	{ 25 },					/* volume */
	{ REGION_SOUND1 },					/* memory regions */
	UPD7759_STANDALONE_MODE,
	{ 0 }
};

M1_BOARD_START( bladestl )
	MDRV_NAME("Blades of Steel")
	MDRV_HWDESC("M6809, YM2203, uPD7759")
	MDRV_SEND( CC_SendCmd )

	MDRV_CPU_ADD(M6809, 2000000)
	MDRV_CPUMEMHAND(&m6809rwmem)
 	MDRV_CPU_MEMORY(bladestl_readmem_sound,bladestl_writemem_sound)

	MDRV_SOUND_ADD(UPD7759, &upd7759_interface)
	MDRV_SOUND_ADD(YM2203, &ym2203_interface)
M1_BOARD_END

static MEMORY_READ_START( readmem_cpu2 )
	{ 0x0000, 0x07ff, MRA_RAM },					/* shared RAM with CPU #1 */
	{ 0x1000, 0x1000, YM2203_status_port_0_r },		/* YM2203 */
	{ 0x1001, 0x1001, YM2203_read_port_0_r },		/* YM2203 */
	{ 0x8000, 0xffff, MRA_ROM },					/* ROM */
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu2 )
	{ 0x0000, 0x07ff, MWA_RAM, &ddrible_snd_sharedram  },	/* shared RAM with CPU #1 */
	{ 0x1000, 0x1000, YM2203_control_port_0_w },			/* YM2203 */
	{ 0x1001, 0x1001, YM2203_write_port_0_w },				/* YM2203 */
	{ 0x3000, 0x3000, VLM5030_data_w },						/* Speech data */
	{ 0x3400, 0x3400, MWA_NOP }, 	// ??
	{ 0x3c00, 0x3c00, MWA_NOP },	// ??
	{ 0x8000, 0xffff, MWA_ROM },							/* ROM */
MEMORY_END

static READ_HANDLER( ddrible_vlm5030_busy_r )
{
	return rand(); /* patch */
	if (VLM5030_BSY()) return 1;
	else return 0;
}

static WRITE_HANDLER( ddrible_vlm5030_ctrl_w )
{
	unsigned char *SPEECH_ROM = memory_region(REGION_SOUND1);
	/* b7 : vlm data bus OE   */
	/* b6 : VLM5030-RST       */
	/* b5 : VLM5030-ST        */
	/* b4 : VLM5300-VCU       */
	/* b3 : ROM bank select   */
	VLM5030_RST( data & 0x40 ? 1 : 0 );
	VLM5030_ST(  data & 0x20 ? 1 : 0 );
	VLM5030_VCU( data & 0x10 ? 1 : 0 );
	VLM5030_set_rom(&SPEECH_ROM[data & 0x08 ? 0x10000 : 0]);
	/* b2 : SSG-C rc filter enable */
	/* b1 : SSG-B rc filter enable */
	/* b0 : SSG-A rc filter enable */
//	set_RC_filter(2,1000,2200,1000,data & 0x04 ? 150000 : 0); /* YM2203-SSG-C */
//	set_RC_filter(1,1000,2200,1000,data & 0x02 ? 150000 : 0); /* YM2203-SSG-B */
//	set_RC_filter(0,1000,2200,1000,data & 0x01 ? 150000 : 0); /* YM2203-SSG-A */
}

static struct YM2203interface ddym2203_interface =
{
	1,			/* 1 chip */
	3580000,	/* 3.58 MHz */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ ddrible_vlm5030_busy_r },
	{ ddrible_vlm5030_ctrl_w },
	{ 0 }
};

static struct VLM5030interface ddvlm5030_interface =
{
	3580000,    /* 3.58 MHz */
	100,         /* volume */
	REGION_SOUND1,/* memory region of speech rom */
	0x10000     /* memory size 64Kbyte * 2 bank */
};

M1_BOARD_START( ddribble )
	MDRV_NAME("Double Dribble")
	MDRV_HWDESC("M6809, YM2203, VLM5030")
	MDRV_SEND( DD_SendCmd )
	MDRV_DELAYS( 2500, 30 )

	MDRV_CPU_ADD(M6809, 1536000)
	MDRV_CPUMEMHAND(&dd6809rwmem)
	MDRV_CPU_MEMORY(readmem_cpu2,writemem_cpu2)

	MDRV_SOUND_ADD(YM2203, &ddym2203_interface)
	MDRV_SOUND_ADD(VLM5030, &ddvlm5030_interface)
M1_BOARD_END

static void CC_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
 	cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
}

static void DD_SendCmd(int cmda, int cmdb)
{
	if (cmda < 0x100)
	{
 		ddrible_snd_sharedram[0] = cmda;
		ddrible_snd_sharedram[1] = 1;
	}
	else
	{
 		ddrible_snd_sharedram[3] = cmda & 0xff;
		ddrible_snd_sharedram[2] = 1;
	}
}


/*
	Iron Horse
*/

static READ_HANDLER( ihlatch_r )
{
 	cpu_set_irq_line(0, 0, CLEAR_LINE);
	return cmd_latch;
}

static MEMORY_READ_START( ih_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8000, 0x8000, ihlatch_r },
MEMORY_END

static MEMORY_WRITE_START( ih_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( ih_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
MEMORY_END

static PORT_WRITE_START( ih_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
MEMORY_END


static struct YM2203interface ihym2203_interface =
{
	1,
	18432000 / 6,
	{ YM2203_VOL(25, 25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

M1_BOARD_START( ironhors )
	MDRV_NAME("Iron Horse")
	MDRV_HWDESC("Z80, YM2203")
	MDRV_SEND( IH_SendCmd )

	MDRV_CPU_ADD(Z80C, 18432000/6)
	MDRV_CPU_MEMORY(ih_readmem, ih_writemem)
	MDRV_CPU_PORTS(ih_readport, ih_writeport)

	MDRV_SOUND_ADD(YM2203, &ihym2203_interface)
M1_BOARD_END

static void IH_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	z80_set_irqvec(Z80_RST_38);
 	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

