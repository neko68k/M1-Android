/*
** M1 driver for software running on Mitchell hardware
** Driver by unknownfile, 2008
** Special thanks to ugetab with some help on the Pang driver.
**
** Games running on this hardware besides mgakuen all use the Kabuki encryption scheme,
** which scrambles the code into two 0x8000-sized chunks of data containing opcodes and data.
**
** TODO:
** - Proper IRQ handling
** - OKI-M2695 sample support (not supported by m1 yet, and will likely need to be sometime soon)
**
** REVISION HISTORY
** 07/10/2008 - started work on Mahjong Gakuen test driver
** 07/13/2008 - Pang and Super Pang driver added, mgakuen now uses its own driver.
** 07/14/2008 - Block Block added, Pang_Patch() redefined to Patch_Kabuki() with setup args
** 07/15/2008 - Rest of the games added except Monsters World, which is a bootleg and I'm pretty sure we 
**              don't need to add it.
** 07/26/2008 - OKIM6295 support added, code cleaned up a bit. Send values over 0x80 to play samples.
*/

#include "m1snd.h"

unsigned char CurrentBank=0;
unsigned short Init_Offset=0, Play_Offset=0;


// I don't know why I did this... but whatever, I guess.
unsigned char myRam[0x4000];

// This is for Kabuki descrambling
unsigned char descrambleOn=0;


// Defined later on
static READ_HANDLER( Mitchell_ReadMem ); 
static READ_HANDLER( Mitchell_ReadPort );
static READ_HANDLER ( Mitchell_ReadOp );
static WRITE_HANDLER( Mitchell_WritePort );
static WRITE_HANDLER ( Mitchell_WriteMem );
static void Mitchell_SendCmd (int cmda, int cmdb);
static void Mitchell_Init (long srate);

static MEMORY_READ_START( m_rd )
	{ 0x0000, 0xffff, Mitchell_ReadMem },
MEMORY_END

static MEMORY_READ_START( m_rdop )
	{ 0x0000, 0xffff, Mitchell_ReadOp },
MEMORY_END

static MEMORY_WRITE_START( m_wr )
	{ 0xC000, 0xffff, Mitchell_WriteMem },
MEMORY_END

static PORT_READ_START( m_rdport )
	{ 0x0000, 0xffff, Mitchell_ReadPort },
PORT_END

static PORT_WRITE_START( m_wrport )
	{ 0x0000, 0xffff, Mitchell_WritePort },
PORT_END

/*
** Kabuki decryption subroutines, see also kabuki.cpp.
** Unsupported games are commented out until they are needed.
** There are three releases of Super Pang, the World version is only supported.
*/

void pang_decode(void);
void spang_decode(void);
void block_decode(void);
void mgakuen2_decode(void);
void cworld_decode(void);
void hatena_decode(void);
void marukin_decode(void);
void qtono1_decode(void);
void qsangoku_decode(void);

//void spangj_decode(void);
//void sbbros_decode(void);

/*
** Sound interfaces
*/

static struct YM2413interface ym2413_interface =
{
	1,
	4000000,		/* 4 MHz on all boards */
	{ YM2413_VOL(100,0,100,0) },
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 990000 / 132 }, // This is a fairly standard clock, but who cares, really
	{ REGION_SOUND1 },
	{ 52 }
};

/*
** Driver: Mahjong Gakuen
** Pretty much the same as the Mitchell driver, 
** Hardware is technically the same as Pang, just a bit slower and without Kabuki encryption.
** Z80 clock speed is 6 MHz
*/
M1_BOARD_START( mgakuen )
	MDRV_NAME("Mahjong Gakuen")
	MDRV_HWDESC("Z80, YM2413")
	MDRV_INIT( Mitchell_Init )

	MDRV_SEND( Mitchell_SendCmd )
	MDRV_CPU_ADD(Z80C, 658000) // TODO: Better IRQ hack, the one Pang uses crashes this!
	MDRV_CPU_MEMORY ( m_rd, m_wr)
	MDRV_CPU_PORTS  ( m_rdport, m_wrport)

	MDRV_SOUND_ADD (YM2413, &ym2413_interface)

M1_BOARD_END

/*
** Driver: Mitchell
** Covers Pang and other games running on the same hardware.
** Uses Kabuki encryption scheme, and a faster Z80. (8MHz ORIGINAL BOARD)
*/

M1_BOARD_START( mitchell )
	MDRV_NAME("Mitchell")
	MDRV_HWDESC("Z80, YM2413, MSM6295")
	MDRV_INIT( Mitchell_Init )

	MDRV_SEND( Mitchell_SendCmd )
	MDRV_CPU_ADD(Z80C, 8000000)
	MDRV_CPU_MEMORY ( m_rd, m_wr)
	MDRV_CPU_PORTS  ( m_rdport, m_wrport)
	MDRV_CPU_READOP ( m_rdop )

	
	MDRV_SOUND_ADD (YM2413, &ym2413_interface)
	MDRV_SOUND_ADD (OKIM6295, &okim6295_interface)
M1_BOARD_END

/*
** MITCHELL READ/WRITE HANDLERS
*/

// Read handler for memory
static READ_HANDLER( Mitchell_ReadMem ) {
//	unsigned int distance=0;
//	char buf[30];
	//printf("Read at %04X.\n",offset);
	if (offset <= 0x7FFF) return prgrom[offset];
	if (offset >= 0x8000 && offset <= 0xBFFF) return prgrom[0x10000+(CurrentBank*0x4000)+(offset-0x8000)];
	if (offset >= 0xC000 && offset <= 0xFFFF) {
		 return myRam[offset - 0xC000];
	}
	return 0;
}

static WRITE_HANDLER ( Mitchell_WriteMem) {
	myRam[offset]=data;
}

unsigned char IRQ_Hack=1;


void Mitchell_Timer (int refcon)
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	timer_set(3.0/180.0, 0, Mitchell_Timer);	
}

static READ_HANDLER ( Mitchell_ReadOp ) {
//	unsigned int distance=0;
//	char buf[30];
	if (offset <= 0x7FFF) return prgrom[(descrambleOn*0x8000)+offset];
	if (offset >= 0x8000 && offset <= 0xBFFF) {
		return prgrom[0x10000+(CurrentBank*0x4000)+(offset-0x8000)];
	}
	if (offset >= 0xC000 && offset <= 0xFFFF) return myRam[offset - 0xC000];
	return 0;

}

// IRQ stuff, will implement later.
static READ_HANDLER ( Mitchell_ReadPort ) {
	int port = offset & 0xFF;

	switch (port) {
		case 0x05:
			return IRQ_Hack;	// shit
		break;
	}

	return 0;

}
static WRITE_HANDLER( Mitchell_WritePort) {
	int port = offset & 0xFF;

	switch (port) {
	  case 0x02: // bankswitch
		CurrentBank = data;
		//printf("Bank switched to %d\n",CurrentBank);
		break;
	  case 0x03: // YM2413 writes
		YM2413_data_port_0_w(0,data);	
		break;
	  case 0x04:
		YM2413_register_port_0_w(0,data);
		break;
	  case 0x05:	// OKIM6295 commands (unused by the driver but here anyways
		OKIM6295_data_0_w(0,data);
		//printf("Caught write to OKI: %02X\n",data);
		break;
	}

	//printf("Port write %02X: %02X\n",port,data);

}


/*
** GAME_SPECIFIC PATCHES AND STUFF
*/

static unsigned char Patch_Data[0x20] = {
//	 00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F
	0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0x16, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00,  'l',  'o',  'l',  ' ',  'u',  'f', 0x00,

};

static unsigned char Patch_Ops[0x20] = {
//	 00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F
	0xFB, 0xED, 0x00, 0x31, 0x00, 0x00, 0x3E, 0x00, 0xCD, 0x00, 0x00, 0xCD, 0x00, 0x00, 0x01, 0x00,
	0x00, 0xED, 0xB0, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void Mitchell_Patch (int kabuki, unsigned short opll_init, unsigned short opll_play, unsigned short sp) {
	int i=0;
	if (kabuki) {
		memcpy(prgrom,Patch_Data,0x20);
		memcpy(prgrom+0x8000,Patch_Ops,0x20);
		prgrom[0x0004]=(sp & 0xFF); prgrom[0x0005]=(sp >> 8);
		prgrom[0x0009]=(opll_init & 0xFF); prgrom[0x000A]=(opll_init >> 8);
		prgrom[0x000C]=(opll_play & 0xFF); prgrom[0x000D]=(opll_play >> 8);
	} else {
		memcpy(prgrom,Patch_Data,0x18);
		for (i=0;i<0x18;i++) prgrom[i] ^= Patch_Ops[i];
		prgrom[0x0004]=(sp & 0xFF); prgrom[0x0005]=(sp >> 8);
		prgrom[0x0009]=(opll_init & 0xFF); prgrom[0x000A]=(opll_init >> 8);
		prgrom[0x000C]=(opll_play & 0xFF); prgrom[0x000D]=(opll_play >> 8);
	}
}

// Gakuen_Patch: Code patch for Mahjong Gakuen.
// I still need to revise this.
void Gakuen_Patch (void) {
	// Clear the first bit of ROM data for code.
	memset(prgrom+3,0,0x20);

	// Enable interrupts
	prgrom[0]=0;

	// set up the stack pointer here
	// This is sort of glitchy but hey
	prgrom[3]=0x31; prgrom[4]=0x40; prgrom[5]=0xEE;	

	// ld a, soundcode
	prgrom[6]=0x3E; prgrom[7]=0x20;

	// call init
	prgrom[8]=0xCD; prgrom[9]=0x03; prgrom[10]=0x78;

	// sync is actually called from 0x38
	// due to the CPU slowdown hack, it's called directly
	prgrom[11]=0xCD; prgrom[12]=0x00; prgrom[13]=0x78;

	// Burn some cycles (ldir)
	prgrom[14]=0xED; prgrom[15]=0xB0;	

	// jump backwards
	prgrom[16] = 0xC3; prgrom[17]=0x09; prgrom[18]=0x00;
	
}

/*
** M1 INTERFACING CRAP
*/

static void Mitchell_Init (long srate) {
	descrambleOn=0;
	
//	printf("Machine->refcon = %d\n", Machine->refcon);
	timer_set(3.0/180.0, 0, Mitchell_Timer);	// Might need this later.
	
	switch (Machine->refcon) {
		case 0x00:			// No encryption - This is an old version, and there's only one game.
			Gakuen_Patch();	// Mahjong Gakuen (1988 Yuga)
			//Mitchell_Patch(0,0x7803,0x7800,0xEE40);
			break;
		case 0x01:			// Pang (World), Pomping World (Japan), Buster Bros. (US)
			pang_decode();
			Mitchell_Patch (1,0x7803,0x7800,0x0000);
			descrambleOn=1;
			break;
		case 0x02:			// Super Pang (World)
			spang_decode();
			Mitchell_Patch(1,0x7803,0x7800,0x0000);
			descrambleOn=1;
			break;
		case 0x03:			// Block Block
			block_decode();
			Mitchell_Patch (1,0x2abf,0x2abc,0xf880);
			descrambleOn=1;
			break;
		case 0x04:			// Mahjong Gakuen 2, Poker Ladies, Dokaben
			mgakuen2_decode();
			Mitchell_Patch (1,0x7803,0x7800,0xFC80);
			descrambleOn=1;
			break;
		case 0x05:			// Capcom World
			cworld_decode();
			Mitchell_Patch (1,0x700C,0x7009,0xFAFF); // this SP is a guess.
			descrambleOn=1;
			break;
		case 0x06:			// Adventure Quiz 2 Hatena Hatena no Daibouken
			hatena_decode();
			Mitchell_Patch (1,0x76AD,0x76AA,0x0000);
			descrambleOn=1;
			break;
		case 0x07:			// Super Marukin-Ban
			marukin_decode();
			Mitchell_Patch (1,0x7629,0x7626,0xF880);
			descrambleOn=1;
			
			break;
		case 0x08:			// Quiz Tonosama no Yabou (Japan)
			qtono1_decode();
			Mitchell_Patch (1,0x7235,0x7232,0xFAC0);
			descrambleOn=1;
			break;
		case 0x09:			// Quiz Sangokushi
			qsangoku_decode();
			Mitchell_Patch (1,0x75CD,0x75CA,0xFAC0);
			descrambleOn=1;
			break;
	}
	memset(myRam,0,0x4000);

}

static void Mitchell_SendCmd (int cmda, int cmdb) {
//	printf("Command sent: %d, %d\n",cmda,cmdb);
	
	// If we write anything equal or above 0x80, it's the OKI's duty to play it, so we stop the Z80.
	// Otherwise, we let the Z80 drive the YM2413.
	if (cmda >= 0x80) { 
		prgrom[0x0001] = 0x78;
		Mitchell_WritePort(5,0x40);
		Mitchell_WritePort(5,0x80 | (cmda & 0x7F));
		Mitchell_WritePort(5,0x80);
	} else {
		prgrom[0x0001] = 0xED;
		prgrom[0x0007]=(cmda & 0x7F);
	}
	
	memset(myRam,0,0x4000);

	cpu_set_reset_line(0,0xFF);

}
