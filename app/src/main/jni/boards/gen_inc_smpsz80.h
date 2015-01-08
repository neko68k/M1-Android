/*
** SMPS Z80 driver initialization routines etc. for sound emulators supporting Megadrive games
** by unknownfile, 2008
**
** This software is public domain. It can be used in pretty much anything.
**
** Note that this driver does NOT cover games on the 32X, Sega CD or
** any game where the 68000 must be running. Find someone else to do that.
**
** TODO:
** - better init detection
**
** 07/29/2008 - got Battletoads working, finding a better way of patching stuff
**        other than cheap hax using previous notes
** 07/30/2008 - patches added. S3&K and Battletoads patches working.
** 08/02/2008 - Sonic Crackers, Sonic 2, PS3 patches working
*/

// INCLUDES GO HERE, AND STUFF.
#include <string.h>

/*
** SMPS Z80 driver signatures!
** These are all tell-tale give-away signs for Z80 drivers,
** taken from various games.
*/

enum {
    Z80_INIT_BATTLETOADS=0,    // Battletoads - uses a jsr to a Z80 bus request before it executes stuff
    Z80_INIT_SCRACKERS,    // Sonic Crackers - does everything in the init function
    Z80_INIT_SONIC3K,    // Sonic 3 & Knuckles
    Z80_INIT_SONIC2,    // Sonic 2
    Z80_INIT_PS3,        // Phantasy Star 3, Generations of Dung
    Z80_INIT_EXAMPLES,    // The maximum
};

unsigned char Z80_Driver_Inits[Z80_INIT_EXAMPLES][32] =
{
    // Battletoads
    { 0x4E, 0xB9, 0x00, 0x00, 0x03, 0x0C, 0x41, 0xF9,
      0x00, 0xA0, 0x00, 0x00, 0x43, 0xF9, 0x00, 0x07,
      0xDC, 0xE4, 0x32, 0x3C, 0x10, 0x00, 0x10, 0xD9,
      0x53, 0x41, 0x66, 0xFA, 0x41, 0xF9, 0x00, 0xA0, },

    // Sonic Crackers
    { 0x46, 0xFC, 0x27, 0x00, 0x33, 0xFC, 0x01, 0x00,
      0x00, 0xA1, 0x11, 0x00, 0x08, 0x39, 0x00, 0x00,
      0x00, 0xA1, 0x11, 0x00, 0x66, 0xF6, 0x33, 0xFC,
      0x01, 0x00, 0x00, 0xA1, 0x12, 0x00, 0x41, 0xFA, },

    // Sonic 3 & Knuckles
    { 0x33, 0xFC, 0x01, 0x00, 0x00, 0xA1, 0x11, 0x00,
      0x33, 0xFC, 0x01, 0x00, 0x00, 0xA1, 0x12, 0x00,
      0x41, 0xF9, 0x00, 0x0F, 0x69, 0x60, 0x43, 0xF9,
      0x00, 0xA0, 0x00, 0x00, 0x61, 0x00, 0x07, 0x44, },

    // Sonic 2
    { 0x4D, 0xFA, 0x00, 0x9C, 0x3E, 0x3C, 0x0F, 0x64,
      0x7C, 0x00, 0x4B, 0xF9, 0x00, 0xA0, 0x00, 0x00,
      0x7A, 0x00, 0x49, 0xF9, 0x00, 0xA0, 0x00, 0x00,
      0xE2, 0x4E, 0x08, 0x06, 0x00, 0x08, 0x66, 0x0A, },

    // Phantasy Star 3
    { 0x33, 0xFC, 0x01, 0x00, 0x00, 0xA1, 0x11, 0x00,
      0x61, 0x00, 0x00, 0xA6, 0x61, 0x00, 0x00, 0xCE,
      0x61, 0xD6, 0x4D, 0xF9, 0x00, 0xA0, 0x00, 0x00,
      0x4B, 0xF9, 0x00, 0x07, 0x57, 0xB8, 0x30, 0x3C, },
} ;

// If the blocks above aren't autodetected, we use this one to guess stuff.
// This is the code that is used to reset the Z80. This, however, is unreliable.
unsigned char Z80_Driver_Guess_Block[] = {
      0x33, 0xFC, 0x01, 0x00, 0x00, 0xA1, 0x11, 0x00,
};

static int smpsz80_init (int * ldoffset) {
    unsigned int LoadOffset=0,i;

    printf("SMPSZ80: now scanning for code blocks!\n");

    for (i=0;i<Z80_INIT_EXAMPLES;i++) {
        printf("Z80 code init block %d: NOT FOUND.\n",i);
        LoadOffset = find_pattern(Z80_Driver_Inits[i],0x20);
        if (LoadOffset) {
            printf("FOUND Z80 LOADER! (ID = %d)\n",i);
            break;
        }
    }

    // If we can't find it, just give up
    if (!LoadOffset) {
          printf("Couldn't find the Z80 code loader stub!\n");
          return -1;
    }


    *ldoffset = LoadOffset;

    printf("Patch went OK\n");

    return 0;

}
