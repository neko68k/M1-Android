/* Incredible Technologies 32-bit games */

#include "m1snd.h"

#define CLOCK_8MHz (8000000)
#define M6809_CLOCK (4000000)
#define ES5506_CLOCK (16000000)

static void IT32_Init(long srate);
static void IT32B_Init(long srate);
static void IT32_SendCmd(int cmda, int cmdb);

static unsigned int IT32_Read(unsigned int address);
static void IT32_Write(unsigned int address, unsigned int data);

static int cmd_latch, brd_type;

static data8_t via6522[32];
static data16_t via6522_timer_count[2];
static void *via6522_timer[2];
static data8_t via6522_int_state;

static int suffix[2] = { 0, -1 };

static struct ES5506interface es5506_interface =
{
	1,
	{ 16000000 },
	{ RGN_SAMP1 },
	{ RGN_SAMP2 },
	{ RGN_SAMP3 },
	{ RGN_SAMP4 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }
};

static M16809T it32rwmem =
{
	IT32_Read,
	IT32_Write,
};

static int bnk_offset = 0;

/*************************************
 *
 *	Sound 6522 VIA handling
 *
 *************************************/

INLINE void update_via_int(void)
{
	/* if interrupts are enabled and one is pending, set the line */
	if ((via6522[14] & 0x80) && (via6522_int_state & via6522[14]))
	{
		cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	}
	else
	{
		cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
	}
}


static void via6522_timer_callback(int which)
{
	via6522_int_state |= 0x40 >> which;
	update_via_int();
}

static WRITE_HANDLER( via6522_w )
{
	/* update the data */
	via6522[offset] = data;

	/* switch off the offset */
	switch (offset)
	{
		case 0:		/* write to port B */
//			pia_portb_out(0, data);	// bit 4 = ticket counter, 5 = coin counter, 6 = sound LED
/*			if (data & 0x40)
			{
				printf ("LED on\n");
			}
			else
			{
				printf ("LED off\n");
			}*/
			break;

		case 5:		/* write into high order timer 1 */
			via6522_timer_count[0] = (via6522[5] << 8) | via6522[4];
			if (via6522_timer[0])
				timer_remove((TimerT *)via6522_timer[0]);
			via6522_timer[0] = timer_pulse(TIME_IN_HZ(CLOCK_8MHz/4) * (double)via6522_timer_count[0], 0, via6522_timer_callback);

			via6522_int_state &= ~0x40;
			update_via_int();
			break;

		case 13:	/* write interrupt flag register */
			via6522_int_state &= ~data;
			update_via_int();
			break;

		default:	/* log everything else */
			break;
	}

}

static READ_HANDLER( via6522_r )
{
	int result = 0;

	/* switch off the offset */
	switch (offset)
	{
		case 4:		/* read low order timer 1 */
			via6522_int_state &= ~0x40;
			update_via_int();
			break;

		case 13:	/* interrupt flag register */
			result = via6522_int_state & 0x7f;
			if (via6522_int_state & via6522[14]) result |= 0x80;
			break;
	}

	return result;
}

static unsigned int IT32_Read(unsigned int address)
{
	if ((address == 0) || (address == 0x400))
	{
//		printf("reading %x from latch\n", cmd_latch);
		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
		return cmd_latch;
	}

	if (address >= 0x800 && address < 0x83f)
	{
		return ES5506_data_0_r(address-0x800);
	}
	
	if (address >= 0x880 && address < 0x8bf)
	{
		return ES5506_data_0_r(address-0x880);
	}

	if (!brd_type)
	{
		if (address >= 0x1400 && address <= 0x140f)
		{
			return via6522_r(address-0x1400);
		}
	}

	if (address == 0x1800)
	{
		return 0;
	}

	if (address >= 0x2000 && address <= 0x3fff)
	{
		return workram[address];
	}

	if (address >= 0x4000 && address <= 0x7fff)
	{
		return prgrom[address-0x4000 + bnk_offset];
	}

	if (address >= 0x8000)
	{	// the first 3 32k chunks of the rom are the banks, the final 32k is the fixed area
		return prgrom[address-0x8000+(rom_getregionsize(RGN_CPU1)-0x8000)];
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static void IT32_Write(unsigned int address, unsigned int data)
{					
	if (address < 0x20) return;

	if (address >= 0x800 && address <= 0x83f)
	{
		ES5506_data_0_w(address-0x800, data);
		return;
	}

	if (address >= 0x880 && address <= 0x8bf)
	{
		ES5506_data_0_w(address-0x880, data);
		return;
	}

	if (address == 0xc00)
	{
		// bankswitch
		bnk_offset = (data * 0x4000);
//		printf("bankswitch: new bank %x\n", bnk_offset);
		return;
	}

	if (address == 0x1000)
	{
		return;	// watchdog?
	}

	if (!brd_type)
	{
		if (address >= 0x1400 && address <= 0x140f)
		{
			via6522_w(address-0x1400, data);
			return;
		}
	}
	else
	{
		if (address == 0x1400)
		{
			cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
		}
	}

	if (address >= 0x2000 && address <= 0x3fff)
	{
		workram[address] = data;
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static void itech020_timer(int arg)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);

	timer_set(1.0/240.0, 0, itech020_timer);
}

static void IT32_Init(long srate)
{
	// reset the VIA chip
	via6522_timer_count[0] = via6522_timer_count[1] = 0;
	via6522_timer[0] = via6522_timer[1] = 0;
	via6522_int_state = 0;

	brd_type = 0;
}

static void IT32B_Init(long srate)
{
	// reset the VIA chip
	via6522_timer_count[0] = via6522_timer_count[1] = 0;
	via6522_timer[0] = via6522_timer[1] = 0;
	via6522_int_state = 0;

	brd_type = 1;
	timer_set(1.0/240.0, 0, itech020_timer);

	switch (Machine->refcon)
	{
		case 2: //GAME_SFTM:
			m1snd_setCmdSuffixStr(suffix);

			m1snd_addToCmdQueue(0x0c);
			m1snd_addToCmdQueue(0x00);
			m1snd_addToCmdQueue(0xfe);
			m1snd_addToCmdQueue(0x00);
			m1snd_addToCmdQueue(0xfe);
			m1snd_addToCmdQueue(0x00);
			m1snd_addToCmdQueue(0x00);
			m1snd_addToCmdQueue(0x00);
			m1snd_addToCmdQueue(0x00);
			m1snd_addToCmdQueue(0x00);
			m1snd_addToCmdQueue(0xfd);
			m1snd_addToCmdQueue(0x00);
			break;

		case 3: //GAME_SHUFFLESHOT:
			m1snd_addToCmdQueue(0xfe);
			m1snd_addToCmdQueue(0xfb);
			m1snd_addToCmdQueue(0x13);
			m1snd_setCmdPrefix(0xfd);
			break;
	}
}

static void IT32_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;

	cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
}

M1_BOARD_START( itech32 )
	MDRV_NAME("ITech32 (hw rev 1)")
	MDRV_HWDESC("M6809, ES5506")
	MDRV_INIT( IT32_Init )
	MDRV_SEND( IT32_SendCmd )
	MDRV_DELAYS( 100, 50 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&it32rwmem)
	
	MDRV_SOUND_ADD(ES5506, &es5506_interface)
M1_BOARD_END

M1_BOARD_START( itech32b )
	MDRV_NAME("ITech32 (hw rev 2)")
	MDRV_HWDESC("M6809, ES5506")
	MDRV_INIT( IT32B_Init )
	MDRV_SEND( IT32_SendCmd )
	MDRV_DELAYS( 100, 50 )

	MDRV_CPU_ADD(M6809, M6809_CLOCK)
	MDRV_CPUMEMHAND(&it32rwmem)
	
	MDRV_SOUND_ADD(ES5506, &es5506_interface)
M1_BOARD_END
