/*
	Mitsubishi M37710 CPU Emulator

	The 7700 series is based on the WDC 65C816 core, with the following 
	notable changes:

	- Second accumulator called "B" (on the 65816, "A" and "B" were the
	  two 8-bit halves of the 16-bit "C" accumulator).
	- 6502 emulation mode and XCE instruction are not present.
	- No NMI line.  BRK and the watchdog interrupt are non-maskable, but there
	  is no provision for the traditional 6502/65816 NMI line.
	- 3-bit interrupt priority levels like the 68000.  Interrupts in general
	  are very different from the 65816.
	- New single-instruction immediate-to-memory move instructions (LDM)
	  replaces STZ.
	- CLM and SEM (clear and set "M" status bit) replace CLD/SED.  Decimal
	  mode is still available via REP/SEP instructions.
	- INC and DEC (0x1A and 0x3A) switch places for no particular reason.
	- The microcode bug that caused MVN/NVP to take 2 extra cycles per byte 
	  on the 65816 seems to have been fixed.
	- The WDM (0x42) and BIT immediate (0x89) instructions are now prefixes.
	  0x42 when used before an instruction involving the A accumulator makes
	  it use the B accumulator instead.  0x89 adds multiply and divide
	  opcodes, which the real 65816 doesn't have.

	The various 7700 series models differ primarily by their on board 
	peripherals.  The 7750 and later models do include some additional
	instructions, vs. the 770x/1x/2x, notably signed multiply/divide and 
	sign extension opcodes.

	Peripherals common across the 7700 series include: programmable timers,
	digital I/O ports, and analog to digital converters.

	Reference: 7700 Family Software User's Manual (instruction set)
	           7702/7703 Family User's Manual (on-board peripherals)
		   7720 Family User's Manual

	Emulator by R. Belmont.
	Based on G65816 Emulator by Karl Stenrud.

	History:
	- v1.0	RB	First version, basic operation OK, timers not complete
	- v1.1  RB	Data bus is 16-bit, dozens of bugfixes to IRQs, opcodes,
	                and opcode mapping.  New opcodes added, internal timers added.
*/

#include "cpuintrf.h"
#include "memory.h"
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "m37710cm.h"

#define M37710_DEBUG	(0)	// enables verbose logging for peripherals, etc.

#define TSET(x) timer_set(TIME_IN_HZ(time), 0, x)
#define TSET2(x, y) timer_set(TIME_IN_HZ(m37710i_cpu.reload[y]), 0, x)

extern void m37710_set_irq_line(int line, int state);

/* Our CPU structure */
m37710i_cpu_struct m37710i_cpu = {0};

int m37710_ICount = 0, m37710_fullCount = 0;

/* Temporary Variables */
uint m37710i_source;
uint m37710i_destination;

/* interrupt control mapping */

int m37710_irq_levels[M37710_LINE_MAX] =
{
	// maskable
	0x70,	// ADC
	0x73,	// UART 1 XMIT
	0x74,	// UART 1 RECV
	0x71,	// UART 0 XMIT 
	0x72,	// UART 0 RECV 
	0x7c,	// Timer B2
	0x7b,	// Timer B1
	0x7a,	// Timer B0
	0x79,	// Timer A4
	0x78,	// Timer A3
	0x77,	// Timer A2
	0x76,	// Timer A1
	0x75,	// Timer A0
	0x7f,	// IRQ 2
	0x7e,	// IRQ 1
	0x7d,	// IRQ 0
	
	// non-maskable
	0,	// watchdog
	0,	// debugger control
	0,	// BRK
	0,	// divide by zero
	0,	// reset
};

static int m37710_irq_vectors[M37710_LINE_MAX] =
{
	// maskable
	0xffd6, // A-D converter
	0xffd8, // UART1 transmit
	0xffda, // UART1 receive
	0xffdc, // UART0 transmit
	0xffde,	// UART0 receive
	0xffe0, // Timer B2
	0xffe2, // Timer B1
	0xffe4, // Timer B0
	0xffe6, // Timer A4
	0xffe8, // Timer A3
	0xffea, // Timer A2
	0xffec, // Timer A1
	0xffee, // Timer A0
	0xfff0, // external INT2 pin
	0xfff2, // external INT1 pin
	0xfff4, // external INT0 pin

	// non-maskable
	0xfff6, // watchdog timer
	0xfff8,	// debugger control (not used in shipping ICs?)
	0xfffa, // BRK
	0xfffc, // divide by zero
	0xfffe,	// RESET
};

/* Layout of the registers in the MAME debugger */
#if 0
static unsigned char m37710i_register_layout[] =
{
	M37710_PB, M37710_PC, M37710_S, M37710_DB, M37710_D, M37710_P, M37710_E, -1,
	M37710_A, M37710_B, M37710_X, M37710_Y, 0
};

/* Layout of the MAME debugger windows x,y,w,h */
static unsigned char m37710i_window_layout[] = {
	 0, 0,80, 4,	/* register window (top rows) */
	 0, 5,34,16,	/* disassembler window (left colums) */
	35, 5,50, 7,	/* memory #1 window (right, upper middle) */
	35,14,50, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};
#endif

// M37710 internal peripherals

#if M37710_DEBUG
static char *m37710_rnames[128] =
{
	"",
	"",
	"Port P0 reg",
	"Port P1 reg",
	"Port P0 dir reg",
	"Port P1 dir reg",
	"Port P2 reg",
	"Port P3 reg",
	"Port P2 dir reg",
	"Port P3 dir reg",
	"Port P4 reg",
	"Port P5 reg",
	"Port P4 dir reg",
	"Port P5 dir reg",
	"Port P6 reg",
	"Port P7 reg",
	"Port P6 dir reg",
	"Port P7 dir reg",
	"Port P8 reg",
	"",
	"Port P8 dir reg",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"A/D control reg",
	"A/D sweep pin select",
	"A/D 0",
	"",
	"A/D 1",
	"",
	"A/D 2",
	"",
	"A/D 3",
	"",
	"A/D 4",
	"",
	"A/D 5",
	"",
	"A/D 6",
	"",
	"A/D 7",
	"",
	"UART0 transmit/recv mode",
	"UART0 baud rate",
	"UART0 transmit buf L",
	"UART0 transmit buf H",
	"UART0 transmit/recv ctrl 0",
	"UART0 transmit/recv ctrl 1",
	"UART0 recv buf L",
	"UART0 recv buf H",
	"UART1 transmit/recv mode",
	"UART1 baud rate",
	"UART1 transmit buf L",
	"UART1 transmit buf H",
	"UART1 transmit/recv ctrl 0",
	"UART1 transmit/recv ctrl 1",
	"UART1 recv buf L",
	"UART1 recv buf H",
	"Count start",
	"",
	"One-shot start",
	"",
	"Up-down register",
	"",
	"Timer A0 L",
	"Timer A0 H",
	"Timer A1 L",
	"Timer A1 H",
	"Timer A2 L",
	"Timer A2 H",
	"Timer A3 L",
	"Timer A3 H",
	"Timer A4 L",
	"Timer A4 H",
	"Timer B0 L",
	"Timer B0 H",
	"Timer B1 L",
	"Timer B1 H",
	"Timer B2 L",
	"Timer B2 H",
	"Timer A0 mode",
	"Timer A1 mode",
	"Timer A2 mode",
	"Timer A3 mode",
	"Timer A4 mode",
	"Timer B0 mode",
	"Timer B1 mode",
	"Timer B2 mode",
	"Processor mode",
	"",
	"Watchdog reset",
	"Watchdog frequency",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"A/D IRQ ctrl",
	"UART0 xmit IRQ ctrl",
	"UART0 recv IRQ ctrl",
	"UART1 xmit IRQ ctrl",
	"UART1 recv IRQ ctrl",
	"Timer A0 IRQ ctrl",
	"Timer A1 IRQ ctrl",
	"Timer A2 IRQ ctrl",
	"Timer A3 IRQ ctrl",
	"Timer A4 IRQ ctrl",
	"Timer B0 IRQ ctrl",
	"Timer B1 IRQ ctrl",
	"Timer B2 IRQ ctrl",
	"INT0 IRQ ctrl",
	"INT1 IRQ ctrl",
	"INT2 IRQ ctrl",
};

static char *m37710_tnames[8] =
{
	"A0", "A1", "A2", "A3", "A4", "B0", "B1", "B2"
};
#endif

static void m37710_timer_a0_cb(int num)
{
//	timer_adjust(m37710i_cpu.timers[0], TIME_IN_HZ(m37710i_cpu.reload[0]), 0, 0);
	TSET2(m37710_timer_a0_cb, 0);

	m37710i_cpu.m37710_regs[m37710_irq_levels[12]] |= 0x04;
	m37710_set_irq_line(M37710_LINE_TIMERA0, PULSE_LINE);
}

static void m37710_timer_a1_cb(int num)
{
//	timer_adjust(m37710i_cpu.timers[1], TIME_IN_HZ(m37710i_cpu.reload[1]), 0, 0);
	TSET2(m37710_timer_a1_cb, 1);

	m37710i_cpu.m37710_regs[m37710_irq_levels[11]] |= 0x04;
	m37710_set_irq_line(M37710_LINE_TIMERA1, PULSE_LINE);
}

static void m37710_timer_a2_cb(int num)
{
//	timer_adjust(m37710i_cpu.timers[2], TIME_IN_HZ(m37710i_cpu.reload[2]), 0, 0);
	TSET2(m37710_timer_a2_cb, 2);

	m37710i_cpu.m37710_regs[m37710_irq_levels[10]] |= 0x04;
	m37710_set_irq_line(M37710_LINE_TIMERA2, PULSE_LINE);
}

static void m37710_timer_a3_cb(int num)
{
//	timer_adjust(m37710i_cpu.timers[3], TIME_IN_HZ(m37710i_cpu.reload[3]), 0, 0);
	TSET2(m37710_timer_a3_cb, 3);

	m37710i_cpu.m37710_regs[m37710_irq_levels[9]] |= 0x04;
	m37710_set_irq_line(M37710_LINE_TIMERA3, PULSE_LINE);
}

static void m37710_timer_a4_cb(int num)
{
//	timer_adjust(m37710i_cpu.timers[4], TIME_IN_HZ(m37710i_cpu.reload[4]), 0, 0);
	TSET2(m37710_timer_a4_cb, 4);

	m37710i_cpu.m37710_regs[m37710_irq_levels[8]] |= 0x04;
	m37710_set_irq_line(M37710_LINE_TIMERA4, PULSE_LINE);
}

static void m37710_timer_b0_cb(int num)
{
//	timer_adjust(m37710i_cpu.timers[5], TIME_IN_HZ(m37710i_cpu.reload[5]), 0, 0);
	TSET2(m37710_timer_b0_cb, 5);

	m37710i_cpu.m37710_regs[m37710_irq_levels[7]] |= 0x04;
	m37710_set_irq_line(M37710_LINE_TIMERB0, PULSE_LINE);
}

static void m37710_timer_b1_cb(int num)
{
//	timer_adjust(m37710i_cpu.timers[6], TIME_IN_HZ(m37710i_cpu.reload[6]), 0, 0);
	TSET2(m37710_timer_b1_cb, 6);

	m37710i_cpu.m37710_regs[m37710_irq_levels[6]] |= 0x04;
	m37710_set_irq_line(M37710_LINE_TIMERB1, PULSE_LINE);
}

static void m37710_timer_b2_cb(int num)
{
//	timer_adjust(m37710i_cpu.timers[7], TIME_IN_HZ(m37710i_cpu.reload[7]), 0, 0);
	TSET2(m37710_timer_b2_cb, 7);

	m37710i_cpu.m37710_regs[m37710_irq_levels[5]] |= 0x04;
	m37710_set_irq_line(M37710_LINE_TIMERB2, PULSE_LINE);
}

static void m37710_recalc_timer(int timer)
{
	int tval;
	int tcr[8] = { 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d };
	double time;
	static double tscales[4] = { 2.0, 16.0, 64.0, 512.0 };

	// check if enabled
	if (m37710i_cpu.m37710_regs[0x40] & (1<<timer))
	{
		#if M37710_DEBUG
		printf("Timer %d (%s) is enabled\n", timer, m37710_tnames[timer]);
		#endif

		// set the timer's value
		tval = m37710i_cpu.m37710_regs[0x46+(timer*2)] | (m37710i_cpu.m37710_regs[0x47+(timer*2)]<<8);

		// check timer's mode
		// modes are slightly different between timer groups A and B
		if (timer < 5)
		{	
			switch (m37710i_cpu.m37710_regs[0x56+timer] & 0x3)
			{
				case 0:	      	// timer mode
					#if M37710_DEBUG
					logerror("Timer %d in timer mode\n", timer);
					#endif					

					time = (double)timer_get_cpu_clock(cpu_getactivecpu()) / tscales[m37710i_cpu.m37710_regs[tcr[timer]]>>6];
					time /= ((double)tval+1.0);
	
//					timer_adjust(m37710i_cpu.timers[timer], TIME_IN_HZ(time), 0, 0);

//					printf("Timer %d (%s) is enabled, time is %f Hz\n", timer, m37710_tnames, time);

					switch (timer)
					{
						case 0: TSET(m37710_timer_a0_cb); break;
						case 1: TSET(m37710_timer_a1_cb); break;
						case 2: TSET(m37710_timer_a2_cb); break;
						case 3: TSET(m37710_timer_a3_cb); break;
						case 4: TSET(m37710_timer_a4_cb); break;
					}

					m37710i_cpu.reload[timer] = time;
					break;

				case 1:	      	// event counter mode
					#if M37710_DEBUG
					logerror("Timer %d in event counter mode\n", timer);
					#endif
					break;

				case 2:		// one-shot pulse mode
					#if M37710_DEBUG
					logerror("Timer %d in one-shot mode\n", timer);
					#endif
					break;

				case 3:	      	// PWM mode
					#if M37710_DEBUG
					logerror("Timer %d in PWM mode\n", timer);
					#endif
					break;
			}
		}
		else
		{
			switch (m37710i_cpu.m37710_regs[0x56+timer] & 0x3)
			{
				case 0:	      	// timer mode
					#if M37710_DEBUG
					logerror("Timer %d in timer mode\n", timer);
					#endif

					time = (double)timer_get_cpu_clock(cpu_getactivecpu()) / tscales[m37710i_cpu.m37710_regs[tcr[timer]]>>6];
					time /= ((double)tval+1.0);
	
//					timer_adjust(m37710i_cpu.timers[timer], TIME_IN_HZ(time), 0, 0);
					switch (timer)
					{
						case 5: TSET(m37710_timer_b0_cb); break;
						case 6: TSET(m37710_timer_b1_cb); break;
						case 7: TSET(m37710_timer_b2_cb); break;
					}
					m37710i_cpu.reload[timer] = time;
					break;

				case 1:	      	// event counter mode
					#if M37710_DEBUG
					logerror("Timer %d in event counter mode\n", timer);
					#endif
					break;

				case 2:		// pulse period/pulse width measurement mode
					#if M37710_DEBUG
					logerror("Timer %d in pulse period/width measurement mode\n", timer);
					#endif
					break;

				case 3:	      
					#if M37710_DEBUG
					logerror("Timer %d in unknown mode!\n", timer);
					#endif
					break;
			}
		}
	}
}

int toggle = 0;

data8_t m37710_internal_r(int offset)
{
	#if M37710_DEBUG
	logerror("m37710_internal_r from %02x: %s (PC=%x)\n", (int)offset, m37710_rnames[(int)offset], REG_PB<<16 | REG_PC);
	#endif

	switch (offset)
	{
		case 2: // p0
		case 3: // p1
		case 6: // p2
		case 7: // p3
		case 0xb: // p5
		case 0xf: // p7
		case 0x12: // p8
			#if M37710_DEBUG
//			logerror("m37710: read I/O port @ %x (PC=%06x)\n", offset, REG_PC);
			#endif
			return 0;
			break;

		case 0xa: // p4
				return 0x10;
			break;

		case 0xe: // p6
			toggle ^= 0xff;
			return toggle;
			break;
		case 0x35:
			return 0xff;	// UART control
			break;
	}

	return m37710i_cpu.m37710_regs[offset];
}

void m37710_internal_w(int offset, data8_t data)
{
	int i;

	switch(offset)
	{
		case 2: // p0
		case 3: // p1
		case 6: // p2
		case 7: // p3
		case 0xa: // p4
		case 0xb: // p5
		case 0xe: // p6
		case 0xf: // p7
		case 0x12: // p8
			#if M37710_DEBUG
//			logerror("m37710: %02x to I/O port @ %x (PC=%06x)\n", data, offset, REG_PC);
			#endif
			break;

		case 0x40:	// count start
			for (i = 0; i < 8; i++)
			{
				if ((data & (1<<i)) && !(m37710i_cpu.m37710_regs[offset] & (1<<i)))
				{
					m37710i_cpu.m37710_regs[offset] |= (1<<i);
					m37710_recalc_timer(i);
				}
			}

			m37710i_cpu.m37710_regs[offset] = data;

			return;
			break;

		case 0x60:     	// watchdog reset
			return;
			break;
	}

	m37710i_cpu.m37710_regs[offset] = data;

	#if M37710_DEBUG
	logerror("m37710_internal_w %x to %02x: %s = %x\n", data, (int)offset, m37710_rnames[(int)offset], m37710i_cpu.m37710_regs[offset]);
	#endif
}

#if 0
static READ16_HANDLER( m37710_internal_word_r )
{
	return (m37710_internal_r(offset*2) | m37710_internal_r((offset*2)+1)<<8);
}

static WRITE16_HANDLER( m37710_internal_word_w )
{
	if (mem_mask == 0)
	{
		m37710_internal_w((offset*2), data & 0xff);
		m37710_internal_w((offset*2)+1, data>>8);
	}
	else if (mem_mask == 0xff)
	{
		m37710_internal_w((offset*2)+1, data>>8);
	}
	else if (mem_mask == 0xff00)
	{
		m37710_internal_w((offset*2), data & 0xff);
	}
}
#endif

extern void (*m37710i_opcodes_M0X0[])(void);
extern void (*m37710i_opcodes42_M0X0[])(void);
extern void (*m37710i_opcodes89_M0X0[])(void);
extern uint m37710i_get_reg_M0X0(int regnum);
extern void m37710i_set_reg_M0X0(int regnum, uint val);
extern void m37710i_set_line_M0X0(int line, int state);
extern int  m37710i_execute_M0X0(int cycles);

extern void (*m37710i_opcodes_M0X1[])(void);
extern void (*m37710i_opcodes42_M0X1[])(void);
extern void (*m37710i_opcodes89_M0X1[])(void);
extern uint m37710i_get_reg_M0X1(int regnum);
extern void m37710i_set_reg_M0X1(int regnum, uint val);
extern void m37710i_set_line_M0X1(int line, int state);
extern int  m37710i_execute_M0X1(int cycles);

extern void (*m37710i_opcodes_M1X0[])(void);
extern void (*m37710i_opcodes42_M1X0[])(void);
extern void (*m37710i_opcodes89_M1X0[])(void);
extern uint m37710i_get_reg_M1X0(int regnum);
extern void m37710i_set_reg_M1X0(int regnum, uint val);
extern void m37710i_set_line_M1X0(int line, int state);
extern int  m37710i_execute_M1X0(int cycles);

extern void (*m37710i_opcodes_M1X1[])(void);
extern void (*m37710i_opcodes42_M1X1[])(void);
extern void (*m37710i_opcodes89_M1X1[])(void);
extern uint m37710i_get_reg_M1X1(int regnum);
extern void m37710i_set_reg_M1X1(int regnum, uint val);
extern void m37710i_set_line_M1X1(int line, int state);
extern int  m37710i_execute_M1X1(int cycles);

void (**m37710i_opcodes[4])(void) =
{
	m37710i_opcodes_M0X0,
	m37710i_opcodes_M0X1,
	m37710i_opcodes_M1X0,
	m37710i_opcodes_M1X1,
};

void (**m37710i_opcodes2[4])(void) =
{
	m37710i_opcodes42_M0X0,
	m37710i_opcodes42_M0X1,
	m37710i_opcodes42_M1X0,
	m37710i_opcodes42_M1X1,
};

void (**m37710i_opcodes3[4])(void) =
{
	m37710i_opcodes89_M0X0,
	m37710i_opcodes89_M0X1,
	m37710i_opcodes89_M1X0,
	m37710i_opcodes89_M1X1,
};

uint (*m37710i_get_reg[4])(int regnum) =
{
	m37710i_get_reg_M0X0,
	m37710i_get_reg_M0X1,
	m37710i_get_reg_M1X0,
	m37710i_get_reg_M1X1,
};

void (*m37710i_set_reg[4])(int regnum, uint val) =
{
	m37710i_set_reg_M0X0,
	m37710i_set_reg_M0X1,
	m37710i_set_reg_M1X0,
	m37710i_set_reg_M1X1,
};

void (*m37710i_set_line[4])(int line, int state) =
{
	m37710i_set_line_M0X0,
	m37710i_set_line_M0X1,
	m37710i_set_line_M1X0,
	m37710i_set_line_M1X1,
};

int (*m37710i_execute[4])(int cycles) =
{
	m37710i_execute_M0X0,
	m37710i_execute_M0X1,
	m37710i_execute_M1X0,
	m37710i_execute_M1X1,
};

/* internal functions */

INLINE void m37710i_push_8(uint value)
{
	m37710_write_8(REG_S, value);
	REG_S = MAKE_UINT_16(REG_S-1);
}

INLINE void m37710i_push_16(uint value)
{
	m37710i_push_8(value>>8);
	m37710i_push_8(value&0xff);
}

INLINE uint m37710i_get_reg_p(void)
{
	return	(FLAG_N&0x80)		|
			((FLAG_V>>1)&0x40)	|
			FLAG_M				|
			FLAG_X				|
			FLAG_D				|
			FLAG_I				|
			((!FLAG_Z)<<1)		|
			((FLAG_C>>8)&1);
}

void m37710i_update_irqs(void)
{
	int curirq, pending = LINE_IRQ;
	int wantedIRQ, curpri;

	if (FLAG_I)
	{
		return;
	}

	curpri = -1;
	wantedIRQ = -1;

	for (curirq = M37710_LINE_MAX - 1; curirq >= 0; curirq--)
	{
		if ((pending & (1 << curirq)))
		{
			// this IRQ is set
			if (m37710_irq_levels[curirq])
			{
				// it's maskable, check if the level works
				if ((m37710i_cpu.m37710_regs[m37710_irq_levels[curirq]] & 7) > curpri)
				{
					// also make sure it's acceptable for the current CPU level
					if ((m37710i_cpu.m37710_regs[m37710_irq_levels[curirq]] & 7) > m37710i_cpu.ipl)
					{
						// mark us as the best candidate
						wantedIRQ = curirq;
						curpri = m37710i_cpu.m37710_regs[m37710_irq_levels[curirq]] & 7;
					}
				}
			}
			else
			{
				// non-maskable
				wantedIRQ = curirq;
				curirq = -1;
				break;	// no more processing, NMIs always win
			}
		}
	}

	if (wantedIRQ != -1)
	{
		if (INT_ACK) INT_ACK(wantedIRQ);

		// indicate we're servicing it now
		if (m37710_irq_levels[wantedIRQ])
		{
			m37710i_cpu.m37710_regs[m37710_irq_levels[wantedIRQ]] &= ~8;
		}

		// auto-clear if it's an internal line
		if (wantedIRQ <= 12)
		{
			m37710_set_irq_line(wantedIRQ, CLEAR_LINE);
		}

		// let's do it...
		// push PB, then PC, then status
		CLK(8);
//		logerror("taking IRQ %d: PC = %06x, SP = %04x\n", wantedIRQ, REG_PB | REG_PC, REG_S);
		m37710i_push_8(REG_PB>>16); 
		m37710i_push_16(REG_PC);
		m37710i_push_8(m37710i_cpu.ipl);
		m37710i_push_8(m37710i_get_reg_p());

		// set I to 1, set IPL to the interrupt we're taking
		FLAG_I = IFLAG_SET;
		m37710i_cpu.ipl = curpri;
		// then PB=0, PC=(vector)
		REG_PB = 0;
		REG_PC = m37710_read_8(m37710_irq_vectors[wantedIRQ]) | 
			 m37710_read_8(m37710_irq_vectors[wantedIRQ]+1)<<8;
//		logerror("IRQ @ %06x\n", REG_PB | REG_PC);
		m37710i_jumping(REG_PB | REG_PC);
	}
}

/* external functions */

void m37710_reset(void* param)
{
	/* Start the CPU */
	CPU_STOPPED = 0;

	/* 37710 boots in full native mode */
	REG_D = 0;
	REG_PB = 0;
	REG_DB = 0;
	REG_S = (REG_S & 0xff) | 0x100;
	REG_A = 0;
	REG_BA = 0;
	REG_B = 0;
	REG_BB = 0;
	REG_X = 0;
	REG_Y = 0;
	if(!FLAG_M)
	{
		REG_B = REG_A & 0xff00;
		REG_A &= 0xff;
	}
	FLAG_M = MFLAG_CLEAR;
	FLAG_X = XFLAG_CLEAR;

	/* Clear D and set I */
	FLAG_D = DFLAG_CLEAR;
	FLAG_I = IFLAG_SET;

	/* Clear all pending interrupts (should we really do this?) */
	LINE_IRQ = 0;
	IRQ_DELAY = 0;

	/* Set the function tables to emulation mode */
	m37710i_set_execution_mode(EXECUTION_MODE_M0X0);
	
	FLAG_Z = ZFLAG_CLEAR;
	REG_S = 0x1ff;

	/* Fetch the reset vector */
	REG_PC = m37710_read_8(0xfffe) | (m37710_read_8(0xffff)<<8);
	m37710i_jumping(REG_PB | REG_PC);

	/* Clear the internal registers */
	memset(m37710i_cpu.m37710_regs, 0, 128);
}

/* Exit and clean up */
void m37710_exit(void)
{
	/* nothing to do yet */
}

/* return elapsed cycles in the current slice */
int m37710_getcycles(void)
{
	return (m37710_fullCount - m37710_ICount);
}

/* yield the current slice */
void m37710_yield(void)
{
	m37710_fullCount = m37710_getcycles();

	m37710_ICount = 0;
}

/* Execute some instructions */
int m37710_execute(int cycles)
{
	m37710_fullCount = cycles;

	m37710i_update_irqs();

	return FTABLE_EXECUTE(cycles);
}


/* Get the current CPU context */
void m37710_get_context(void *dst_context)
{
	*(m37710i_cpu_struct*)dst_context = m37710i_cpu;
}

/* Set the current CPU context */
void m37710_set_context(void *src_context)
{
	m37710i_cpu = *(m37710i_cpu_struct*)src_context;
	m37710i_jumping(REG_PB | REG_PC);
}

/* Get the current Program Counter */
unsigned m37710_get_pc(void)
{
	return REG_PC;
}

/* Set the Program Counter */
void m37710_set_pc(unsigned val)
{
	REG_PC = MAKE_UINT_16(val);
	m37710_jumping(REG_PB | REG_PC);
}

/* Get the current Stack Pointer */
unsigned m37710_get_sp(void)
{
	return REG_S;
}

/* Set the Stack Pointer */
void m37710_set_sp(unsigned val)
{
	REG_S = MAKE_UINT_16(val);
}

/* Get a register */
unsigned m37710_get_reg(int regnum)
{
	return FTABLE_GET_REG(regnum);
}

/* Set a register */
void m37710_set_reg(int regnum, unsigned value)
{
	FTABLE_SET_REG(regnum, value);
}

/* Load a CPU state */
void m37710_state_load(void *file)
{
}

/* Save the current CPU state */
void m37710_state_save(void *file)
{
}

/* Set an interrupt line */
void m37710_set_irq_line(int line, int state)
{
	FTABLE_SET_LINE(line, state);
}

/* Set the callback that is called when servicing an interrupt */
void m37710_set_irq_callback(int (*callback)(int))
{
	INT_ACK = callback;
}

/* Disassemble an instruction */
#ifdef MAME_DEBUG
#include "m7700ds.h"
#endif
unsigned m37710_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return m7700_disassemble(buffer, (pc&0xffff), REG_PB>>16, FLAG_M, FLAG_X);
#else
	sprintf(buffer, "$%02X", m37710_read_8_immediate(REG_PB | (pc&0xffff)));
	return 1;
#endif
}


void m37710_init(void)
{ 
#if 0
	m37710i_cpu.timers[0] = timer_alloc(m37710_timer_a0_cb);
	m37710i_cpu.timers[1] = timer_alloc(m37710_timer_a1_cb);
	m37710i_cpu.timers[2] = timer_alloc(m37710_timer_a2_cb);
	m37710i_cpu.timers[3] = timer_alloc(m37710_timer_a3_cb);
	m37710i_cpu.timers[4] = timer_alloc(m37710_timer_a4_cb);
	m37710i_cpu.timers[5] = timer_alloc(m37710_timer_b0_cb);
	m37710i_cpu.timers[6] = timer_alloc(m37710_timer_b1_cb);
	m37710i_cpu.timers[7] = timer_alloc(m37710_timer_b2_cb);
#endif
}

/**************************************************************************
 * Generic set_info
 **************************************************************************/
#if 0
static void m37710_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + M37710_LINE_IRQ0: 	m37710_set_irq_line(M37710_LINE_IRQ0, info->i); break;
		case CPUINFO_INT_INPUT_STATE + M37710_LINE_IRQ1: 	m37710_set_irq_line(M37710_LINE_IRQ1, info->i); break;
		case CPUINFO_INT_INPUT_STATE + M37710_LINE_IRQ2: 	m37710_set_irq_line(M37710_LINE_IRQ2, info->i); break;

		case CPUINFO_INT_PC:					REG_PB = info->i & 0xff0000; m37710_set_pc(info->i & 0xffff);	break;
		case CPUINFO_INT_SP:					m37710_set_sp(info->i);	     			break;

		case CPUINFO_INT_REGISTER + M37710_PC:			m37710_set_reg(M37710_PC, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_S:			m37710_set_reg(M37710_S, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_P:			m37710_set_reg(M37710_P, info->i&0xff);	m37710i_cpu.ipl = (info->i>>8)&0xff;	break;
		case CPUINFO_INT_REGISTER + M37710_A:			m37710_set_reg(M37710_A, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_X:			m37710_set_reg(M37710_X, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_Y:			m37710_set_reg(M37710_Y, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_PB:			m37710_set_reg(M37710_PB, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_DB:			m37710_set_reg(M37710_DB, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_D:			m37710_set_reg(M37710_D, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_E:			m37710_set_reg(M37710_E, info->i);		break;
		case CPUINFO_INT_REGISTER + M37710_NMI_STATE:		m37710_set_reg(M37710_NMI_STATE, info->i); break;
		case CPUINFO_INT_REGISTER + M37710_IRQ_STATE:		m37710_set_reg(M37710_IRQ_STATE, info->i); break;

		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_IRQ_CALLBACK:					INT_ACK = info->irqcallback; break;
	}
}

// On-board RAM and peripherals
static ADDRESS_MAP_START( m37710_internal_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00007f) AM_READWRITE(m37710_internal_word_r, m37710_internal_word_w)
	AM_RANGE(0x000080, 0x00027f) AM_RAM
ADDRESS_MAP_END

/**************************************************************************
 * Generic get_info
 **************************************************************************/

void m37710_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(m37710i_cpu);			break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:				info->i = 0;					break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;				break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;					break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:				info->i = 1;					break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:				info->i = 5;					break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;					break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 20; /* rough guess */			break;
		case CPUINFO_INT_INPUT_LINES:        				info->i = M37710_LINE_MAX;                      break;
		
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;				break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 24;				break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;				break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;				break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;				break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;				break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:	info->i = 8;				break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 	info->i = 16;				break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 	info->i = 0;				break;

		case CPUINFO_INT_INPUT_STATE + M37710_LINE_IRQ0: 	info->i = 0;				break;
		case CPUINFO_INT_INPUT_STATE + M37710_LINE_IRQ1: 	info->i = 0;				break;
		case CPUINFO_INT_INPUT_STATE + M37710_LINE_IRQ2: 	info->i = 0;				break;
		case CPUINFO_INT_INPUT_STATE + M37710_LINE_RESET:	info->i = 0;				break;

		case CPUINFO_INT_PREVIOUSPC:				info->i = REG_PPC;			break;
		case CPUINFO_INT_PC:	 				info->i = (REG_PB | REG_PC);		break;
		case CPUINFO_INT_SP:					info->i = m37710_get_sp();		break;

		case CPUINFO_INT_REGISTER + M37710_PC:			info->i = m37710_get_reg(M37710_PC);	break;
		case CPUINFO_INT_REGISTER + M37710_S:			info->i = m37710_get_reg(M37710_S);	break;
		case CPUINFO_INT_REGISTER + M37710_P:			info->i = m37710_get_reg(M37710_P) | (m37710i_cpu.ipl<<8);	break;
		case CPUINFO_INT_REGISTER + M37710_A:			info->i = m37710_get_reg(M37710_A);	break;
		case CPUINFO_INT_REGISTER + M37710_B:			info->i = m37710_get_reg(M37710_B);	break;
		case CPUINFO_INT_REGISTER + M37710_X:			info->i = m37710_get_reg(M37710_X);	break;
		case CPUINFO_INT_REGISTER + M37710_Y:			info->i = m37710_get_reg(M37710_Y);	break;
		case CPUINFO_INT_REGISTER + M37710_PB:			info->i = m37710_get_reg(M37710_PB);	break;
		case CPUINFO_INT_REGISTER + M37710_DB:			info->i = m37710_get_reg(M37710_DB);	break;
		case CPUINFO_INT_REGISTER + M37710_D:			info->i = m37710_get_reg(M37710_D);	break;
		case CPUINFO_INT_REGISTER + M37710_E:			info->i = m37710_get_reg(M37710_E);	break;
		case CPUINFO_INT_REGISTER + M37710_NMI_STATE:	info->i = m37710_get_reg(M37710_NMI_STATE); 	break;
		case CPUINFO_INT_REGISTER + M37710_IRQ_STATE:	info->i = m37710_get_reg(M37710_IRQ_STATE); 	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = m37710_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m37710_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m37710_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = m37710_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = m37710_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = m37710_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = m37710_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m37710_dasm;		break;
		case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = INT_ACK;			break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m37710_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = m37710i_register_layout;		break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m37710i_window_layout;		break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = &construct_map_m37710_internal_map; break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_DATA:    info->internal_map = 0; break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_IO:      info->internal_map = 0; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:						strcpy(info->s = cpuintrf_temp_str(), "M37710"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "M7700"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.1"); break;
		case CPUINFO_STR_CORE_FILE:					strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (c) 2004 R. Belmont, based on G65816 by Karl Stenerud"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
				m37710i_cpu.flag_n & NFLAG_SET ? 'N':'.',
				m37710i_cpu.flag_v & VFLAG_SET ? 'V':'.',
				m37710i_cpu.flag_m & MFLAG_SET ? 'M':'.',
				m37710i_cpu.flag_x & XFLAG_SET ? 'X':'.',
				m37710i_cpu.flag_d & DFLAG_SET ? 'D':'.',
				m37710i_cpu.flag_i & IFLAG_SET ? 'I':'.',
				m37710i_cpu.flag_z == 0        ? 'Z':'.',
				m37710i_cpu.flag_c & CFLAG_SET ? 'C':'.');
			break;

		case CPUINFO_STR_REGISTER + M37710_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", m37710i_cpu.pc); break;
		case CPUINFO_STR_REGISTER + M37710_PB:			sprintf(info->s = cpuintrf_temp_str(), "PB:%02X", m37710i_cpu.pb>>16); break;
		case CPUINFO_STR_REGISTER + M37710_DB:			sprintf(info->s = cpuintrf_temp_str(), "DB:%02X", m37710i_cpu.db>>16); break;
		case CPUINFO_STR_REGISTER + M37710_D:			sprintf(info->s = cpuintrf_temp_str(), "D:%04X", m37710i_cpu.d); break;
		case CPUINFO_STR_REGISTER + M37710_S:			sprintf(info->s = cpuintrf_temp_str(), "S:%04X", m37710i_cpu.s); break;
		case CPUINFO_STR_REGISTER + M37710_P:			sprintf(info->s = cpuintrf_temp_str(), "P:%04X",
																 (m37710i_cpu.flag_n&0x80)		|
																((m37710i_cpu.flag_v>>1)&0x40)	|
																m37710i_cpu.flag_m				|
																m37710i_cpu.flag_x				|
																m37710i_cpu.flag_d				|
																m37710i_cpu.flag_i				|
																((!m37710i_cpu.flag_z)<<1)		|
																((m37710i_cpu.flag_c>>8)&1)) | (m37710i_cpu.ipl<<8); break;
		case CPUINFO_STR_REGISTER + M37710_E:			sprintf(info->s = cpuintrf_temp_str(), "E:%d", m37710i_cpu.flag_e); break;
		case CPUINFO_STR_REGISTER + M37710_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%04X", m37710i_cpu.a | m37710i_cpu.b); break;
		case CPUINFO_STR_REGISTER + M37710_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%04X", m37710i_cpu.ba | m37710i_cpu.bb); break;
		case CPUINFO_STR_REGISTER + M37710_X:			sprintf(info->s = cpuintrf_temp_str(), "X:%04X", m37710i_cpu.x); break;
		case CPUINFO_STR_REGISTER + M37710_Y:			sprintf(info->s = cpuintrf_temp_str(), "Y:%04X", m37710i_cpu.y); break;
		case CPUINFO_STR_REGISTER + M37710_IRQ_STATE:	sprintf(info->s = cpuintrf_temp_str(), "IRQ:%X", m37710i_cpu.line_irq); break;
	}
}
#endif
/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */
