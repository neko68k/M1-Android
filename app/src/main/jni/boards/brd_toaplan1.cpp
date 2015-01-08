// Toaplan shmups

#include "m1snd.h"

static void Toa_Init(long srate);
static void Toa_SendCmd(int cmda, int cmdb);

static READ_HANDLER(toa_read);
static READ_HANDLER(toa_readport);
static WRITE_HANDLER(toa_write);
static WRITE_HANDLER(toa_writeport);

static int bInit, ymbase, wakeval;
static unsigned int wakeaddr, ramlimit;

static void irqhandler(int irq)
{
//	printf("3812 IRQ: %d\n", irq);
	cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM3812interface ym3812_interface =
{
	1,
	28000000/8,		/* 3.5MHz (28MHz Oscillator) */
	{ 100 },
	{ irqhandler },
};

static struct YM3812interface sbym3812_interface =
{
	1,
	3000000,		/* 3MHz */
	{ 100 },
	{ irqhandler },
};

static MEMORY_READ_START( toa_rd )
	{ 0x0000, 0xffff, toa_read },
MEMORY_END

static MEMORY_WRITE_START( toa_wr )
	{ 0x0000, 0xffff, toa_write },
MEMORY_END

static PORT_READ_START( toa_rdport )
	{ 0x0000, 0xffff, toa_readport },
PORT_END

static PORT_WRITE_START( toa_wrport )
	{ 0x0000, 0xffff, toa_writeport },
PORT_END

M1_BOARD_START( zerowing )
	MDRV_NAME("Toaplan1")
	MDRV_HWDESC("Z80, YM3812")
	MDRV_INIT( Toa_Init )
	MDRV_SEND( Toa_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(toa_rd, toa_wr)
	MDRV_CPU_PORTS(toa_rdport, toa_wrport)
	
	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
M1_BOARD_END

M1_BOARD_START( snowbros )
	MDRV_NAME("Snow Bros.")
	MDRV_HWDESC("Z80, YM3812")
	MDRV_INIT( Toa_Init )
	MDRV_SEND( Toa_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000)
	MDRV_CPU_MEMORY(toa_rd, toa_wr)
	MDRV_CPU_PORTS(toa_rdport, toa_wrport)
	
	MDRV_SOUND_ADD(YM3812, &sbym3812_interface)
M1_BOARD_END

static READ_HANDLER(toa_read)
{
//	printf("read %x\n", offset);
	if (offset <= 0x7fff) return prgrom[offset];

	if ((!bInit) && (offset == wakeaddr))
	{
//		printf("hitting wakeaddr\n");
		workram[wakeaddr] = wakeval;
		bInit = 1;
	}

	if (offset >= 0x8000 && offset <= ramlimit) return workram[offset];
	if (offset == 0xe000 && Machine->refcon == 5) return YM3812_status_port_0_r(0);

 	return 0;
}

static WRITE_HANDLER(toa_write)
{
	if (offset >= 0x8000 && offset <= ramlimit)
	{
		workram[offset] = data;
		return;
	}

	if (Machine->refcon == 5)
	{
		if (offset == 0xe000) YM3812_control_port_0_w(0, data);
		if (offset == 0xe001) YM3812_write_port_0_w(0, data);
	}
}

static READ_HANDLER(toa_readport)
{
	int port = offset & 0xff;
	if (port == ymbase)
		return YM3812_status_port_0_r(0);

	return 0;
}

static WRITE_HANDLER(toa_writeport)
{
	int port = offset & 0xff;

	if (port == ymbase)
		YM3812_control_port_0_w(0, data);
	else if (port == ymbase+1)
		YM3812_write_port_0_w(0, data);
}

static void Toa_Init(long srate)
{
	bInit = 0;
	wakeaddr = 0x8001;
	wakeval  = 0xaa;
	ramlimit = 0xffff;

	switch (Machine->refcon)
	{
		case 0:
			ymbase = 0x60;
			break;

		case 1:
			ymbase = 0xa8;
			break;

		case 2:
		      	ymbase = 0x70;
			break;

		case 3:
			ymbase = 0;
			break;

		case 4:
			ymbase = 2;
			break;

		case 5:
			ymbase = 0;
			ramlimit = 0x87ff;
			wakeval = 0xff;
			break;

		case 6:
			ymbase = 0;
			wakeaddr = 0xc002;
			break;
	}
}

static void Toa_SendCmd(int cmda, int cmdb)
{
	workram[wakeaddr & 0xf000] = cmda;
}
