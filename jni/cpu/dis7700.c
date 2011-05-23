/*

 Mitsubishi 7700 series disassembler

*/

#include <stdio.h>
#include "m7700ds.h"
#include "m37710.h"

unsigned char prg[512*1024];
unsigned char workram[0x10000];

int trace = 0;

int main(int argc, char *argv[])
{
	FILE *f;
	int pc = 0xc030, len;
	int i, j, m, x;
	char txt[2048];
	char *eptr;

	m = x = 0;

	if (argc < 2)
	{
		printf("USAGE: %s filename (offset)\n", argv[0]);
		return(0);
	}

	if (argc >= 3)
	{
		pc = strtoul(argv[2], &eptr, 0);
	}

	f = fopen(argv[1], "rb");
	fread(prg, 512*1024, 1, f);
	fclose(f);

	m37710_init();
	m37710_reset(NULL);

	pc = m37710_get_pc();
	trace = 0;
	while (pc < 512*1024)
	{
#if 1
if (trace)
{
		switch (prg[pc])
		{
			case 0xc2:	// REP (CLP in mitsu)
				if (prg[pc+1] & 0x10)
				{
					m = 0;
				}
				if (prg[pc+1] & 0x20)
				{
					x = 0;
				}
				break;

			case 0xe2:	// SEP
				if (prg[pc+1] & 0x10)
				{
					m = 1;
				}
				if (prg[pc+1] & 0x20)
				{
					x = 1;
				}
				break;

			case 0xd8:	// CLM
				m = 0;
				break;

			case 0xf8:	// SEM
				m = 1;
				break;
		}
		printf("A=%x B=%x X=%x Y=%x K=%x PC=%x DB=%x P=%x SP=%x\n",
			m37710_get_reg(M37710_A),
			m37710_get_reg(M37710_B),
			m37710_get_reg(M37710_X),
			m37710_get_reg(M37710_Y),
			m37710_get_reg(M37710_PB),
			m37710_get_pc(),
			m37710_get_reg(M37710_DB),
			m37710_get_reg(M37710_P),
			m37710_get_reg(M37710_S));

		len = m7700_disassemble(txt, pc, 0, m, x);
		printf("[%06x]: ", pc);
		printf("%18s   ", txt);
		for (j = 0; j < len; j++)
		{
			printf("%02x ", m7700_read_8_disassembler(pc+j));
		}
		printf("(m=%d x=%d)", m, x);
		printf("\n");
		pc += len;
}
//#else
		m37710_execute(1);
		pc = m37710_get_pc();
		if (pc == 0xc11d)
		{
			trace = 1;
//			return 0; //  trace = 1;
		}
		if (m37710_get_reg(M37710_P) & 0x10)
			x = 1;
		else
			x = 0;
		if (m37710_get_reg(M37710_P) & 0x20)
			m = 1;
		else
			m = 0;
#endif
	}
	return 0;
}

unsigned int m7700_read_8_disassembler(unsigned int addr)
{
	return prg[addr];
}

unsigned int m37710_read(unsigned int addr)
{
	if (trace)
		printf("Read at %x (PC=%x)\n", addr, m37710_get_pc());

	if (addr >= 0x80 && addr <= 0x27f)
		return workram[addr];

	if (addr >= 0x4000 && addr <= 0xbfff)
		return workram[addr];

	if (addr >= 0xc000 && addr <= 0x7ffff)
		return prg[addr];

	printf("Unmapped read at %x (PC=%x)\n", addr, m37710_get_pc());

	return 0;
}

unsigned int m37710_write(unsigned int addr, unsigned int data)
{
	if (trace)
		printf("Write %x to %x (PC=%x)\n", data, addr, m37710_get_pc());

	if (addr >= 0x80 && addr <= 0x27f)
	{
		workram[addr] = data;
		return;
	}

	if (addr >= 0x4000 && addr <= 0xbfff)
	{
		workram[addr] = data;
		return;
	}

	printf("Unknown write %x to %x (PC=%x)\n", data, addr, m37710_get_pc());
}

#if 0
[00c12f]: BBS #$02, $80, 00c147 ($14)   24 80 02 14 (m=1 x=0)
#endif
