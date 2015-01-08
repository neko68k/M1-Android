/*
** Arkanoid driver for M1
** by unknownfile, 2008
**
** This game uses a very simple range of addresses at 0xE980.
**
** TODO: Use proper interrupts instead of using a CPU slowdown hack!
**
*/

#include "m1snd.h"

static struct AY8910interface ay910intf = {
	1,
	1500000,
	{ 33 },
	{ 0 },
	{ 0 },
        { 0,0 },
        { 0,0 },
};

// eh, simple enough
static MEMORY_READ_START( ark_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xC000, 0xc7ff, MRA_RAM },
	{ 0xd001, 0xd001, AY8910_read_port_0_r },
	{ 0xe840, 0xefff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( ark_writemem )
	{ 0x0000, 0xbfff, MWA_ROM }, // although, it *is* rom
	{ 0xC000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, AY8910_control_port_0_w },
	{ 0xd001, 0xd001, AY8910_write_port_0_w },
	{ 0xe840, 0xefff, MWA_RAM },
MEMORY_END


void Arkanoid_SendCmd (int a, int b) {
//	unsigned char base=1;
	// Write that sound stuff and it'll play

//	printf("Command received: %d\n",a);
	prgrom[5]=(a & 0xff);	

	cpu_set_reset_line(0, 0xFF);

}

void Arkanoid_Init (long sr) {
	// Just patch the thing up a bit
	// set the stack pointer up and then keep calling the update address
	
	//prgrom[0] = 0x00; // KEEP INTERRUPTS ENABLED.
	prgrom[1] = 0x31;	   // Init stack pointer
	prgrom[2] = 0xC0;
	prgrom[3] = 0xC0;

	// Load sound code
	prgrom[4] = 0x3E; prgrom[5]=0xFF;

	// Call init
	prgrom[6] = 0xCD; prgrom[7]=0xAE; prgrom[8]=0x67;

	// Call some more functions in a loop
	prgrom[9] = 0xCD; prgrom[10] = 0x92; prgrom[11] = 0x67;
	prgrom[12] = 0xCD; prgrom[13] = 0xBE; prgrom[14] = 0x15;

	// Loop backwards
	prgrom[15] = 0xC3; prgrom[16] = 0x09; prgrom[17] = 0x00;

	// Initial sound code, for whatever reason, is 0x10
//	Arkanoid_SendCmd (0x10, 0);

}

// Board definition!
M1_BOARD_START( arkanoid )
	MDRV_NAME("Arkanoid")
	MDRV_HWDESC("Z80, AY-3-8910A")
	MDRV_INIT( Arkanoid_Init )
	MDRV_SEND( Arkanoid_SendCmd )

	MDRV_CPU_ADD(Z80C, 6000000/13) 
	MDRV_SOUND_ADD(AY8910, &ay910intf) 
	MDRV_CPU_MEMORY( ark_readmem, ark_writemem )
M1_BOARD_END


