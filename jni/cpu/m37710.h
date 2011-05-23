#ifndef HEADER__M37710
#define HEADER__M37710

/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

M37710 CPU Emulator v0.1

*/

/* ======================================================================== */
/* =============================== DEFINES ================================ */
/* ======================================================================== */

/* Interrupt lines - used with m37710_set_irq_line().
   WARNING: these are in the same order as the vector table for simplicity.
   Do not alter this order!
*/

enum
{
	// these interrupts are maskable
	M37710_LINE_ADC = 0,
	M37710_LINE_UART1XMIT,
	M37710_LINE_UART1RECV,
	M37710_LINE_UART0XMIT,
	M37710_LINE_UART0RECV,
	M37710_LINE_TIMERB2,
	M37710_LINE_TIMERB1,
	M37710_LINE_TIMERB0,
	M37710_LINE_TIMERA4,
	M37710_LINE_TIMERA3,
	M37710_LINE_TIMERA2,
	M37710_LINE_TIMERA1,
	M37710_LINE_TIMERA0,
	M37710_LINE_IRQ2,
	M37710_LINE_IRQ1,
	M37710_LINE_IRQ0, 
	// these interrupts are non-maskable
	M37710_LINE_WATCHDOG,
	M37710_LINE_DEBUG,
	M37710_LINE_BRK,
	M37710_LINE_ZERODIV,
	M37710_LINE_RESET,

	M37710_LINE_MAX,
};

/* Registers - used by m37710_set_reg() and m37710_get_reg() */
enum
{
	M37710_PC=1, M37710_S, M37710_P, M37710_A, M37710_B, M37710_X, M37710_Y,
	M37710_PB, M37710_DB, M37710_D, M37710_E,
	M37710_NMI_STATE, M37710_IRQ_STATE
};



/* ======================================================================== */
/* ============================== PROTOTYPES ============================== */
/* ======================================================================== */

extern int m37710_ICount;				/* cycle count */

/* ======================================================================== */
/* ================================= MAME ================================= */
/* ======================================================================== */

/* Clean up after the emulation core - Not used in this core - */
void m37710_exit(void);

/* Save the current CPU state to disk */
void m37710_state_save(void *file);

/* Load a CPU state from disk */
void m37710_state_load(void *file);

/* Disassemble an instruction */
unsigned m37710_dasm(char *buffer, unsigned pc);

#undef M37710_CALL_DEBUGGER

#define M37710_CALL_DEBUGGER CALL_MAME_DEBUG
#define m37710_read_8(addr) 			m37710_read(addr)      
#define m37710_write_8(addr,data)		m37710_write(addr, data)
#define m37710_read_8_immediate(A)		m37710_read(A)
#define m37710_read_16(addr) 			m37710_read16(addr)
#define m37710_write_16(addr,data)		m37710_write16(addr,data)
#define m37710_jumping(A)			
#define m37710_branching(A)

void m37710_init(void);
void m37710_reset(void* param);
int m37710_getcycles(void);
void m37710_yield(void);
int m37710_execute(int cycles);
void m37710_set_irq_line(int line, int state);
data8_t m37710_internal_r(int offset);
void m37710_internal_w(int offset, data8_t data);
unsigned m37710_get_reg(int regnum);

void m37710_get_info(UINT32 state, union cpuinfo *info);

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* HEADER__M37710 */
