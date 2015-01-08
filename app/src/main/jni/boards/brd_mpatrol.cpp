/* Irem M62 hardware - Moon Patrol and friends */

#include "m1snd.h"

#define BASE_CLOCK (3579545)
#define M6803_CLOCK (BASE_CLOCK/4)
#define AY_CLOCK (BASE_CLOCK/4)
#define MSM_CLOCK (384000)

static void M62_SendCmd(int cmda, int cmdb);

static unsigned int M62_Read(unsigned int address);
static void M62_Write(unsigned int address, unsigned int data);
static unsigned int M62_ReadPort(unsigned int address);
static void M62_WritePort(unsigned int address, unsigned int data);
static void irem_adpcm_int(int data);
static WRITE_HANDLER( irem_analog_w );
static WRITE_HANDLER( irem_msm5205_w );
static READ_HANDLER( irem_readcmd );

static int cmd_latch;

struct AY8910interface irem_ay8910_interface =
{
	2,	/* 2 chips */
	3579545/4,
	{ 20, 20 },
	{ irem_readcmd, 0 },
	{ 0 },
	{ 0, irem_analog_w },
	{ irem_msm5205_w, 0 }
};

struct MSM5205interface irem_msm5205_interface =
{
	2,					/* 2 chips            */
	384000,				/* 384KHz             */
	{ irem_adpcm_int, 0 },/* interrupt function */
	{ MSM5205_S96_4B,MSM5205_SEX_4B },	/* default to 4KHz, but can be changed at run time */
	{ 100, 100 }
};

static M16800T m62rwmem =
{
	M62_Read,
	M62_Write,
	M62_ReadPort,
	M62_WritePort,
};

static int port1,port2;

static WRITE_HANDLER( irem_port1_w )
{
	port1 = data;
}

static WRITE_HANDLER( irem_port2_w )
{
	/* write latch */
	if ((port2 & 0x01) && !(data & 0x01))
	{
		/* control or data port? */
		if (port2 & 0x04)
		{
			/* PSG 0 or 1? */
			if (port2 & 0x08)
				AY8910_control_port_0_w(0,port1);
			if (port2 & 0x10)
				AY8910_control_port_1_w(0,port1);
		}
		else
		{
			/* PSG 0 or 1? */
			if (port2 & 0x08)
				AY8910_write_port_0_w(0,port1);
			if (port2 & 0x10)
				AY8910_write_port_1_w(0,port1);
		}
	}
	port2 = data;
}


static READ_HANDLER( irem_port1_r )
{
	/* PSG 0 or 1? */
	if (port2 & 0x08)
		return AY8910_read_port_0_r(0);
	if (port2 & 0x10)
		return AY8910_read_port_1_r(0);
	return 0xff;
}

static READ_HANDLER( irem_port2_r )
{
	return 0;
}

static READ_HANDLER( irem_readcmd )
{
 	m6803_set_irq_line(M6803_IRQ_LINE, CLEAR_LINE);
//	printf("Read %x from latch\n", cmd_latch);
	return cmd_latch;
}

static WRITE_HANDLER( irem_msm5205_w )
{
	/* bits 2-4 select MSM5205 clock & 3b/4b playback mode */
	MSM5205_playmode_w(0,(data >> 2) & 7);
	MSM5205_playmode_w(1,((data >> 2) & 4) | 3);	/* always in slave mode */

	/* bits 0 and 1 reset the two chips */
	MSM5205_reset_w(0,data & 1);
	MSM5205_reset_w(1,data & 2);
}

static WRITE_HANDLER( irem_adpcm_w )
{
	MSM5205_data_w(offset,data);
}

static void irem_adpcm_int(int data)
{
	if (data) return;	// only from chip 0 thanks

	m6803_set_irq_line(IRQ_LINE_NMI, ASSERT_LINE);
	m6803_set_irq_line(IRQ_LINE_NMI, CLEAR_LINE);

	/* the first MSM5205 clocks the second */
	MSM5205_vclk_w(1,1);
	MSM5205_vclk_w(1,0);
}

static WRITE_HANDLER( irem_analog_w )
{
}

static unsigned int M62_ReadPort(unsigned int address)
{
	switch (address)
	{
		case M6803_PORT1:
			return irem_port1_r(0);
			break;

		case M6803_PORT2:
			return irem_port2_r(0);
			break;
	}
	return 0;
}

static void M62_WritePort(unsigned int address, unsigned int data)
{
	switch (address)
	{
		case M6803_PORT1:
			irem_port1_w(0, data);
			return;
			break;

		case M6803_PORT2:
			irem_port2_w(0, data);
			return;
			break;
	}
}
 
static unsigned int M62_Read(unsigned int address)
{
//	printf("read at %x\n", address);
	if (address <= 0x1f)
	{
		return m6803_internal_registers_r(address);
	}
	
	if (address >= 0x0080 && address <= 0x00ff)
	{
		return workram[address];
	}

	if (address >= 0x4000)
	{
		return prgrom[address];
	}

	printf("Unmapped read at %x\n", address);
	return 0;
}

static void M62_Write(unsigned int address, unsigned int data)
{					
	if (address <= 0x1f)
	{
		m6803_internal_registers_w(address, data);
		return;
	}

	if (address >= 0x0080 && address <= 0x00ff)
	{
		workram[address] = data;
		return;
	}

	if (address == 0x800)
	{
		return;
	}

	if (address >= 0x801 && address <= 0x802)
	{
		irem_adpcm_w(address-0x801, data);
		return;
	}

	if (address == 0x9000)
	{
		return;
	}

	printf("Unmapped write %x to %x\n", data, address);
}

static void M62_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
 
 	m6803_set_irq_line(M6803_IRQ_LINE, ASSERT_LINE);
}

M1_BOARD_START( m62 )
	MDRV_NAME("Irem M62")
	MDRV_HWDESC("6803, AY-3-8910(x2), MSM-5205(x2)")
	MDRV_DELAYS( 1000, 50 )
	MDRV_SEND( M62_SendCmd )

	MDRV_CPU_ADD(M6803, M6803_CLOCK)
	MDRV_CPUMEMHAND(&m62rwmem)

	MDRV_SOUND_ADD(MSM5205, &irem_msm5205_interface)
	MDRV_SOUND_ADD(AY8910, &irem_ay8910_interface)
M1_BOARD_END

