/* Atari Star Wars */

#include "m1snd.h"

#define M6809_CLOCK (1500000)
#define POKEY_CLOCK (1500000)

static void SW_Init(long srate);
static void SW_SendCmd(int cmda, int cmdb);

static unsigned int sw_readmem(unsigned int address);
static void sw_writemem(unsigned int address, unsigned int data);

static int port_A = 0;   /* 6532 port A data register */
static int port_B = 0;     /* 6532 port B data register        */
static int irq_flag = 0;   /* 6532 interrupt flag register */

static int port_A_ddr = 0; /* 6532 Data Direction Register A */
static int port_B_ddr = 0; /* 6532 Data Direction Register B */
                           /* for each bit, 0 = input, 1 = output */
static int PA7_irq = 0;  /* IRQ-on-write flag (sound CPU) */

static int sound_data;	/* data for the sound cpu */
static int main_data;   /* data for the main  cpu */

static int enable_5220;

static struct POKEYinterface pokey_interface =
{
	4,			/* 4 chips */
	1500000,	/* 1.5 MHz? */
	{ 20, 20, 20, 20 },	/* volume */
	/* The 8 pot handlers */
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	/* The allpot handler */
	{ 0, 0, 0, 0 },
};


static struct TMS5220interface tms5220_interface =
{
	640000,     /* clock speed (80*samplerate) */
	50,         /* volume */
	0           /* IRQ handler */
};

static M16809T swrw = 
{
	sw_readmem,
	sw_writemem,
};

M1_BOARD_START( starwars )
	MDRV_NAME( "Star Wars" )
	MDRV_HWDESC( "6809, POKEY(x4), TMS5220" )
	MDRV_DELAYS( 1200, 220 )
	MDRV_INIT( SW_Init )
	MDRV_SEND( SW_SendCmd )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&swrw)

	MDRV_SOUND_ADD(POKEY, &pokey_interface)
	MDRV_SOUND_ADD(TMS5220, &tms5220_interface)
M1_BOARD_END

/*************************************
 *
 *	Sound interrupt generation
 *
 *************************************/

static void snd_interrupt(int foo)
{
	irq_flag |= 0x80; /* set timer interrupt flag */
	cpu_set_irq_line(0, M6809_IRQ_LINE, HOLD_LINE);
}



/*************************************
 *
 *	M6532 I/O read
 *
 *************************************/

READ_HANDLER( starwars_m6532_r )
{
	static int temp;

	switch (offset)
	{
		case 0: /* 0x80 - Read Port A */

			/* Note: bit 4 is always set to avoid sound self test */

			return port_A|0x10|(!tms5220_ready_r()<<2);

		case 1: /* 0x81 - Read Port A DDR */
			return port_A_ddr;

		case 2: /* 0x82 - Read Port B */
			return port_B;  /* speech data read? */

		case 3: /* 0x83 - Read Port B DDR */
			return port_B_ddr;

		case 5: /* 0x85 - Read Interrupt Flag Register */
			temp = irq_flag;
			irq_flag = 0;   /* Clear int flags */
//			cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
			return temp;

		default:
			return 0;
	}

	return 0; /* will never execute this */
}



/*************************************
 *
 *	M6532 I/O write
 *
 *************************************/

WRITE_HANDLER( starwars_m6532_w )
{
	switch (offset)
	{
		case 0: /* 0x80 - Port A Write */

			/* Write to speech chip on PA0 falling edge */

			if((port_A&0x01)==1)
			{
				port_A = (port_A&(~port_A_ddr))|(data&port_A_ddr);
				if ((port_A&0x01)==0)
				{
					if (enable_5220)
						tms5220_data_w(0,port_B);
				}
			}
			else
				port_A = (port_A&(~port_A_ddr))|(data&port_A_ddr);

			return;

		case 1: /* 0x81 - Port A DDR Write */
			port_A_ddr = data;
			return;

		case 2: /* 0x82 - Port B Write */
			/* TMS5220 Speech Data on port B */

			/* ignore DDR for now */
			port_B = data;

			return;

		case 3: /* 0x83 - Port B DDR Write */
			port_B_ddr = data;
			return;

		case 7: /* 0x87 - Enable Interrupt on PA7 Transitions */

			/* This feature is emulated now.  When the Main CPU  */
			/* writes to mainwrite, it may send an IRQ to the    */
			/* sound CPU, depending on the state of this flag.   */

			PA7_irq = data;
			return;


		case 0x1f: /* 0x9f - Set Timer to decrement every n*1024 clocks, */
			/*        With IRQ enabled on countdown               */

			/* Should be decrementing every data*1024 6532 clock cycles */
			/* 6532 runs at 1.5 MHz, so there a 3 cylces in 2 usec */

			timer_set (TIME_IN_USEC((1024*2/3)*data), 0, snd_interrupt);
			return;

		default:
			return;
	}

	return; /* will never execute this */

}



/*************************************
 *
 *	Sound CPU to/from main CPU
 *
 *************************************/

READ_HANDLER( starwars_sin_r )
{
	int res;

	port_A &= 0x7f; /* ready to receive new commands from main */
//	printf("read %d from sound latch\n", sound_data);
//	cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
	res = sound_data;
	sound_data = 0;
	return res;
}


WRITE_HANDLER( starwars_sout_w )
{
	port_A |= 0x40; /* result from sound cpu pending */
	main_data = data;
	return;
}



/*************************************
 *
 *	Main CPU to/from source CPU
 *
 *************************************/

WRITE_HANDLER( starwars_main_wr_w )
{
	port_A |= 0x80;  /* command from main cpu pending */
	sound_data = data;
	if (PA7_irq)
	{
//		printf("PA7 IRQ: setting M6809 IRQ\n");
		cpu_set_irq_line(0, M6809_IRQ_LINE, HOLD_LINE);
	}
}

static unsigned int sw_readmem(unsigned int address)
{
	if (address < 0x1000)
	{	
		return starwars_sin_r(address);
	}

	if (address >= 0x1000 && address <= 0x107f)
	{
		return workram[address&0x7f];
	}

	if (address >= 0x1080 && address <= 0x109f)
	{
		return starwars_m6532_r(address-0x1080);
	}

	if (address >= 0x2000 && address <= 0x27ff)
	{
		return workram[address-0x2000 + 0x8000];
	}

	if (address >= 0x4000 && address < 0x6000)
	{
		return prgrom[address-0x4000];
	}

	if (address >= 0x6000 && address < 0x8000)
	{
		return prgrom[address-0x6000+0x4000];
	}

  	if (address >= 0xc000 && address <= 0xdfff)
	{
		return prgrom[address-0xc000+0x2000];
	}

  	if (address >= 0xe000 && address <= 0xffff)
	{
		return prgrom[address-0xe000+0x6000];
	}

//	printf("Unmapped read from %x\n", address);
	return 0;
}

static void sw_writemem(unsigned int address, unsigned int data)
{
	if (address < 0x7ff)
	{
		return;		// ack to main cpu
	}

	if (address >= 0x1000 && address <= 0x107f)
	{
		workram[address&0x7f] = data;
		return;
	}

	if (address >= 0x1080 && address <= 0x109f)
	{
		starwars_m6532_w(address-0x1080, data);
		return;
	}

	if (address >= 0x1800 && address <= 0x183f)
	{
//		printf("POKEY write %x to %x\n", data, address);
		quad_pokey_w(address-0x1800, data);
		return;
	}

	if (address >= 0x2000 && address <= 0x27ff)
	{
		workram[address-0x2000 + 0x8000] = data;
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void SW_Init(long srate)
{
	m1snd_addToCmdQueue(3);
	m1snd_addToCmdQueue(0);

	enable_5220 = 0;
}

static void SW_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xff) return;

	if (cmda == 0) enable_5220 = 1;

	starwars_main_wr_w(0, cmda);
}
