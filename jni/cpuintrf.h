#ifndef CPUINTRF_H
#define CPUINTRF_H

#include "osd_cpu.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CALL_MAME_DEBUG

void cpuintrf_boot(void);
void cpuintrf_reset(void);

#define ADDRESS_SPACES (3)
#define MAX_INPUT_LINES (32+3)
#define MAX_OUTPUT_LINES (32	)

enum
{
	ADDRESS_SPACE_PROGRAM = 0,
	ADDRESS_SPACE_DATA,
	ADDRESS_SPACE_IO,
};

/* get_reg/set_reg constants */
enum
{
	MAX_REGS = 128,				/* maximum number of register of any CPU */

	/* This value is passed to activecpu_get_reg to retrieve the previous
	 * program counter value, ie. before a CPU emulation started
	 * to fetch opcodes and arguments for the current instrution. */
	REG_PREVIOUSPC = -1,

	/* This value is passed to activecpu_get_reg to retrieve the current
	 * program counter value. */
	REG_PC = -2,

	/* This value is passed to activecpu_get_reg to retrieve the current
	 * stack pointer value. */
	REG_SP = -3,

	/* This value is passed to activecpu_get_reg/activecpu_set_reg, instead of one of
	 * the names from the enum a CPU core defines for it's registers,
	 * to get or set the contents of the memory pointed to by a stack pointer.
	 * You can specify the n'th element on the stack by (REG_SP_CONTENTS-n),
	 * ie. lower negative values. The actual element size (UINT16 or UINT32)
	 * depends on the CPU core. */
	REG_SP_CONTENTS = -4
};

enum
{
	/* line states */
	CLEAR_LINE = 0,				/* clear (a fired, held or pulsed) line */
	ASSERT_LINE,				/* assert an interrupt immediately */
	HOLD_LINE,					/* hold interrupt line until acknowledged */
	PULSE_LINE,					/* pulse interrupt line for one instruction */

	/* internal flags (not for use by drivers!) */
	INTERNAL_CLEAR_LINE = 100 + CLEAR_LINE,
	INTERNAL_ASSERT_LINE = 100 + ASSERT_LINE,

	/* interrupt parameters */
	MAX_IRQ_LINES =	8,			/* maximum number of IRQ lines per CPU */
	IRQ_LINE_NMI = 127			/* IRQ line for NMIs */
};

enum
{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	CPUINFO_INT_FIRST = 0x00000,

	CPUINFO_INT_CONTEXT_SIZE = CPUINFO_INT_FIRST,		/* R/O: size of CPU context in bytes */
	CPUINFO_INT_INPUT_LINES,							/* R/O: number of input lines */
	CPUINFO_INT_OUTPUT_LINES,							/* R/O: number of output lines */
	CPUINFO_INT_DEFAULT_IRQ_VECTOR,						/* R/O: default IRQ vector */
	CPUINFO_INT_ENDIANNESS,								/* R/O: either CPU_IS_BE or CPU_IS_LE */
	CPUINFO_INT_CLOCK_DIVIDER,							/* R/O: internal clock divider */
	CPUINFO_INT_MIN_INSTRUCTION_BYTES,					/* R/O: minimum bytes per instruction */
	CPUINFO_INT_MAX_INSTRUCTION_BYTES,					/* R/O: maximum bytes per instruction */
	CPUINFO_INT_MIN_CYCLES,								/* R/O: minimum cycles for a single instruction */
	CPUINFO_INT_MAX_CYCLES,								/* R/O: maximum cycles for a single instruction */

	CPUINFO_INT_DATABUS_WIDTH,							/* R/O: data bus size for each address space (8,16,32,64) */
	CPUINFO_INT_DATABUS_WIDTH_LAST = CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACES - 1,
	CPUINFO_INT_ADDRBUS_WIDTH,							/* R/O: address bus size for each address space (12-32) */
	CPUINFO_INT_ADDRBUS_WIDTH_LAST = CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACES - 1,
	CPUINFO_INT_ADDRBUS_SHIFT,							/* R/O: shift applied to addresses each address space (+3 means >>3, -1 means <<1) */
	CPUINFO_INT_ADDRBUS_SHIFT_LAST = CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACES - 1,

	CPUINFO_INT_SP,										/* R/W: the current stack pointer value */
	CPUINFO_INT_PC,										/* R/W: the current PC value */
	CPUINFO_INT_PREVIOUSPC,								/* R/W: the previous PC value */
	CPUINFO_INT_INPUT_STATE,							/* R/W: states for each input line */
	CPUINFO_INT_INPUT_STATE_LAST = CPUINFO_INT_INPUT_STATE + MAX_INPUT_LINES - 1,
	CPUINFO_INT_OUTPUT_STATE,							/* R/W: states for each output line */
	CPUINFO_INT_OUTPUT_STATE_LAST = CPUINFO_INT_OUTPUT_STATE + MAX_OUTPUT_LINES - 1,
	CPUINFO_INT_REGISTER,								/* R/W: values of up to MAX_REGs registers */
	CPUINFO_INT_REGISTER_LAST = CPUINFO_INT_REGISTER + MAX_REGS - 1,

	CPUINFO_INT_CPU_SPECIFIC = 0x08000,					/* R/W: CPU-specific values start here */

	/* --- the following bits of info are returned as pointers to data or functions --- */
	CPUINFO_PTR_FIRST = 0x10000,

	CPUINFO_PTR_SET_INFO = CPUINFO_PTR_FIRST,			/* R/O: void (*set_info)(UINT32 state, INT64 data, void *ptr) */
	CPUINFO_PTR_GET_CONTEXT,							/* R/O: void (*get_context)(void *buffer) */
	CPUINFO_PTR_SET_CONTEXT,							/* R/O: void (*set_context)(void *buffer) */
	CPUINFO_PTR_INIT,									/* R/O: void (*init)(void) */
	CPUINFO_PTR_RESET,									/* R/O: void (*reset)(void *param) */
	CPUINFO_PTR_EXIT,									/* R/O: void (*exit)(void) */
	CPUINFO_PTR_EXECUTE,								/* R/O: int (*execute)(int cycles) */
	CPUINFO_PTR_BURN,									/* R/O: void (*burn)(int cycles) */
	CPUINFO_PTR_DISASSEMBLE,							/* R/O: void (*disassemble)(char *buffer, offs_t pc) */
	CPUINFO_PTR_IRQ_CALLBACK,							/* R/W: int (*irqcallback)(int state) */
	CPUINFO_PTR_INSTRUCTION_COUNTER,					/* R/O: int *icount */
	CPUINFO_PTR_REGISTER_LAYOUT,						/* R/O: struct debug_register_layout *layout */
	CPUINFO_PTR_WINDOW_LAYOUT,							/* R/O: struct debug_window_layout *layout */
	CPUINFO_PTR_INTERNAL_MEMORY_MAP,					/* R/O: construct_map_t map */
	CPUINFO_PTR_INTERNAL_MEMORY_MAP_LAST = CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACES - 1,

	CPUINFO_PTR_CPU_SPECIFIC = 0x18000,					/* R/W: CPU-specific values start here */

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	CPUINFO_STR_FIRST = 0x20000,

	CPUINFO_STR_NAME = CPUINFO_STR_FIRST,				/* R/O: name of the CPU */
	CPUINFO_STR_CORE_FAMILY,							/* R/O: family of the CPU */
	CPUINFO_STR_CORE_VERSION,							/* R/O: version of the CPU core */
	CPUINFO_STR_CORE_FILE,								/* R/O: file containing the CPU core */
	CPUINFO_STR_CORE_CREDITS,							/* R/O: credits for the CPU core */
	CPUINFO_STR_FLAGS,									/* R/O: string representation of the main flags value */
	CPUINFO_STR_REGISTER,								/* R/O: string representation of up to MAX_REGs registers */
	CPUINFO_STR_REGISTER_LAST = CPUINFO_STR_REGISTER + MAX_REGS - 1,

	CPUINFO_STR_CPU_SPECIFIC = 0x28000					/* R/W: CPU-specific values start here */
};

union cpuinfo
{
	INT64	i;											/* generic integers */
	void *	p;											/* generic pointers */
	char *	s;											/* generic strings */

	void	(*setinfo)(UINT32 state, union cpuinfo *info);/* CPUINFO_PTR_SET_INFO */
	void	(*getcontext)(void *context);				/* CPUINFO_PTR_GET_CONTEXT */
	void	(*setcontext)(void *context);				/* CPUINFO_PTR_SET_CONTEXT */
	void	(*init)(void);								/* CPUINFO_PTR_INIT */
	void	(*reset)(void *param);						/* CPUINFO_PTR_RESET */
	void	(*exit)(void);								/* CPUINFO_PTR_EXIT */
	int		(*execute)(int cycles);						/* CPUINFO_PTR_EXECUTE */
	void	(*burn)(int cycles);						/* CPUINFO_PTR_BURN */
	offs_t	(*disassemble)(char *buffer, offs_t pc);	/* CPUINFO_PTR_DISASSEMBLE */
	int		(*irqcallback)(int state);					/* CPUINFO_PTR_IRQCALLBACK */
	int *	icount;										/* CPUINFO_PTR_INSTRUCTION_COUNTER */
//	construct_map_t internal_map;						/* CPUINFO_PTR_INTERNAL_MEMORY_MAP */
};

struct MachineCPU
{
	int cpu_type;	/* see #defines below. */
	int cpu_clock;	/* in Hertz */
	const void *memory_read;	/* struct Memory_ReadAddress */
	const void *memory_write;	/* struct Memory_WriteAddress */
	const struct IO_ReadPort *port_read;
	const struct IO_WritePort *port_write;
	int (*vblank_interrupt)(void);
    int vblank_interrupts_per_frame;    /* usually 1 */
/* use this for interrupts which are not tied to vblank 	*/
/* usually frequency in Hz, but if you need 				*/
/* greater precision you can give the period in nanoseconds */
	int (*timed_interrupt)(void);
	int timed_interrupts_per_second;
/* pointer to a parameter to pass to the CPU cores reset function */
	void *reset_param;
};

/* set this if the CPU is used as a slave for audio. It will not be emulated if */
/* sound is disabled, therefore speeding up a lot the emulation. */
#define CPU_AUDIO_CPU 0x8000

/* the Z80 can be wired to use 16 bit addressing for I/O ports */
#define CPU_16BIT_PORT 0x4000

#define CPU_FLAGS_MASK 0xff00



/* The old system is obsolete and no longer supported by the core */
#define NEW_INTERRUPT_SYSTEM    1

#define MAX_IRQ_LINES   8       /* maximum number of IRQ lines per CPU */

#define CLEAR_LINE		0		/* clear (a fired, held or pulsed) line */
#define ASSERT_LINE     1       /* assert an interrupt immediately */
#define HOLD_LINE       2       /* hold interrupt line until enable is true */
#define PULSE_LINE		3		/* pulse interrupt line for one instruction */

#define MAX_REGS		128 	/* maximum number of register of any CPU */

/* Values passed to the cpu_info function of a core to retrieve information */
enum {
	CPU_INFO_REG,
	CPU_INFO_FLAGS=MAX_REGS,
	CPU_INFO_NAME,
	CPU_INFO_FAMILY,
	CPU_INFO_VERSION,
	CPU_INFO_FILE,
	CPU_INFO_CREDITS,
	CPU_INFO_REG_LAYOUT,
	CPU_INFO_WIN_LAYOUT
};

#define CPU_IS_LE		0	/* emulated CPU is little endian */
#define CPU_IS_BE		1	/* emulated CPU is big endian */

/*
 * This value is passed to cpu_get_reg to retrieve the previous
 * program counter value, ie. before a CPU emulation started
 * to fetch opcodes and arguments for the current instrution.
 */
#define REG_PREVIOUSPC	-1

/*
 * This value is passed to cpu_get_reg/cpu_set_reg, instead of one of
 * the names from the enum a CPU core defines for it's registers,
 * to get or set the contents of the memory pointed to by a stack pointer.
 * You can specify the n'th element on the stack by (REG_SP_CONTENTS-n),
 * ie. lower negative values. The actual element size (UINT16 or UINT32)
 * depends on the CPU core.
 * This is also used to replace the cpu_geturnpc() function.
 */
#define REG_SP_CONTENTS -2

void cpu_init(void);
void cpu_run(void);

/* optional watchdog */
WRITE_HANDLER( watchdog_reset_w );
READ_HANDLER( watchdog_reset_r );
WRITE16_HANDLER( watchdog_reset16_w );
READ16_HANDLER( watchdog_reset16_r );
/* Use this function to reset the machine */
void machine_reset(void);
/* Use this function to reset a single CPU */
void cpu_set_reset_line(int cpu,int state);
/* Use this function to halt a single CPU */
void cpu_set_halt_line(int cpu,int state);

/* This function returns CPUNUM current status (running or halted) */
int cpu_getstatus(int cpunum);
int cpu_gettotalcpu(void);
int cpu_getactivecpu(void);
void cpu_setactivecpu(int cpunum);

/* Returns the current program counter */
unsigned cpu_get_pc(void);
/* Set the current program counter */
void cpu_set_pc(unsigned val);

/* Returns the current stack pointer */
unsigned cpu_get_sp(void);
/* Set the current stack pointer */
void cpu_set_sp(unsigned val);

/* Get the active CPUs context and return it's size */
unsigned cpu_get_context(void *context);
/* Set the active CPUs context */
void cpu_set_context(void *context);

/* Get a pointer to the active CPUs cycle count lookup table */
void *cpu_get_cycle_table(int which);
/* Override a pointer to the active CPUs cycle count lookup table */
void cpu_set_cycle_tbl(int which, void *new_table);

/* Returns a specific register value (mamedbg) */
unsigned cpu_get_reg(int regnum);
/* Sets a specific register value (mamedbg) */
void cpu_set_reg(int regnum, unsigned val);

/* Returns previous pc (start of opcode causing read/write) */
/* int cpu_getpreviouspc(void); */
#define cpu_getpreviouspc() 0

/* Returns the return address from the top of the stack (Z80 only) */
/* int cpu_getreturnpc(void); */
/* This can now be handled with a generic function */
#define cpu_geturnpc() cpu_get_reg(REG_SP_CONTENTS)

int cycles_currently_ran(void);
int cycles_left_to_run(void);
void cpu_set_op_base(unsigned val);

/* Returns the number of CPU cycles which take place in one video frame */
int cpu_gettotalcycles(void);
/* Returns the number of CPU cycles before the next interrupt handler call */
int cpu_geticount(void);
/* Returns the number of CPU cycles before the end of the current video frame */
int cpu_getfcount(void);
/* Returns the number of CPU cycles in one video frame */
int cpu_getfperiod(void);
/* Scales a given value by the ratio of fcount / fperiod */
int cpu_scalebyfcount(int value);
/* Returns the current scanline number */
int cpu_getscanline(void);
/* Returns the amount of time until a given scanline */
double cpu_getscanlinetime(int scanline);
/* Returns the duration of a single scanline */
double cpu_getscanlineperiod(void);
/* Returns the duration of a single scanline in cycles */
int cpu_getscanlinecycles(void);
/* Returns the number of cycles since the beginning of this frame */
int cpu_getcurrentcycles(void);
/* Returns the current horizontal beam position in pixels */
int cpu_gethorzbeampos(void);
/*
  Returns the number of times the interrupt handler will be called before
  the end of the current video frame. This is can be useful to interrupt
  handlers to synchronize their operation. If you call this from outside
  an interrupt handler, add 1 to the result, i.e. if it returns 0, it means
  that the interrupt handler will be called once.
*/
int cpu_getiloops(void);

/* Returns the current VBLANK state */
int cpu_getvblank(void);

/* Returns the number of the video frame we are currently playing */
int cpu_getcurrentframe(void);


/* generate a trigger after a specific period of time */
void cpu_triggertime (double duration, int trigger);
/* generate a trigger now */
void cpu_trigger (int trigger);

/* burn CPU cycles until a timer trigger */
void cpu_spinuntil_trigger (int trigger);
/* burn CPU cycles until the next interrupt */
void cpu_spinuntil_int (void);
/* burn CPU cycles until our timeslice is up */
void cpu_spin (void);
/* burn CPU cycles for a specific period of time */
void cpu_spinuntil_time (double duration);

/* yield our timeslice for a specific period of time */
void cpu_yielduntil_trigger (int trigger);
/* yield our timeslice until the next interrupt */
void cpu_yielduntil_int (void);
/* yield our current timeslice */
void cpu_yield (void);
/* yield our timeslice for a specific period of time */
void cpu_yielduntil_time (double duration);

/* set the NMI line state for a CPU, normally use PULSE_LINE */
void cpu_set_nmi_line(int cpunum, int state);
/* set the IRQ line state for a specific irq line of a CPU */
/* normally use state HOLD_LINE, irqline 0 for first IRQ type of a cpu */
void cpu_set_irq_line(int cpunum, int irqline, int state);
/* this is to be called by CPU cores only! */
void cpu_generate_internal_interrupt(int cpunum, int type);
/* set the vector to be returned during a CPU's interrupt acknowledge cycle */
void cpu_irq_line_vector_w(int cpunum, int irqline, int vector);

/* use this function to install a driver callback for IRQ acknowledge */
void cpu_set_irq_callback(int cpunum, int (*callback)(int));

/* use these in your write memory/port handles to set an IRQ vector */
/* offset corresponds to the irq line number here */
WRITE_HANDLER( cpu_0_irq_line_vector_w );
WRITE_HANDLER( cpu_1_irq_line_vector_w );
WRITE_HANDLER( cpu_2_irq_line_vector_w );
WRITE_HANDLER( cpu_3_irq_line_vector_w );
WRITE_HANDLER( cpu_4_irq_line_vector_w );
WRITE_HANDLER( cpu_5_irq_line_vector_w );
WRITE_HANDLER( cpu_6_irq_line_vector_w );
WRITE_HANDLER( cpu_7_irq_line_vector_w );

/* Obsolete functions: avoid to use them in new drivers if possible. */

/* cause an interrupt on a CPU */
void cpu_cause_interrupt(int cpu,int type);
void cpu_clear_pending_interrupts(int cpu);
WRITE_HANDLER( interrupt_enable_w );
WRITE_HANDLER( interrupt_vector_w );
int interrupt(void);
int nmi_interrupt(void);
#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
int m68_level1_irq(void);
int m68_level2_irq(void);
int m68_level3_irq(void);
int m68_level4_irq(void);
int m68_level5_irq(void);
int m68_level6_irq(void);
int m68_level7_irq(void);
#endif
int ignore_interrupt(void);

/* CPU context access */
void* cpu_getcontext (int _activecpu);
int cpu_is_saving_context(int _activecpu);

/***************************************************************************
 * Get information for the currently active CPU
 * cputype is a value from the CPU enum in driver.h
 ***************************************************************************/
/* Return number of address bits */
unsigned cpu_address_bits(void);
/* Return address mask */
unsigned cpu_address_mask(void);
/* Return address shift factor (TMS34010 bit addressing mode) */
int cpu_address_shift(void);
/* Return endianess of the emulated CPU (CPU_IS_LE or CPU_IS_BE) */
unsigned cpu_endianess(void);
/* Return opcode align unit (1 byte, 2 word, 4 dword) */
unsigned cpu_align_unit(void);
/* Return maximum instruction length */
unsigned cpu_max_inst_len(void);

/* Return name of the active CPU */
const char *cpu_name(void);
/* Return family name of the active CPU */
const char *cpu_core_family(void);
/* Return core version of the active CPU */
const char *cpu_core_version(void);
/* Return core filename of the active CPU */
const char *cpu_core_file(void);
/* Return credits info for of the active CPU */
const char *cpu_core_credits(void);
/* Return register layout definition for the active CPU */
const char *cpu_reg_layout(void);
/* Return (debugger) window layout definition for the active CPU */
const char *cpu_win_layout(void);

/* Disassemble an instruction at PC into the given buffer */
unsigned cpu_dasm(char *buffer, unsigned pc);
/* Return a string describing the currently set flag (status) bits of the active CPU */
const char *cpu_flags(void);
/* Return a string with a register name and hex value for the active CPU */
/* regnum is a value defined in the CPU cores header files */
const char *cpu_dump_reg(int regnum);
/* Return a string describing the active CPUs current state */
const char *cpu_dump_state(void);

/***************************************************************************
 * Get information for a specific CPU type
 * cputype is a value from the CPU enum in driver.h
 ***************************************************************************/
/* Return address shift factor */
/* TMS320C10 -1: word addressing mode, TMS34010 3: bit addressing mode */
int cputype_address_shift(int cputype);
/* Return number of address bits */
unsigned cputype_address_bits(int cputype);
/* Return address mask */
unsigned cputype_address_mask(int cputype);
/* Return endianess of the emulated CPU (CPU_IS_LE or CPU_IS_BE) */
unsigned cputype_endianess(int cputype);
/* Return data bus width of the emulated CPU */
unsigned cputype_databus_width(int cputype);
/* Return opcode align unit (1 byte, 2 word, 4 dword) */
unsigned cputype_align_unit(int cputype);
/* Return maximum instruction length */
unsigned cputype_max_inst_len(int cputype);

/* Return name of the CPU */
const char *cputype_name(int cputype);
/* Return family name of the CPU */
const char *cputype_core_family(int cputype);
/* Return core version number of the CPU */
const char *cputype_core_version(int cputype);
/* Return core filename of the CPU */
const char *cputype_core_file(int cputype);
/* Return credits for the CPU core */
const char *cputype_core_credits(int cputype);
/* Return register layout definition for the CPU core */
const char *cputype_reg_layout(int cputype);
/* Return (debugger) window layout definition for the CPU core */
const char *cputype_win_layout(int cputype);

/***************************************************************************
 * Get (or set) information for a numbered CPU of the running machine
 * cpunum is a value between 0 and cpu_gettotalcpu() - 1
 ***************************************************************************/
/* Return number of address bits */
unsigned cpunum_address_bits(int cpunum);
/* Return address mask */
unsigned cpunum_address_mask(int cpunum);
/* Return endianess of the emulated CPU (CPU_LSB_FIRST or CPU_MSB_FIRST) */
unsigned cpunum_endianess(int cpunum);
/* Return data bus width of the emulated CPU */
unsigned cpunum_databus_width(int cpunum);
/* Return opcode align unit (1 byte, 2 word, 4 dword) */
unsigned cpunum_align_unit(int cpunum);
/* Return maximum instruction length */
unsigned cpunum_max_inst_len(int cpunum);

/* Get a register value for the specified CPU number of the running machine */
unsigned cpunum_get_reg(int cpunum, int regnum);
/* Set a register value for the specified CPU number of the running machine */
void cpunum_set_reg(int cpunum, int regnum, unsigned val);

/* Return (debugger) register layout definition for the CPU core */
const char *cpunum_reg_layout(int cpunum);
/* Return (debugger) window layout definition for the CPU core */
const char *cpunum_win_layout(int cpunum);

unsigned cpunum_dasm(int cpunum,char *buffer,unsigned pc);
/* Return a string describing the currently set flag (status) bits of the CPU */
const char *cpunum_flags(int cpunum);
/* Return a string with a register name and value */
/* regnum is a value defined in the CPU cores header files */
const char *cpunum_dump_reg(int cpunum, int regnum);
/* Return a string describing the CPUs current state */
const char *cpunum_dump_state(int cpunum);
/* Return a name for the specified cpu number */
const char *cpunum_name(int cpunum);
/* Return a family name for the specified cpu number */
const char *cpunum_core_family(int cpunum);
/* Return a version for the specified cpu number */
const char *cpunum_core_version(int cpunum);
/* Return a the source filename for the specified cpu number */
const char *cpunum_core_file(int cpunum);
/* Return a the credits for the specified cpu number */
const char *cpunum_core_credits(int cpunum);

/* Dump all of the running machines CPUs state to stderr */
void cpu_dump_states(void);

/* daisy-chain link */
typedef struct {
	void (*reset)(int);             /* reset callback     */
	int  (*interrupt_entry)(int);   /* entry callback     */
	void (*interrupt_reti)(int);    /* reti callback      */
	int irq_param;                  /* callback paramater */
}	Z80_DaisyChain;

#define Z80_MAXDAISY	4		/* maximum of daisy chan device */

#define Z80_INT_REQ     0x01    /* interrupt request mask       */
#define Z80_INT_IEO     0x02    /* interrupt disable mask(IEO)  */

#define Z80_VECTOR(device,state) (((device)<<8)|(state))

void z80_set_irqvec(int vector);

extern unsigned int m6809_read(unsigned int address);
extern void m6809_write(unsigned int address, unsigned int data);
extern unsigned int m6502_read(unsigned int address);
extern unsigned int m6502_readop(unsigned int address);
extern void m6502_write(unsigned int address, unsigned int data);
extern unsigned int h6280_read(unsigned int address);
extern void h6280_write(unsigned int address, unsigned int data);
extern void h6280_writeport(unsigned int address, unsigned int data);
extern unsigned int m6800_read(unsigned int address);
extern void m6800_write(unsigned int address, unsigned int data);
extern unsigned int m6800_readport(unsigned int address);
extern void m6800_writeport(unsigned int address, unsigned int data);
extern unsigned int nec_readmem(unsigned int address);
extern unsigned int nec_readop(unsigned int address);
extern void nec_writemem(unsigned int address, unsigned int data);
extern unsigned int nec_readport(unsigned int address);
extern void nec_writeport(unsigned int address, unsigned int data);
extern unsigned int adsp_read8(unsigned int address);
extern unsigned int adsp_read16(unsigned int address);
extern unsigned int adsp_read32(unsigned int address);
extern void adsp_write8(unsigned int address, unsigned int data);
extern void adsp_write16(unsigned int address, unsigned int data);
extern void adsp_write32(unsigned int address, unsigned int data);
extern unsigned int z80_readmem(unsigned int address);
extern unsigned int z80_readop(unsigned int address);
extern void z80_writemem(unsigned int address, unsigned int data);
extern unsigned int z80_readport(unsigned int address);
extern void z80_writeport(unsigned int address, unsigned int data);
extern unsigned int m37710_read(unsigned int address);
extern void m37710_write(unsigned int address, unsigned int data);
extern unsigned int m37710_read16(unsigned int address);
extern void m37710_write16(unsigned int address, unsigned int data);
unsigned int tms32010_readport(unsigned int port);
void tms32010_writeport(unsigned int port, unsigned int data);
unsigned int tms32010_read16(unsigned int address);
void tms32010_write16(unsigned int port, unsigned int data);
unsigned int tms32031_read32(unsigned int address);
void tms32031_write32(unsigned int address, unsigned int data);

#ifdef __cplusplus
extern "C" void logerror(char *fmt,...);
#else
extern void logerror(char *fmt,...);
#endif

// cpus we know
enum
{
	CPU_DUMMY,
	CPU_HU6280,
	CPU_M6502,
	CPU_M6803,
	CPU_M6808,
	CPU_HD63701,
	CPU_M6809,
	CPU_HD6309,
	CPU_I8039,
	CPU_N7751,
	CPU_MC68000,
	CPU_V30,
	CPU_ADSP2104,
	CPU_ADSP2105,
	CPU_ADSP2115,
	CPU_Z80C,
	CPU_M37710,
	CPU_H8_3002,
	CPU_M6809B, 	// (managed memory)
	CPU_M6502B,	// (managed memory)
	CPU_TMS32031,	// TMS32031 DSP
	CPU_M65C02,	// 65c02
	CPU_8085A,

	CPU_COUNT,
};

/* return the value of a register on the active CPU */
unsigned activecpu_get_reg(int regnum);

#define		activecpu_get_pc()			activecpu_get_reg(REG_PC)
#define		activecpu_get_previouspc()	activecpu_get_reg(REG_PREVIOUSPC)

/* circular string buffer */
char *cpuintrf_temp_str(void);

#ifdef __cplusplus
}
#endif

#endif	/* CPUINTRF_H */
