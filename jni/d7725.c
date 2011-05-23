#include <stdio.h>
#include <stdlib.h>

#define ID  ((opcode & 0x3fffc0)>>6)
#define SRC ((opcode & 0xf0)>>4)
#define DST (opcode & 0xf)

#define BRCH ((opcode & 0x3fe000)>>13)
#define NA   ((opcode & 0x001ffc)>>2)

#define ALU ((opcode & 0x0f0000)>>16)
#define PSEL ((opcode & 0x300000)>>20)

#define ASEL (opcode & 0x008000)

#define DPL ((opcode & 0x006000)>>13)

static char *destregs[16] = {
	"NON", "A", "B", "TR", "DP", "RP", "DR", "SR", "SOL", "SOM", "K", "KLR", "KLM", "L", "TRB", "MEM"
};

static char *srcregs[16] = {
	"TRB", "A", "B", "TR", "DP", "RP", "RO", "SGN", "DR", "DRNF", "SR", "SIM", "SIL", "K", "L", "MEM"
};

static char *aluops[16] = {
	"NOP", "OR", "AND", "XOR", "SUB", "ADD", "SBB", "ADC", "DEC", "INC", "CMP", "SHR1", "SHL1", "SHL2", "SHL4", "XCHG"
};

static char *psources[] = { "RAM", "IDB", "M", "N" };

static char *dec_branch(int brch)
{
	switch (brch)
	{
		case 0x100:
			return "JMP";
			break;

		case 0x140:
			return "CALL";
			break;

		case 0x080:
			return "JNCA";
			break;

		case 0x082:
			return "JCA";
			break;

		case 0x084:
			return "JNCB";
			break;

		case 0x086:
			return "JCB";
			break;

		case 0x088:
			return "JNZA";
			break;

		case 0x08a:
			return "JZA";
			break;

		case 0x08c:
			return "JNZB";
			break;

		case 0x08e:
			return "JZB";
			break;

		case 0x090:
			return "JNOVA0";
			break;

		case 0x092:
			return "JOVA0";
			break;

		case 0x094:
			return "JNOVB0";
			break;

		case 0x096:
			return "JOVB0";
			break;

		case 0x098:
			return "JNOVA1";
			break;

		case 0x09a:
			return "JOVA1";
			break;

		case 0x09c:
			return "JNOVB1";
			break;

		case 0x09e:
			return "JOVB1";
			break;

		case 0x0a0:
			return "JNSA0";
			break;

		case 0x0a2:
			return "JSA0";
			break;

		case 0x0a4:
			return "JNSB0";
			break;

		case 0x0a6:
			return "JSB0";
			break;

		case 0x0a8:
			return "JNSA1";
			break;

		case 0x0aa:
			return "JSA1";
			break;

		case 0x0ac:
			return "JNSB1";
			break;

		case 0x0ae:
			return "JSB1";
			break;

		case 0x0b0:
			return "JDPL0";
			break;

		case 0x0b1:
			return "JDPLN0";
			break;

		case 0x0b2:
			return "JDPLF";
			break;

		case 0x0b3:
			return "JDPLNF";
			break;

		case 0x0b4:
			return "JNSIAK";
			break;

		case 0x0b6:
			return "JSIAK";
			break;

		case 0x0b8:
			return "JNSOAK";
			break;

		case 0x0ba:
			return "JSOAK";
			break;
		
		case 0x0bc:
			return "JNRQM";
			break;

		case 0x0be:
			return "JRQM";
			break;

		default:
			return "JP UNK!";
			break;
	}
}

int main(void)
{
	int i;
	unsigned int opcode;
	unsigned char in;
	char disasm[512];
	FILE *f;

	f = fopen("d7725.01", "rb");

	for (i = 0; i < 0x2000; i++)
	{
		opcode = 0;
		sprintf(disasm, "");
		fread(&in, 1, 1, f);
		opcode = (in << 16);
		fread(&in, 1, 1, f);
		opcode |= (in << 8);
		fread(&in, 1, 1, f);
		opcode |= in;
		fread(&in, 1, 1, f);	// skip the check byte

		switch (opcode & 0xc00000)
		{
			case 0:	// OP instruction
				if (ALU)
				{
					if (ALU <= 10)
					{
						sprintf(disasm, "%s %s, (%s), %s", aluops[ALU], srcregs[SRC], psources[PSEL], ASEL ? "B" : "A");
					}
					else
					{
						sprintf(disasm, "%s %s", aluops[ALU], srcregs[SRC]);	
					}
				}
				else
				{	// ALU NOP is a move, maybe
					sprintf(disasm, "MOV %s, %s (%s)", srcregs[SRC], destregs[DST], psources[PSEL]);
				}

				switch (DPL)
				{
					case 0:
						break;
					case 1:
						strcat(disasm, " (DP+)");
						break;
					case 2:
						strcat(disasm, " (DP-)");
						break;
					case 3:
						strcat(disasm, " (DP = 0)");
						break;
				}
				break;

			case 0x400000:	// RT
				if (ALU)
				{
					if (ALU <= 10)
					{
						sprintf(disasm, "RT %s %s, %s, %s", aluops[ALU], srcregs[SRC], psources[PSEL], ASEL ? "A" : "B");
					}
					else
					{
						sprintf(disasm, "RT %s %s", aluops[ALU], srcregs[SRC]);	
					}
				}
				else
				{
					sprintf(disasm, "RT");
				}

				switch (DPL)
				{
					case 0:
						break;
					case 1:
						strcat(disasm, " (DP+)");
						break;
					case 2:
						strcat(disasm, " (DP-)");
						break;
					case 3:
						strcat(disasm, " (DP = 0)");
						break;
				}
				break;

			case 0x800000:	// JP
				sprintf(disasm, "%s %x", dec_branch(BRCH), NA);
				break;

			case 0xc00000:	// LD
				sprintf(disasm, "LD #$%x, %s", ID, destregs[DST]);
				break;
		}

		printf("%04x: %06x  %s\n", i, opcode, disasm);
	}

	return 0;
}
