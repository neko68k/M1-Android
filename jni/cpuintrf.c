/* CPU interface routines */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "m1snd.h"
#include "m1ui.h"
#include "rom.h"
#include "oss.h"
#include "wavelog.h"
#include "filter.h"

static M168KT *rw68k;
static M16800T *rw6800;
static M16809T *rw6809;
static M16502T *rw6502;
static M16280T *rw6280;
static M1NECT  *rwnec;
static M1ADSPT *rwadsp;
static M137710T *rw37710;
static M1H83K2T *rwh83002;
static M132031T *rwtms32031;

static M16809T m6809b =
{
	(unsigned int(*)(unsigned int))memory_read8,
	(void(*)(unsigned int, unsigned int))memory_write8
};

static M16502T m6502b =
{
	(unsigned int(*)(unsigned int))memory_read8,
	(unsigned int(*)(unsigned int))memory_readop8,
	(void(*)(unsigned int, unsigned int))memory_write8
};

unsigned char *adsp_rom;

static int nec_vector, z80_vector;

// dummy set context for CPU cores not supporting it yet
static void dummy_setctx(void *ctx)
{
}

static void dummy_getctx(void *ctx)
{
}

// TMS32031 memory handlers

unsigned int tms32031_read32(unsigned int address)
{
	return rwtms32031->read32(address);
}

void tms32031_write32(unsigned int address, unsigned int data)
{
	rwtms32031->write32(address, data);
}

// --- MUSASHI 68K HANDLERS ---

unsigned int m68k_read_memory_8(unsigned int address)
{
	return(rw68k->read8(address));
}

unsigned int m68k_read_memory_16(unsigned int address)
{
	return(rw68k->read16(address));
}

unsigned int m68k_read_memory_32(unsigned int address)
{
	return(rw68k->read32(address));
}

void m68k_write_memory_8(unsigned int address, unsigned int data)
{
	rw68k->write8(address, data);
}

void m68k_write_memory_16(unsigned int address, unsigned int data)
{
	rw68k->write16(address, data);
}

void m68k_write_memory_32(unsigned int address, unsigned int data)
{
	rw68k->write32(address, data);
}

// M6809 handlers

unsigned int m6809_read(unsigned int address)
{
	address &= 0xffff;
	return rw6809->read(address);
}

void m6809_write(unsigned int address, unsigned int data)
{
	data &= 0xff;
	address &= 0xffff;
	rw6809->write(address, data);
}

int m6809_irq_callback(int irqline)
{
	if (irqline == M6809_FIRQ_LINE)
	{
		m6809_set_irq_line(M6809_FIRQ_LINE, CLEAR_LINE);
	}

	if (irqline == M6809_IRQ_LINE)
	{
		m6809_set_irq_line(M6809_IRQ_LINE, CLEAR_LINE);
	}

	return 0;
}

// HD6309 handlers

int hd6309_irq_callback(int irqline)
{
	if (irqline == HD6309_FIRQ_LINE)
	{
		hd6309_set_irq_line(HD6309_FIRQ_LINE, CLEAR_LINE);
	}

	if (irqline == M6809_IRQ_LINE)
	{
		hd6309_set_irq_line(HD6309_IRQ_LINE, CLEAR_LINE);
	}

	return 0;
}

// M6502 handlers

unsigned int m6502_read(unsigned int address)
{
	return rw6502->read(address);
}

unsigned int m6502_readop(unsigned int address)
{
	return rw6502->readop(address);
}

void m6502_write(unsigned int address, unsigned int data)
{
	rw6502->write(address, data);
}

int m6502_irq_callback(int irqline)
{
	m6502_set_irq_line(irqline, CLEAR_LINE);

	return 0;
}

// Hu6280 handlers

static int h6280_irq_callback(int irqline)
{
//	printf("6280 irq callback: line %d\n", irqline);
	h6280_set_irq_line(irqline, CLEAR_LINE);
	return 0;
}

unsigned int h6280_read(unsigned int address)
{
	return rw6280->read(address);
}

void h6280_write(unsigned int address, unsigned int data)
{
	rw6280->write(address, data);
}

void h6280_writeport(unsigned int address, unsigned int data)
{
	rw6280->writeport(address, data);
}

// M6800 handlers

unsigned int m6800_read(unsigned int address)
{
	return rw6800->read(address);
}

void m6800_write(unsigned int address, unsigned int data)
{
	rw6800->write(address, data);
}

unsigned int m6800_readport(unsigned int address)
{
	return rw6800->readport(address);
}

void m6800_writeport(unsigned int address, unsigned int data)
{
	rw6800->writeport(address, data);
}

int m6800_irq_callback(int irqline)
{
	return 0;
}

// NEC V20/V30/V33 handlers

unsigned int nec_readmem(unsigned int address)
{
	return rwnec->read(address);
}

unsigned int nec_readop(unsigned int address)
{
	return rwnec->readop(address);
}

void nec_writemem(unsigned int address, unsigned int data)
{
	rwnec->write(address, data);
}

unsigned int nec_readport(unsigned int address)
{
	return rwnec->readport(address);
}

void nec_writeport(unsigned int address, unsigned int data)
{
	rwnec->writeport(address, data);
}

int nec_irq_callback(int irqline)
{
	return nec_vector;
}

void nec_set_irqvec(int vector)
{
	nec_vector = vector;
}

// ADSP 2105 memory handlers

unsigned int adsp_read8(unsigned int address)
{
	return(rwadsp->read8(address));
}

unsigned int adsp_read16(unsigned int address)
{
	return(rwadsp->read16(address));
}

unsigned int adsp_read32(unsigned int address)
{
	return(rwadsp->read32(address));
}

void adsp_write8(unsigned int address, unsigned int data)
{
	rwadsp->write8(address, data);
}

void adsp_write16(unsigned int address, unsigned int data)
{
	rwadsp->write16(address, data);
}

void adsp_write32(unsigned int address, unsigned int data)
{
	rwadsp->write32(address, data);
}

int adsp2100_irq_callback(int irqline)
{
//	adsp2105_set_irq_line(irqline, CLEAR_LINE);
	return 0;
}

// H8/3002 memory handlers

UINT8 h8_mem_read8(unsigned int address)
{
	return(rwh83002->read8(address));
}

UINT8 h8_io_read8(unsigned int address)
{
	if (!rwh83002->ioread8) return 0;
	return(rwh83002->ioread8(address));
}

UINT16 h8_mem_read16(unsigned int address)
{
	return(rwh83002->read16(address));
}

void h8_mem_write8(unsigned int address, UINT8 data)
{
	rwh83002->write8(address, data);
}

void h8_io_write8(unsigned int address, UINT8 data)
{
	if (!rwh83002->iowrite8) return;
	rwh83002->iowrite8(address, data);
}

void h8_mem_write16(unsigned int address, UINT16 data)
{
	rwh83002->write16(address, data);
}

// MAME Z80 handlers

int z80_irq_callback(int irqline)
{
	return z80_vector;
}

void z80_set_irqvec(int vector)
{
	z80_vector = vector;
}

/* M37710 stuff */
unsigned int m37710_read(unsigned int address)
{
	return rw37710->read(address);
}

void m37710_write(unsigned int address, unsigned int data)
{
	rw37710->write(address, data);
}

unsigned int m37710_read16(unsigned int address)
{
	return rw37710->read16(address);
}

void m37710_write16(unsigned int address, unsigned int data)
{
	rw37710->write16(address, data);
}

/* TMS32010 stuff */
unsigned int tms32010_readport(unsigned int port)
{
	return 0;
}

void tms32010_writeport(unsigned int port, unsigned int data)
{
}

unsigned int tms32010_read16(unsigned int address) 
{
	return 0;
}

void tms32010_write16(unsigned int port, unsigned int data)
{
}

// add a 6803 cpu
void m1snd_add6803(long clock, void *handlers)
{
	rw6800 = handlers;

	timer_add_cpu(CPU_M6803, clock, m6803_execute, m6803_getcycles, m6803_yield, dummy_getctx, dummy_setctx, NULL);

	m6803_init();
	m6803_reset(NULL);
	m6803_set_irq_callback(m6800_irq_callback);
}

// add a 6808 cpu
void m1snd_add6808(long clock, void *handlers)
{
	rw6800 = handlers;

	timer_add_cpu(CPU_M6808, clock, m6808_execute, m6803_getcycles, m6803_yield, dummy_getctx, dummy_setctx, NULL);

	m6808_init();
	m6808_reset(NULL);
	m6803_set_irq_callback(m6800_irq_callback);
}

// add a hd63701 mcu
void m1snd_add63701(long clock, void *handlers)
{
	rw6800 = handlers;

	timer_add_cpu(CPU_HD63701, clock, hd63701_execute, hd63701_getcycles, hd63701_yield, dummy_getctx, dummy_setctx, NULL);

	hd63701_init();
	hd63701_reset(NULL);
	hd63701_set_irq_callback(m6800_irq_callback);
}

// Z80c (MAME C core) context management functions
static void m6809_getctx(void *ctx)
{
	M16809T **ourctx;

	ourctx = (M16809T **)ctx;

	*ourctx++ = rw6809;

	m6809_get_context(ourctx);
}

static void m6809_setctx(void *ctx)
{
	M16809T **ourctx;

	ourctx = (M16809T **)ctx;

	rw6809 = *ourctx++;

	m6809_set_context(ourctx);
}

static void m6502_getctx(void *ctx)
{
	M16502T **ourctx;

	ourctx = (M16502T **)ctx;

	*ourctx++ = rw6502;

	m6502_get_context(ourctx);
}

static void m6502_setctx(void *ctx)
{
	M16502T **ourctx;

	ourctx = (M16502T **)ctx;

	rw6502 = *ourctx++;

	m6502_set_context(ourctx);
}

// do a z80c irq in a context-safe way
void m1snd_setm6809irq(int cpunum, int irqline, int state)
{
	int curcpu;

	curcpu = timer_get_cur_cpu();

	// optimize the case where we call this inside the current cpu's memory handlers
	if (curcpu == cpunum)
	{
		m6809_set_irq_line(irqline, state);
	}
	else
	{
		if (curcpu != -1)
		{
			timer_save_cpu(curcpu);
		}

		timer_restore_cpu(cpunum);
		m6809_set_irq_line(irqline, state);
		timer_save_cpu(cpunum);

		if (curcpu != -1)
		{
			timer_restore_cpu(curcpu);	
		}
	}
}

// add a 6809 cpu
void m1snd_add6809(long clock, void *handlers)
{
	char *ctx;
	int cpunum;

	rw6809 = handlers;

	ctx = malloc(4*1024);
	cpunum = timer_add_cpu(CPU_M6809, clock, m6809_execute, m6809_getcycles, m6809_yield, m6809_getctx, m6809_setctx, ctx);

//	printf("Adding 6809 CPU #%d, ctx=%x\n", cpunum, (unsigned int)ctx);

	m6809_reset(NULL);
	m6809_set_irq_callback(m6809_irq_callback);

	m6809_getctx(ctx);
}

// 6809, managed via memory manager
void m1snd_add6809b(long clock, void *handlers)
{
	char *ctx;
	int cpunum;

	rw6809 = &m6809b;

	ctx = malloc(4*1024);
	cpunum = timer_add_cpu(CPU_M6809, clock, m6809_execute, m6809_getcycles, m6809_yield, m6809_getctx, m6809_setctx, ctx);

//	printf("Adding 6809b CPU #%d, ctx=%x\n", cpunum, (unsigned int)ctx);

	memory_register_cpu(cpunum, 8, M1_CPU_LE);
	memory_context_switch(cpunum);

	m6809_reset(NULL);
	m6809_set_irq_callback(m6809_irq_callback);

	m6809_getctx(ctx);
}

// add a 6309 cpu
void m1snd_add6309(long clock, void *handlers)
{
	int cpu = timer_add_cpu(CPU_HD6309, clock, hd6309_execute, hd6309_getcycles, hd6309_yield, dummy_getctx, dummy_setctx, NULL);

	memory_register_cpu(cpu, 8, M1_CPU_LE);

	hd6309_init();
	hd6309_set_irq_callback(hd6309_irq_callback);
}

// add a 6502 cpu
void m1snd_add6502(long clock, void *handlers)
{
	rw6502 = handlers;

	timer_add_cpu(CPU_M6502, clock, m6502_execute, m6502_getcycles, m6502_yield, dummy_getctx, dummy_setctx, NULL);

	m6502_init();
	m6502_reset(NULL);
	m6502_set_irq_callback(m6502_irq_callback);
}

// add a 65c02 cpu
void m1snd_add65c02(long clock, void *handlers)
{
	rw6502 = handlers;

	timer_add_cpu(CPU_M65C02, clock, m65c02_execute, m6502_getcycles, m6502_yield, dummy_getctx, dummy_setctx, NULL);

	m65c02_init();
	m65c02_reset(NULL);
	m65c02_set_irq_callback(m6502_irq_callback);
}

// add a 6502 cpu, managed via memory manager
void m1snd_add6502b(long clock, void *handlers)
{
	char *ctx;
	int cpunum;

	rw6502 = &m6502b;

	ctx = malloc(4*1024);
	cpunum = timer_add_cpu(CPU_M6502, clock, m6502_execute, m6502_getcycles, m6502_yield, m6502_getctx, m6502_setctx, ctx);

	memory_register_cpu(cpunum, 8, M1_CPU_LE);
	memory_context_switch(cpunum);

	m6502_init();
	m6502_reset(NULL);
	m6502_set_irq_callback(m6502_irq_callback);

	m6502_getctx(ctx);
}

// add a m37710 cpu
void m1snd_add37710(long clock, void *handlers)
{
	rw37710 = handlers;

	timer_add_cpu(CPU_M37710, clock, m37710_execute, m37710_getcycles, m37710_yield, dummy_getctx, dummy_setctx, NULL);

	m37710_init();
	m37710_reset(NULL);
}

// add a 6280 cpu
void m1snd_add6280(long clock, void *handlers)
{
	rw6280 = handlers;

	timer_add_cpu(CPU_HU6280, clock, h6280_execute, h6280_getcycles, h6280_yield, dummy_getctx, dummy_setctx, NULL);

	h6280_init();
	h6280_reset(NULL);
	h6280_set_irq_callback(h6280_irq_callback);
}

// add a 8039 cpu
void m1snd_add8039(long clock, void *handlers)
{
	int cpunum;

	cpunum = timer_add_cpu(CPU_I8039, clock, i8039_execute, i8039_getcycles, i8039_yield, dummy_getctx, dummy_setctx, NULL);

	memory_register_cpu(cpunum, 8, M1_CPU_LE);

	i8039_reset(NULL);
}

// add an 8085A cpu
void m1snd_add8085(long clock, void *handlers)
{
	int cpunum;

	cpunum = timer_add_cpu(CPU_8085A, clock, i8085_execute, i8085_getcycles, i8085_yield, dummy_getctx, dummy_setctx, NULL);

	memory_register_cpu(cpunum, 8, M1_CPU_LE);

	i8085_reset(NULL);
}

// add a n7751 cpu
void m1snd_add7751(long clock, void *handlers)
{
	int cpunum;

	cpunum = timer_add_cpu(CPU_N7751, clock, n7751_execute, n7751_getcycles, n7751_yield, dummy_getctx, dummy_setctx, NULL);

	memory_register_cpu(cpunum, 8, M1_CPU_LE);

	n7751_reset(NULL);
}

// add a 68k cpu
void m1snd_add68k(long clock, void *handlers)
{
	rw68k = handlers;

	// $$$HACK: if this isn't called, model 1/2 games choke (IRQs don't get through)
	// if called up after Taito m68k-based games.
	m68k_init();

	timer_add_cpu(CPU_MC68000, clock, m68k_execute, m68k_cycles_run, m68k_end_timeslice, dummy_getctx, dummy_setctx, NULL);

	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();
}

// add a 32031 cpu
void m1snd_add32031(long clock, void *handlers)
{
	rwtms32031 = handlers;

	timer_add_cpu(CPU_TMS32031, clock, tms32031_execute, tms32031_getcycles, tms32031_yield, dummy_getctx, dummy_setctx, NULL);

	tms32031_init();
	tms32031_reset(NULL);
}

// add a V30 cpu
void m1snd_addv30(long clock, void *handlers)
{
	rwnec = handlers;

	timer_add_cpu(CPU_V30, clock, v30_execute, nec_getcycles, nec_yield, dummy_getctx, dummy_setctx, NULL);

	v30_init();
	v30_reset(NULL);
	v30_set_irq_callback(nec_irq_callback);
	nec_vector = 0;
}

// Z80c (MAME C core) context management functions
static void z80_getctx(void *ctx)
{
	M1Z80T **ourctx;

	ourctx = (M1Z80T **)ctx;

	#ifdef USE_Z80
	z80_get_context(ourctx);
	#endif
	#ifdef USE_DRZ80
	drz80_get_context(ourctx);
	#endif
}

static void z80_setctx(void *ctx)
{
	M1Z80T **ourctx;

	ourctx = (M1Z80T **)ctx;

	#ifdef USE_Z80
	z80_set_context(ourctx);
	#endif
	#ifdef USE_DRZ80
	drz80_set_context(ourctx);
	#endif
}

// add a Z80 cpu (MAME core)
#ifdef USE_Z80
void m1snd_addz80(long clock, void *handlers)
{
	char *z80cctx;
	int cpunum;

	z80cctx = malloc(4*1024);
	cpunum = timer_add_cpu(CPU_Z80C, clock, z80_execute, z80_getcycles, z80_yield, z80_getctx, z80_setctx, z80cctx);

	memory_register_cpu(cpunum, 8, M1_CPU_LE);

//	printf("adding Z80 #%d\n", cpunum);

	z80_init();
	z80_reset(NULL);
	z80_set_irq_callback(z80_irq_callback);
	z80_vector = 0xff;

	z80_getctx(z80cctx);
}
#endif
#ifdef USE_DRZ80
void m1snd_addz80(long clock, void *handlers)
{
	char *z80cctx;
	int cpunum;

	z80cctx = malloc(8*1024);
	cpunum = timer_add_cpu(CPU_Z80C, clock, drz80_execute, drz80_getcycles, drz80_yield, z80_getctx, z80_setctx, z80cctx);

	memory_register_cpu(cpunum, 8, M1_CPU_LE);

//	printf("adding Z80 #%d\n", cpunum);

	//z80_init();
	drz80_reset(NULL);
	drz80_set_irq_callback(z80_irq_callback);
	z80_vector = 0xff;

	z80_getctx(z80cctx);
}
#endif

// add an ADSP 2104 CPU
extern int adsp2100_getcycles(void);
extern void adsp2100_yield(void);
extern int adsp2100_execute(int cycles);
extern void adsp2100_init(void);
extern void adsp2101_reset(void *param);
extern void adsp2104_reset(void *param);
extern void adsp2105_reset(void *param);
extern void adsp2115_reset(void *param);
extern void adsp2104_set_irq_line(int irqline, int state);
extern void adsp2104_set_info(UINT32 state, union cpuinfo *info);

static union cpuinfo info;
static void adsp2104_set_irq_callback(int (*callback)(int irqline))
{
	info.irqcallback = callback;
	adsp2104_set_info(CPUINFO_PTR_IRQ_CALLBACK, &info);
}

void m1snd_addadsp2104(long clock, void *handlers)
{
	rwadsp = handlers;

	timer_add_cpu(CPU_ADSP2104, clock, adsp2100_execute, adsp2100_getcycles, adsp2100_yield, dummy_getctx, dummy_setctx, NULL);

	adsp2100_init();
	adsp2104_reset(NULL);
	adsp2104_set_irq_callback(adsp2100_irq_callback);

	adsp_rom = rwadsp->op_rom;
}

// add an ADSP 2105 CPU
void m1snd_addadsp2105(long clock, void *handlers)
{
	rwadsp = handlers;

	timer_add_cpu(CPU_ADSP2105, clock, adsp2100_execute, adsp2100_getcycles, adsp2100_yield, dummy_getctx, dummy_setctx, NULL);

	adsp2100_init();
	adsp2105_reset(NULL);
	adsp2104_set_irq_callback(adsp2100_irq_callback);

	adsp_rom = rwadsp->op_rom;
}

// add an ADSP 2115 CPU
void m1snd_addadsp2115(long clock, void *handlers)
{
	rwadsp = handlers;

	timer_add_cpu(CPU_ADSP2115, clock, adsp2100_execute, adsp2100_getcycles, adsp2100_yield, dummy_getctx, dummy_setctx, NULL);

	adsp2100_init();
	adsp2115_reset(NULL);
	adsp2104_set_irq_callback(adsp2100_irq_callback);

	adsp_rom = rwadsp->op_rom;
}

void m1snd_addh83002(long clock, void *handlers)
{
	rwh83002 = handlers;

	timer_add_cpu(CPU_H8_3002, clock, h8_execute, h8_getcycles, h8_yield, dummy_getctx, dummy_setctx, NULL);

	h8_init();
	h8_reset(NULL);
}

unsigned cpu_get_pc(void)
{
	return 0;
}

int cpu_getactivecpu(void)
{
	return 0;
}

static void m1snd_adddummy(long clock, void *handlers)
{
}

struct cpu_interface 
{
	char *name;
	void (*addroutine)(long, void *);
	void (*reset)(void *);
	void (*set_irq)(int, int);
	void (*set_irq_callback)(int (*)(int));
	int  (*callback_handler)(int);
};

struct cpu_interface CPUIntrf[] =
{
	{
		"Dummy",
		m1snd_adddummy,
		NULL,
		NULL,
		NULL,
		NULL,
	},
	{
		"Hu6280",
		m1snd_add6280,
		h6280_reset,
		h6280_set_irq_line,
		h6280_set_irq_callback,
		h6280_irq_callback,
	},
	{
		"M6502",
		m1snd_add6502,
		m6502_reset,
		m6502_set_irq_line,
		m6502_set_irq_callback,
		m6502_irq_callback,
	},
	{
		"M6803",
		m1snd_add6803,
		m6803_reset,
		m6803_set_irq_line,
		m6803_set_irq_callback,
		NULL,
	},
	{
		"M6808",
		m1snd_add6808,
		m6808_reset,
		m6803_set_irq_line,
		m6803_set_irq_callback,
		NULL,
	},
	{
		"HD63701",
		m1snd_add63701,
		hd63701_reset,
		hd63701_set_irq_line,
		hd63701_set_irq_callback,
		NULL,
	},
	{
		"M6809",
		m1snd_add6809,
		m6809_reset,
		m6809_set_irq_line,
		m6809_set_irq_callback,
		m6809_irq_callback,
	},
	{
		"HD6309",
		m1snd_add6309,
		hd6309_reset,
		hd6309_set_irq_line,
		hd6309_set_irq_callback,
		hd6309_irq_callback,
	},
	{
		"i8039",
		m1snd_add8039,
		i8039_reset,
		i8039_set_irq_line,
		i8039_set_irq_callback,
		NULL,
	},
	{
		"N7751",
		m1snd_add7751,
		n7751_reset,
		n7751_set_irq_line,
		n7751_set_irq_callback,
		NULL,
	},
	{
		"MC68000",
		m1snd_add68k,
		NULL,
		NULL,
		NULL,
		NULL,
	},
	{
		"V30",
		m1snd_addv30,
		v30_reset,
		v30_set_irq_line,
		v30_set_irq_callback,
		nec_irq_callback,
	},
	{
		"ADSP-2104",
		m1snd_addadsp2104,
		adsp2104_reset,
		adsp2104_set_irq_line,
		adsp2104_set_irq_callback,
		NULL,
	},
	{
		"ADSP-2105",
		m1snd_addadsp2105,
		adsp2105_reset,
		adsp2104_set_irq_line,
		adsp2104_set_irq_callback,
		NULL,
	},
	{
		"ADSP-2115",
		m1snd_addadsp2115,
		adsp2115_reset,
		adsp2104_set_irq_line,
		adsp2104_set_irq_callback,
		NULL,
	},
	{
		"Z80",
		m1snd_addz80,
#ifdef USE_Z80
		z80_reset,
		z80_set_irq_line,
		z80_set_irq_callback,
#endif
#ifdef USE_DRZ80
		drz80_reset,
		drz80_set_irq_line,
		drz80_set_irq_callback,
#endif
		z80_irq_callback,
	},
	{
		"M37710",
		m1snd_add37710,
		NULL,
		m37710_set_irq_line,
		NULL,
		NULL,
	},
	{
		"H8/3002",
		m1snd_addh83002,
		NULL,
		NULL,
		NULL,
		NULL,
	},
	{
		"M6809 (managed)",
		m1snd_add6809b,
		m6809_reset,
		m6809_set_irq_line,
		m6809_set_irq_callback,
		m6809_irq_callback,
	},
	{
		"M6502 (managed)",
		m1snd_add6502b,
		m6502_reset,
		m6502_set_irq_line,
		m6502_set_irq_callback,
		m6502_irq_callback,
	},
	{
		"TMS32031",	// temp placeholder
		m1snd_add32031,
		tms32031_reset,
		tms32031_set_irq_line,
		tms32031_set_irq_callback,
		NULL
	},
	{
		"M65C02",
		m1snd_add65c02,
		m65c02_reset,
		m65c02_set_irq_line,
		m65c02_set_irq_callback,
		m6502_irq_callback,
	},
	{
		"8085A",
		m1snd_add8085,
		i8085_reset,
		i8085_set_irq_line,
		i8085_set_irq_callback,
		NULL,
	},
};

// boot all CPUs
void cpuintrf_boot(void)
{
	int i, type;

	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		if (Machine->drv->cpu[i].CPU_type != 0)
		{
			type = Machine->drv->cpu[i].CPU_type;

			CPUIntrf[type].addroutine(Machine->drv->cpu[i].speed, Machine->drv->cpu[i].memhandler);

//			printf("Registered CPU type %d (%s at %f MHz)\n", type, CPUIntrf[type].name, (double)Machine->drv->cpu[i].speed / 1000000.0);
		}
	}
}

void cpuintrf_reset(void)
{
	int i, type;

	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		if (Machine->drv->cpu[i].CPU_type != 0)
		{
			type = Machine->drv->cpu[i].CPU_type;

			if (CPUIntrf[type].reset)
			{
				memory_context_switch(i);
				CPUIntrf[type].reset(NULL);
			}

			if ((CPUIntrf[type].set_irq_callback) && (CPUIntrf[type].callback_handler))
			{
				CPUIntrf[type].set_irq_callback(CPUIntrf[type].callback_handler);
			}
		}
	}
}

unsigned activecpu_get_reg(int regnum)
{
	return 0;
}

void cpu_set_irq_line(int cpunum, int irqline, int state)
{
	int curcpu, type;

	curcpu = timer_get_cur_cpu();
	type = timer_get_cpu_type(cpunum);

	// optimize the case where we call this inside the current cpu's memory handlers
	if (curcpu == cpunum)
	{
		if (CPUIntrf[type].set_irq)
		{
			CPUIntrf[type].set_irq(irqline, state);
		}
	}
	else
	{
		if (curcpu != -1)
		{
			timer_save_cpu(curcpu);
		}

		timer_restore_cpu(cpunum);
		if (CPUIntrf[type].set_irq)
		{
			CPUIntrf[type].set_irq(irqline, state);
		}
		timer_save_cpu(cpunum);

		if (curcpu != -1)
		{
			timer_restore_cpu(curcpu);	
		}
	}
}

void cpu_set_reset_line(int cpunum, int state)
{
	int curcpu, type;

	if (state == CLEAR_LINE)
	{
		return;
	}

	curcpu = timer_get_cur_cpu();
	type = timer_get_cpu_type(cpunum);

	// optimize the case where we call this inside the current cpu's memory handlers
	if (curcpu == cpunum)
	{
		if (CPUIntrf[type].reset)
		{
			CPUIntrf[type].reset(NULL);
		}

		if ((CPUIntrf[type].set_irq_callback) && (CPUIntrf[type].callback_handler))
		{
			CPUIntrf[type].set_irq_callback(CPUIntrf[type].callback_handler);
		}
	}
	else
	{
		if (curcpu != -1)
		{
			timer_save_cpu(curcpu);
		}

		timer_restore_cpu(cpunum);
		if (CPUIntrf[type].reset)
		{
			CPUIntrf[type].reset(NULL);
		}

		if ((CPUIntrf[type].set_irq_callback) && (CPUIntrf[type].callback_handler))
		{
			CPUIntrf[type].set_irq_callback(CPUIntrf[type].callback_handler);
		}
		timer_save_cpu(cpunum);

		if (curcpu != -1)
		{
			timer_restore_cpu(curcpu);	
		}
	}
}

