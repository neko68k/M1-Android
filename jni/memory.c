/* 

 M1 CPU memory handler system
  
 As with most of our core components, it's at least superficially MAME compatible 
 but doesn't use MAME's actual code. 

 This wouldn't be much of a learning experience otherwise ;-)
 
 */

#include <assert.h>
#include "m1snd.h"

// masks passed to memory handlers for various widths
//static int byte_masks[4] = { 0x00ffffff, 0xff00ffff, 0xffff00ff, 0xffffff00 };
//static int word_masks[2] = { 0x0000ffff, 0xffff0000 };

#define MEMORY_MARKER ((offs_t)~0)
#define IS_MEMORY_MARKER( ma ) ((ma)->start == MEMORY_MARKER && (ma)->end < MEMORY_MARKER)
#define IS_MEMORY_END( ma ) ((ma)->start == MEMORY_MARKER && (ma)->end == 0)

// definition of a memory zone
typedef struct M1ZoneT
{
	offs_t start;			// start address, shifted as necessary
	offs_t end;			// end address, shifted as necessary
	void *location;			// ROM pointer for ROM regions, RAM pointer for RAM regions
	void *handler;			// handler

	struct M1ZoneT *next;	  	// next entry in chain
} M1ZoneT;

// definition of a RAM region
typedef struct M1MemT
{
	offs_t start, end;
	void *location;

	struct M1MemT *next;
} M1MemT;

// definition for per-CPU data
typedef struct
{
	int num;			// CPU number
	int bits;			// CPU # of bits
	int endian;			// CPU endianness
	int flags;			// flags
	void *prgrom;			// ROM base (memory region for ROM_CPUx)

	M1MemT *ramchain;		// RAM allocations chain

	M1ZoneT *readchain;		// read handler list
	M1ZoneT *opreadchain;		// opcode read handler list (for encrypted CPUs)
	M1ZoneT *writechain;		// write handler list

	M1ZoneT *ioreadchain;		// I/O read handler list
	M1ZoneT *iowritechain;		// I/O write handler list
} M1CPUMemT;

#define CPU_FLG_INUSE		(0x01)
#define CPU_FLG_HASCHAIN	(0x02)

static M1CPUMemT mem_cpu[TIMER_MAX_CPUS];
static int mem_cur_cpu;

static void *bank_offsets[MAX_BANKS+1];

#define MAX_AUTOMALLOC	(16)
static int mem_num_automallocs = 0;
static void *mem_auto_mallocs[MAX_AUTOMALLOC];

// private functions

// add a memory region to the chain
static void *add_ram_chain(M1CPUMemT *cpu, offs_t start, offs_t end)
{
	void *rgn;
	M1MemT *new, *lp;

//	logerror("add_ram_chain: RAM region at %x, end %x\n", (int)start, (int)end);

	// check if an entry already exists
	lp = cpu->ramchain;
	if (lp)
	{
		while (lp != (M1MemT *)NULL)
		{
			if (lp->start == start)
			{
				if (lp->end != end)
				{
//					logerror("ERROR: memory region at %x has mismatched ends (was %x now %x)\n", lp->start, lp->end, end);
				}

//				logerror("add_ram_chain: Region already exists\n");
				return lp->location;
			}

			lp = lp->next;
		}
	}

	// allocate the actual memory
	rgn = malloc((end-start)+1);
	memset(rgn, 0, (end-start)+1);

	// allocate the new RAM node
	new = (M1MemT *)malloc(sizeof(M1MemT));
	memset(new, 0, sizeof(M1MemT));
	new->start = start;
	new->end = end;
	new->location = rgn;
	new->next = (M1MemT *)NULL;

	// insert into the nodelist.
	lp = cpu->ramchain;
	if (lp)
	{
		// list is populated, walk the chain to find the end
		while (lp->next)
		{
			lp = lp->next;
		}

		// insert the node
		lp->next = new;
	}
	else	// list is empty
	{
		cpu->ramchain = new;
	}

	return rgn;
}

// construct a handler chain from a MEMORY_READ_START/MEMORY_WRITE_START structure
static void construct_chain_r8(M1CPUMemT *cpu, M1ZoneT **chain, void *info)
{
	struct Memory_ReadAddress *ra;
	M1ZoneT *newchain;

	ra = (struct Memory_ReadAddress *)info;	
	cpu->flags |= CPU_FLG_HASCHAIN;

	while (!IS_MEMORY_END(ra))
	{
		// it's a regular entry, do it
		if (!IS_MEMORY_MARKER(ra))
		{
			// allocate node
			newchain = (M1ZoneT *)malloc(sizeof(M1ZoneT));
			memset(newchain, 0, sizeof(M1ZoneT));

			// hook it up
			if (*chain == (M1ZoneT *)NULL)
			{
				*chain = newchain;
			}
			else
			{
				newchain->next = (*chain)->next;
				(*chain)->next = newchain;
			}

			newchain->start = ra->start;
			newchain->end = ra->end;
			newchain->handler = ra->handler;

//			logerror("construct_chain_r8: start %x end %x\n", (int)ra->start, (int)ra->end);

			// if it's a RAM region, allocate the RAM
			if (ra->handler == MRA_RAM)
			{
				newchain->location = add_ram_chain(cpu, ra->start, ra->end);
				newchain->handler = NULL;
			}
		}

		ra++;
	}
}

static void construct_chain_w8(M1CPUMemT *cpu, M1ZoneT **chain, void *info)
{
	struct Memory_WriteAddress *wa;
	M1ZoneT *newchain;

	wa = (struct Memory_WriteAddress *)info;	
	cpu->flags |= CPU_FLG_HASCHAIN;

	while (!IS_MEMORY_END(wa))
	{
		// it's a regular entry, do it
		if (!IS_MEMORY_MARKER(wa))
		{
			// allocate node
			newchain = (M1ZoneT *)malloc(sizeof(M1ZoneT));
			memset(newchain, 0, sizeof(M1ZoneT));

			// hook it up
			if (*chain == (M1ZoneT *)NULL)
			{
				*chain = newchain;
			}
			else
			{
				newchain->next = (*chain)->next;
				(*chain)->next = newchain;
			}

			newchain->start = wa->start;
			newchain->end = wa->end;
			newchain->handler = wa->handler;

//			logerror("construct_chain_w8: start %x end %x\n", (int)wa->start, (int)wa->end);

			// if it's a RAM region, allocate the RAM
			if ((wa->handler == MWA_RAM) || (wa->base))
			{
				newchain->location = add_ram_chain(cpu, wa->start, wa->end);
				if (wa->base)
				{
					*wa->base = (data8_t *)newchain->location;
					if ((size_t)wa->handler == STATIC_RAM)
					{
						newchain->handler = NULL;
					}
				}
				else
				{
					newchain->handler = NULL;
				}
			}
		}

		if (wa->size)
		{
			*wa->size = wa->end - wa->start;
		}

		wa++;
	}
}

// construct a handler chain from a MEMORY_READ_START/MEMORY_WRITE_START structure
static void construct_chain_r8port(M1CPUMemT *cpu, M1ZoneT **chain, void *info)
{
	struct IO_ReadPort *ra;
	M1ZoneT *newchain;

	ra = (struct IO_ReadPort *)info;	
	cpu->flags |= CPU_FLG_HASCHAIN;

	while (!IS_MEMORY_END(ra))
	{
		// it's a regular entry, do it
		if (!IS_MEMORY_MARKER(ra))
		{
			// allocate node
			newchain = (M1ZoneT *)malloc(sizeof(M1ZoneT));
			memset(newchain, 0, sizeof(M1ZoneT));

			// hook it up
			if (*chain == (M1ZoneT *)NULL)
			{
				*chain = newchain;
			}
			else
			{
				newchain->next = (*chain)->next;
				(*chain)->next = newchain;
			}

			newchain->start = ra->start;
			newchain->end = ra->end;
			newchain->handler = ra->handler;

//			logerror("construct_chain_r8port: start %x end %x\n", (int)ra->start, (int)ra->end);

			// if it's a RAM region, allocate the RAM
			if (ra->handler == MRA_RAM)
			{
				newchain->location = add_ram_chain(cpu, ra->start, ra->end);
				newchain->handler = NULL;
			}
		}

		ra++;
	}
}

static void construct_chain_w8port(M1CPUMemT *cpu, M1ZoneT **chain, void *info)
{
	struct IO_WritePort *wa;
	M1ZoneT *newchain;

	wa = (struct IO_WritePort *)info;	
	cpu->flags |= CPU_FLG_HASCHAIN;

	while (!IS_MEMORY_END(wa))
	{
		// it's a regular entry, do it
		if (!IS_MEMORY_MARKER(wa))
		{
			// allocate node
			newchain = (M1ZoneT *)malloc(sizeof(M1ZoneT));
			memset(newchain, 0, sizeof(M1ZoneT));

			// hook it up
			if (*chain == (M1ZoneT *)NULL)
			{
				*chain = newchain;
			}
			else
			{
				newchain->next = (*chain)->next;
				(*chain)->next = newchain;
			}

			newchain->start = wa->start;
			newchain->end = wa->end;
			newchain->handler = wa->handler;

//			logerror("construct_chain_w8port: start %x end %x\n", (int)wa->start, (int)wa->end);

			// if it's a RAM region, allocate the RAM
			if (wa->handler == MWA_RAM)
			{
				newchain->location = add_ram_chain(cpu, wa->start, wa->end);
				newchain->handler = NULL;
			}
		}

		wa++;
	}
}

// public functions

// init: starts up the memory system
int memory_boot(void)
{
	int i;

	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		mem_cpu[i].num = i;
		mem_cpu[i].bits = 0;
		mem_cpu[i].endian = M1_CPU_LE;
		mem_cpu[i].flags = 0;
		mem_cpu[i].prgrom = NULL;
		mem_cpu[i].readchain = (M1ZoneT *)NULL;
		mem_cpu[i].opreadchain = (M1ZoneT *)NULL;
		mem_cpu[i].writechain = (M1ZoneT *)NULL;
		mem_cpu[i].ioreadchain = (M1ZoneT *)NULL;
		mem_cpu[i].iowritechain = (M1ZoneT *)NULL;
		mem_cpu[i].ramchain = (M1MemT *)NULL;
	}
	
	mem_cur_cpu = -1;

	if (mem_num_automallocs > 0)
	{
		logerror("Possible leak: booting with num_automallocs = %d\n", mem_num_automallocs);
	}

	mem_num_automallocs = 0;
	
	return 0;
}

// unlink and deallocate the handler chains
void destroy_chain(M1ZoneT *head)
{
	M1ZoneT *cur, *nxt;

	if (head != (M1ZoneT *)NULL)
	{
		cur = head;
		while (cur != (M1ZoneT *)NULL)
		{
			nxt = cur->next;
			free((void *)cur);
			cur = nxt;
		}
	}
}

// destroy the ram chain and free all RAM
void destroy_mem_chain(M1MemT *head)
{
	M1MemT *cur, *nxt;

	if (head != (M1MemT *)NULL)
	{
		cur = head;
		while (cur != (M1MemT *)NULL)
		{
			nxt = cur->next;
			free(cur->location);
			free((void *)cur);
			cur = nxt;
		}
	}
}

// shuts down the memory system and frees all memory
void memory_shutdown(void)
{
	int i;

	for (i = 0; i < TIMER_MAX_CPUS; i++)
	{
		if (mem_cpu[i].flags & CPU_FLG_HASCHAIN)
		{
			destroy_chain(mem_cpu[i].readchain);
			destroy_chain(mem_cpu[i].opreadchain);
			destroy_chain(mem_cpu[i].writechain);
			destroy_chain(mem_cpu[i].ioreadchain);
			destroy_chain(mem_cpu[i].iowritechain);

			mem_cpu[i].readchain = (M1ZoneT *)NULL;
			mem_cpu[i].opreadchain = (M1ZoneT *)NULL; 
			mem_cpu[i].writechain = (M1ZoneT *)NULL; 
			mem_cpu[i].ioreadchain = (M1ZoneT *)NULL; 
			mem_cpu[i].iowritechain = (M1ZoneT *)NULL; 

			destroy_mem_chain(mem_cpu[i].ramchain);
			mem_cpu[i].ramchain = (M1MemT *)NULL;

			mem_cpu[i].flags = 0;
		}
	}

	for (i = 0; i < mem_num_automallocs; i++)
	{
		free(mem_auto_mallocs[i]);
	}
	mem_num_automallocs = 0;
}

void *auto_malloc(size_t size)
{
	void *rv;

	rv = malloc(size);
	mem_auto_mallocs[mem_num_automallocs++] = rv;
	if (mem_num_automallocs >= MAX_AUTOMALLOC)
	{
		logerror("Out of automallocs\n");
		exit(-1);
	}

	return rv;
}

void memory_context_switch(int num)
{
	mem_cur_cpu = num;
}

void memory_register_cpu(int num, int bits, int endian)
{
	mem_cpu[num].num = num;
	mem_cpu[num].bits = bits;
	mem_cpu[num].endian = endian;
	mem_cpu[num].prgrom = rom_getregion(RGN_CPU1 + num);
	mem_cpu[num].flags |= CPU_FLG_INUSE;
}

void memory_cpu_rw(int num, void *readmem, void *writemem)
{
	if (readmem)
		construct_chain_r8(&mem_cpu[num], &mem_cpu[num].readchain, readmem);
	if (writemem)
		construct_chain_w8(&mem_cpu[num], &mem_cpu[num].writechain, writemem);
}

void memory_cpu_readop(int num, void *opread)
{
	if (opread)
		construct_chain_r8(&mem_cpu[num], &mem_cpu[num].opreadchain, opread);
}

void memory_cpu_iorw(int num, void *ioread, void *iowrite)
{
	if (ioread)
		construct_chain_r8port(&mem_cpu[num], &mem_cpu[num].ioreadchain, ioread);
	if (iowrite)
		construct_chain_w8port(&mem_cpu[num], &mem_cpu[num].iowritechain, iowrite);
}

void cpu_setbank(int bank, void *base)
{
	bank_offsets[bank] = base;
}

// memory read/write functions (8 bit)
data8_t memory_read8(offs_t address)
{
	M1ZoneT *rc;
	offs_t offset;
	data8_t (*handler)(offs_t offset);
	data8_t *loc;

	rc = mem_cpu[mem_cur_cpu].readchain;
	while (rc != (M1ZoneT *)NULL)
	{
//		logerror("r8: Checking %x against %x-%x\n", (int)address, (int)rc->start, (int)rc->end);
		if (address >= rc->start && address <= rc->end)
		{
			offset = address - rc->start;
			if (rc->handler)
			{
				// check if it's a bank
				if ((size_t)rc->handler <= STATIC_COUNT)
				{
					int bnk = (size_t)rc->handler;

					if (bnk >= STATIC_BANK1 && bnk <= STATIC_BANKMAX)
					{
						bnk -= STATIC_BANK1;
						loc = bank_offsets[bnk+1];
						return loc[offset];
					}

					if (bnk == STATIC_ROM)
					{
						loc = mem_cpu[mem_cur_cpu].prgrom;
						if (!loc)
						{
							printf("Reading @ %x with no handler!\n", address);
							return 0;
						}
						return loc[address];
					}

					if (bnk == STATIC_NOP)
					{
						return 0;
					}
				}

				handler = rc->handler;
				return (*handler)(offset);
			}

			loc = rc->location;
			return loc[offset];
		}

		rc = rc->next;
	}

	logerror("CPU %d: Unmapped read8 at %x\n", mem_cur_cpu, (int)address);
	return 0;
}

data8_t memory_readop8(offs_t address)
{
	M1ZoneT *rc;
	offs_t offset;
	data8_t (*handler)(offs_t offset);
	data8_t *loc;

	rc = mem_cpu[mem_cur_cpu].opreadchain;
	// if no special opread chain has been set up,
	// use the normal read
	if (rc == (M1ZoneT *)NULL) 
	{
		rc = mem_cpu[mem_cur_cpu].readchain;
	}

	while (rc != (M1ZoneT *)NULL)
	{
//		logerror("Checking %x against %x-%x\n", (int)address, (int)rc->start, (int)rc->end);
		if (address >= rc->start && address <= rc->end)
		{
			offset = address - rc->start;
			if (rc->handler)
			{
				// check if it's a bank
				if ((size_t)rc->handler <= STATIC_COUNT)
				{
					int bnk = (size_t)rc->handler;

					if (bnk >= STATIC_BANK1 && bnk <= STATIC_BANKMAX)
					{
						bnk -= STATIC_BANK1;
						loc = bank_offsets[bnk+1];
						return loc[offset];
					}

					if (bnk == STATIC_ROM)
					{
						loc = mem_cpu[mem_cur_cpu].prgrom;
						return loc[address];
					}

					if (bnk == STATIC_NOP)
					{
						return 0;
					}
				}

				handler = rc->handler;
				return (*handler)(offset);
			}

			loc = rc->location;
			return loc[offset];
		}

		rc = rc->next;
	}

	logerror("CPU %d: Unmapped readop8 at %x\n", mem_cur_cpu, (int)address);
	return 0;
}

data8_t memory_readport8(offs_t port)
{
	M1ZoneT *rc;
	offs_t offset;
	data8_t (*handler)(offs_t offset);
	data8_t *loc;
	offs_t address = port;

	rc = mem_cpu[mem_cur_cpu].ioreadchain;
	if (rc != (M1ZoneT *)NULL)
	{
		if ((rc->start & 0xff00) || (rc->end & 0xff00))
		{
			address = port;
		}
		else
		{
			address = port & 0xff;
		}
	}

	while (rc != (M1ZoneT *)NULL)
	{
		if (address >= rc->start && address <= rc->end)
		{
			if ((rc->start & 0xff00) || (rc->end & 0xff00))
			{
				offset = port;
			}
			else
			{
				offset = address - rc->start;
			}
			if (rc->handler)
			{
				if (rc->handler == (void *)STATIC_NOP)
				{
					return 0;
				}

				handler = rc->handler;
				return (*handler)(offset);
			}

			loc = rc->location;
			return loc[offset];
		}

		rc = rc->next;
	}

	logerror("Unmapped readport8 at %x (%x)\n", (int)address, (int)port);
	return 0;
}

void memory_write8(offs_t address, data8_t data)
{
	M1ZoneT *rc;
	offs_t offset;
	void (*handler)(offs_t offset, data8_t data);
	data8_t *loc;

	rc = mem_cpu[mem_cur_cpu].writechain;
	while (rc != (M1ZoneT *)NULL)
	{
//		printf("W8: Check %x against (%x-%x)\n", address, rc->start, rc->end);
		if (address >= rc->start && address <= rc->end)
		{
			offset = address - rc->start;
			if (rc->location)
			{
				loc = rc->location;
				loc[offset] = data;
			}

			if (rc->handler)
			{
				// check if it's a bank
				if ((size_t)rc->handler <= STATIC_COUNT)
				{
					int bnk = (size_t)rc->handler;

					if (bnk == STATIC_NOP)
					{
						return;
					}

					if (bnk >= STATIC_BANK1 && bnk <= STATIC_BANKMAX)
					{
						bnk -= STATIC_BANK1;
						loc = bank_offsets[bnk+1];
						loc[offset] = data;
						return;
					}

					if (bnk == STATIC_ROM)
					{
						logerror("write8: %x to %x in ROM region!\n", data, address);
						return;
					}
				}

				handler = rc->handler;
				(*handler)(offset, data);
			}

			return;
		}

		rc = rc->next;
	}

	logerror("Unmapped write8 %x at %x\n", data, (int)address);
	return;
}

void memory_writeport8(offs_t port, data8_t data)
{
	M1ZoneT *rc;
	offs_t offset;
	void (*handler)(offs_t offset, data8_t data);
	offs_t address = port;
	int bnk;

	rc = mem_cpu[mem_cur_cpu].iowritechain;
	if (rc != (M1ZoneT *)NULL)
	{
		if ((rc->start & 0xff00) || (rc->end & 0xff00))
		{
			address = port;
		}
		else
		{
			address = port & 0xff;
		}
	}
	while (rc != (M1ZoneT *)NULL)
	{
		if (address >= rc->start && address <= rc->end)
		{
			if ((rc->start & 0xff00) || (rc->end & 0xff00))
			{
				offset = port;
			}
			else
			{
				offset = address - rc->start;
			}

			if (rc->handler)
			{
				bnk = (size_t)rc->handler;
				if (bnk == STATIC_NOP)
				{
					return;
				}

				handler = rc->handler;
				(*handler)(offset, data);
			}

			return;
		}

		rc = rc->next;
	}

	logerror("Unmapped writeport8 %x at %x\n", data, (int)address);
	return;
}














