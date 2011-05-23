/*
** Megadrive/Genesis sound driver include
** Sound Images driver
** by unknownfile
*/

// From Blaster Master 2, might cover other drivers.
static unsigned char SI_Z80_Block[16] =
{
    0xF3, 0x31, 0xF0, 0x1F, 0xC3, 0x42, 0x00, 0x00, 0x47, 0x65, 0x6E, 0x65, 0x73, 0x69, 0x73, 0x20, 
} ;


enum {
	SI_VER_1_2=0,
	SI_INVALID_VER,
};

static int SI_CurrentVer=0;

static int SI_GetZ80 (void) {
	int offset=0;

	offset = find_pattern(SI_Z80_Block, 0x10);
	if (offset) SI_CurrentVer = 0;
	else SI_CurrentVer = SI_INVALID_VER;

	return offset;
}

static void SI_PlaySound (unsigned char id) {
	switch (SI_CurrentVer) {
		case SI_VER_1_2:
			z80_ram[0x80] = id;
			z80_ram[0x81] = 0;
			//z80_ram[0x92] = z80_ram[0x82] = 12;
			//z80_ram[0x1fff] = 5;
			break;
		default:
			break;
	}
}