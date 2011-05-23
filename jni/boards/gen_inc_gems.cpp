// AO Genesis driver: include to handle Sega GEMS sound driver

unsigned char z80pat[] = { 0xf3, 0xed, 0x56, 0x31, 0x20, 0x1b, 0xc3 };
unsigned char z80pat2[] = { 0xf3, 0xed, 0x56, 0x31, 0x20, 0x1a, 0xc3 };
unsigned char z80pat3[] = { 0xf3, 0xed, 0x56, 0x31, 0x60, 0x1a, 0xc3 };
unsigned char z80pat4[] = { 0xf3, 0xed, 0x56, 0x31, 0x30, 0x15, 0xc3 };
unsigned char z80pat5[] = { 0xf3, 0xed, 0x56, 0x31, 0x1e, 0x1b, 0xc3 };
unsigned char gemsz80pat[] = { 0x2f, 0x09, 0x40, 0xe7, 0x00, 0x7c, 0x07, 0x00, 0x33, 0xfc, 0x01, 0x00, 0x00, 0xa1, 0x12, 0x00 };
unsigned char gemsz80pat2[] = {0x40, 0xe7, 0x00, 0x7c, 0x07, 0x00, 0x2f, 0x09, 0x33, 0xfc, 0x01, 0x00, 0x00, 0xa1, 0x12, 0x00 };
unsigned char gemsz80pat3[] = {0x40, 0xe7, 0x00, 0x7c, 0x07, 0x00, 0x33, 0xfc, 0x01, 0x00, 0x00, 0xa1, 0x12, 0x00 };
unsigned char gemsinit0[] = { 0x4e, 0x56, 0x00, 0x00, 0x4e, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x4e, 0xb9 };
unsigned char gemsinit1[] = { 0x4e, 0x56, 0x00, 0x00, 0x4e, 0xba };
unsigned char gemsinit2[] = { 0x4e, 0x56, 0x00, 0x00, 0x61, 0x00 };
unsigned char gemsinit3[] = { 0x4e, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x4e, 0xb9 };
unsigned char callinit[] = { 0x4e, 0xb9, 0, 0, 0, 0 };
unsigned char gemsstartpat[] = { 0x70, 0x10, 0x4e, 0xb9 };
unsigned char gemsstartpat2[] = { 0x70, 0x10, 0x4e, 0xba };
unsigned char gemsstartpat3[] = { 0x70, 0x10, 0x61 };
unsigned char gemsstart2pat[] = { 0x20, 0x2e, 0x00, 0x08, 0x4e, 0xb9 };
unsigned char gemsstart2pat2[] = { 0x20, 0x2e, 0x00, 0x08, 0x4e, 0xba };
unsigned char gemsstart2pat3[] = { 0x20, 0x2e, 0x00, 0x08, 0x61 };
unsigned char gemsbsrwpat[] = { 0x61, 0x00 };
unsigned char gemsjsrwpat[] = { 0x4e, 0xba };

static unsigned int find_pattern(unsigned char *pat, int patlen)
{
	int ofs = 0;

	while (ofs < 4*1024*1024)
	{
		if (!memcmp(&prgrom[ofs], pat, patlen))
		{
			return ofs;
		}

		ofs++;
	}

	return 0;
}

static unsigned int find_pattern_ex(unsigned char *pat, int patlen, int ofs)
{
	if (ofs < 0) ofs = 0;

	while (ofs < 4*1024*1024)
	{
		if (!memcmp(&prgrom[ofs], pat, patlen))
		{
			return ofs;
		}

		ofs++;
	}

	return 0;
}

static unsigned int find_patterns_ex(unsigned char *pat, int patlen, unsigned char *pat2, int patlen2, int ofs)
{
	while (ofs < 4*1024*1024)
	{
		if ((!memcmp(&prgrom[ofs], pat, patlen)) || (!memcmp(&prgrom[ofs], pat2, patlen2)))
		{
			return ofs;
		}

		ofs++;
	}

	return 0;
}

static int scanbacktorte(int start)
{
	int temp = start;

	// scan backwards to an RTE
	while ((prgrom[temp] != 0x4e) || (prgrom[temp+1] != 0x75))
	{
		temp -= 2;
	}

	return temp+2;
}

static int findstop(int start)
{
	int temp = start;

	while (((prgrom[temp] != 0x4e) || (prgrom[temp+1] != 0xba) || (prgrom[temp+4] != 0x70) || (prgrom[temp+5] != 0x12)) && (temp < 4*1024*1024))
	{
		temp += 2;
	}

	if (temp >= 4*1024*1024) return 0;

	return temp;
}

static int findstop2(int start)
{
	int temp = start;

	while (((prgrom[temp] != 0x4e) || (prgrom[temp+1] != 0xb9) || (prgrom[temp+6] != 0x70) || (prgrom[temp+7] != 0x12)) && (temp < 4*1024*1024))
	{
		temp += 2;
	}

	if (temp >= 4*1024*1024) return 0;

	return temp;
}

static int findstop3(int start)
{
	int temp = start;

	while (((prgrom[temp] != 0x61) || (prgrom[temp+1] != 0x00) || (prgrom[temp+4] != 0x70) || (prgrom[temp+5] != 0x12)) && (temp < 4*1024*1024))
	{
		temp += 2;
	}

	if (temp >= 4*1024*1024) return 0;

	return temp;
}

static int checkrelsig(int gemsinit)
{
	if ((prgrom[gemsinit+12] == 0x70) && (prgrom[gemsinit+13] == 0xff) && (prgrom[gemsinit+14] == 0x2f) && (prgrom[gemsinit+15] == 0x00))
	{
		return 1;
	}

	return 0;
}

static int find_gemsstart(unsigned char *startpat, int startpatlen, unsigned char *pat, int patlen)
{
	int gemsstart;

	gemsstart = find_pattern(startpat, startpatlen);
	if (!gemsstart) return 0;

	while (1)
	{
//		printf("Candidate (%x): %02x %02x %02x %02x %02x %02x %02x %02x\n", gemsstart, prgrom[gemsstart-6], prgrom[gemsstart-5], prgrom[gemsstart+4], prgrom[gemsstart+5], prgrom[gemsstart+6], prgrom[gemsstart+7], prgrom[gemsstart+8], prgrom[gemsstart+9]);

		if (((prgrom[gemsstart-6] == 0x4e) || (prgrom[gemsstart-6] == 0x61)) && ((!memcmp(&prgrom[gemsstart+8], pat, patlen)) || (!memcmp(&prgrom[gemsstart+6], pat, patlen)) || (!memcmp(&prgrom[gemsstart+4], pat, patlen))) )
		{
			return scanbacktorte(gemsstart-2);
		}
		else
		{
			if ((prgrom[gemsstart-4] == 0x4e) && (prgrom[gemsstart-3] == 0xba) && (prgrom[gemsstart+2] == 0x4e) && (prgrom[gemsstart+3] == 0xba))
			{
				return gemsstart-4;
			}

			gemsstart = find_pattern_ex(startpat, startpatlen, gemsstart+1);
			if (!gemsstart) return 0;
		}
	}

	return 0;
}

static int gems_init(INT32 *ginit, INT32 *initcall, INT32 *startsong, INT32 *stopsong)
{
	int z80prg, gemsloadz80, gemsinit, type = 0, initcode, gemsstart, gemsstop;
	int found;

	// validity check
	if (memcmp(&prgrom[0x100], "SEGA", 4))
	{
//		printf("SEGA signature not found, maybe not correct?\n");
		return -1;
	}
	
	z80prg = find_pattern(z80pat, 7);

	if (!z80prg)
	{
		z80prg = find_pattern(z80pat2, 7);

		if (!z80prg)
		{
			z80prg = find_pattern(z80pat3, 7);

			if (!z80prg)
			{
				z80prg = find_pattern(z80pat4, 7);

				if (!z80prg)
				{
					z80prg = find_pattern(z80pat5, 7);

					if (!z80prg) { printf("Couldn't find Z80 program, probably not GEMS?\n"); return -1; }
				}
			}
		}
	}

	//printf("Found GEMS Z80 program at %08x\n", z80prg);	
	
	gemsloadz80 = find_pattern(gemsz80pat, 16);

	if (!gemsloadz80)
	{
		gemsloadz80 = find_pattern(gemsz80pat2, 16);

		if (!gemsloadz80)
		{
			gemsloadz80 = find_pattern(gemsz80pat3, 14);
		}

		if (!gemsloadz80) { printf("Couldn't find gemsloadz80, unknown GEMS version?\n"); return -1; }
	}

	//printf("Found gemsloadz80 subroutine at %08x\n", gemsloadz80);

	gemsinit0[6] = (gemsloadz80>>24) & 0xff;
	gemsinit0[7] = (gemsloadz80>>16) & 0xff;
	gemsinit0[8] = (gemsloadz80>>8) & 0xff;
	gemsinit0[9] = (gemsloadz80 & 0xff);

	// try type 0 gemsinit with absolute addressing
	gemsinit = find_pattern(gemsinit0, 12);

	if (!gemsinit)
	{
		// try type 1 gemsinit with relative addressing
		gemsinit = find_pattern_ex(gemsinit1, 6, gemsloadz80-(64*1024));

		// verify it's type 1
		if (!checkrelsig(gemsinit))
		{
			found = 0;

			// try type 2 gemsinit with relative addressing
			gemsinit = find_pattern(gemsinit2, 6);

			if (!checkrelsig(gemsinit))
			{
				gemsinit3[2] = (gemsloadz80>>24) & 0xff;
				gemsinit3[3] = (gemsloadz80>>16) & 0xff;
				gemsinit3[4] = (gemsloadz80>>8) & 0xff;
				gemsinit3[5] = (gemsloadz80 & 0xff);
				gemsinit = find_pattern(gemsinit3, 8);

				if (!gemsinit)
				{
//					printf("Couldn't find gemsinit, unknown GEMS version?\n"); 
					return -1;	
				}
			}
			else 
			{
				type = 2;
			}
		}
		else
		{
			type = 1;
		}
	}
	else
	{
		type = 0;
	}
	
//	printf("Found gemsinit subroutine at %08x, type %d\n", gemsinit, type);

	callinit[2] = (gemsinit>>24) & 0xff;
	callinit[3] = (gemsinit>>16) & 0xff;
	callinit[4] = (gemsinit>>8) & 0xff;
	callinit[5] = (gemsinit & 0xff);

	initcode = find_pattern(callinit, 6);

	if (!initcode)
	{
		// try bsr.w version
		found = 0;
		while (!found)
		{
			short offset;
			int test;

			initcode = find_patterns_ex(gemsbsrwpat, 2, gemsjsrwpat, 2, initcode);

			if (!initcode) 
			{ 
			 	if ((prgrom[gemsinit+0x18] == 0x2f) && (prgrom[gemsinit+0x19] == 0x3c) && (prgrom[gemsinit+0x20] == 0x2f) && (prgrom[gemsinit+0x21] == 0x3c))
				{
//					printf("Self contained gemsinit found\n");
					initcode = 1;
					found = 1;
				}
				else
				{
//					printf("Couldn't find init code, unknown GEMS version?\n");  
					return -1; 
				}
			}
			else
			{
				offset = prgrom[initcode+2]<<8 | prgrom[initcode+3];

				test = initcode+offset+2;

				if (test == gemsinit)
				{
					found = 1;
				}
				else
				{
					initcode++;
				}
			}
		}
	}

	if (!initcode) 
	{ 
	 	if ((prgrom[gemsinit+0x18] == 0x2f) && (prgrom[gemsinit+0x19] == 0x3c) && (prgrom[gemsinit+0x20] == 0x2f) && (prgrom[gemsinit+0x21] == 0x3c))
		{
//			printf("Self contained gemsinit found\n");
			initcode = 1;
		}
		else
		{
//			printf("Couldn't find init code, not major but do report\n"); 
			return -1; 
		}
	};
/*
	if ((prgrom[initcode-6] == 0x2f) && (prgrom[initcode-5] == 0x3c))
	{
		printf("Found init call at %08x with move.l parameters\n", initcode);
	}
	else if ((prgrom[initcode-6] == 0x48) && (prgrom[initcode-5] == 0x79)) 
	{
		printf("Found init call at %08x with pea parameters\n", initcode);
	}
	else
	{
		printf("Found init call at %08x with unknown parameters\n", initcode);
	}
*/
	gemsstart = find_gemsstart(gemsstartpat, 4, gemsstart2pat, 6);

	if (!gemsstart) 
	{ 
		gemsstart = find_gemsstart(gemsstartpat2, 4, gemsstart2pat2, 6);
		if (!gemsstart)
		{
			gemsstart = find_gemsstart(gemsstartpat3, 3, gemsstart2pat3, 5);

			if (!gemsstart)
			{
//				printf("Couldn't find play function\n"); 
				return -1; 
			}
		}
	};

//	printf("Found gemsstartsong function at %08x\n", gemsstart);
 
 	gemsstop = findstop(gemsstart+2);

	if (gemsstop)
	{
//		printf("Found gemsstopsong function at %08x\n", gemsstop);
	}
	else
	{
		gemsstop = findstop2(gemsstart+2);

		if (gemsstop)
		{
//			printf("Found gemsstopsong function at %08x\n", gemsstop);
		}
		else
		{
			gemsstop = findstop3(gemsstart+2);


			if (gemsstop)
			{
//				printf("Found gemsstopsong function at %08x\n", gemsstop);
			}
			else
			{
//				printf("Couldn't find stop function\n");
				return -1;
			}
		}
	}
 
 	*ginit = gemsinit;
	*initcall = initcode;
	*startsong = gemsstart;
	*stopsong = gemsstop;
 	
	return 0;
}
