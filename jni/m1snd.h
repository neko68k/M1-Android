#ifndef _M1SND_H
#define _M1SND_H

// includes
#include <stdio.h>
#include <string.h>
#include "m1stdinc.h"	// get all the includes
#include "m1queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __WIN32__
#define STDCALL	__stdcall
#else
#define STDCALL
#endif

#if LSB_FIRST
#define Endian16_Swap(x) (x)
#define Endian_16_Swap(x) (x)
#define Endian32_Swap(x) (x)
#else
#define Endian16_Swap(x) (((x)<<8) | ((x)>>8))
#define Endian_16_Swap(x) (((x)<<8) | ((x)>>8))
#define Endian32_Swap(x) ( ((x&0xff000000)>>24) || ((x&0x00ff0000)>>8) || ((x&0x0000ff00)<<8) || ((x&0x000000ff)<<24) )
#endif

#define LE16(x) Endian16_Swap(x)
#define LE32(x) Endian32_Swap(x)

// ROM loading utilities
#define CRC(z) 0x##z
#define SHA1(x)

/***************************************************************************

	Useful macros to deal with bit shuffling encryptions

***************************************************************************/

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
		(((((val) >> (B7)) & 1) << 7) | \
		 ((((val) >> (B6)) & 1) << 6) | \
		 ((((val) >> (B5)) & 1) << 5) | \
		 ((((val) >> (B4)) & 1) << 4) | \
		 ((((val) >> (B3)) & 1) << 3) | \
		 ((((val) >> (B2)) & 1) << 2) | \
		 ((((val) >> (B1)) & 1) << 1) | \
		 ((((val) >> (B0)) & 1) << 0))

#define BITSWAP16(val,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		(((((val) >> (B15)) & 1) << 15) | \
		 ((((val) >> (B14)) & 1) << 14) | \
		 ((((val) >> (B13)) & 1) << 13) | \
		 ((((val) >> (B12)) & 1) << 12) | \
		 ((((val) >> (B11)) & 1) << 11) | \
		 ((((val) >> (B10)) & 1) << 10) | \
		 ((((val) >> ( B9)) & 1) <<  9) | \
		 ((((val) >> ( B8)) & 1) <<  8) | \
		 ((((val) >> ( B7)) & 1) <<  7) | \
		 ((((val) >> ( B6)) & 1) <<  6) | \
		 ((((val) >> ( B5)) & 1) <<  5) | \
		 ((((val) >> ( B4)) & 1) <<  4) | \
		 ((((val) >> ( B3)) & 1) <<  3) | \
		 ((((val) >> ( B2)) & 1) <<  2) | \
		 ((((val) >> ( B1)) & 1) <<  1) | \
		 ((((val) >> ( B0)) & 1) <<  0))

#define BITSWAP24(val,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		(((((val) >> (B23)) & 1) << 23) | \
		 ((((val) >> (B22)) & 1) << 22) | \
		 ((((val) >> (B21)) & 1) << 21) | \
		 ((((val) >> (B20)) & 1) << 20) | \
		 ((((val) >> (B19)) & 1) << 19) | \
		 ((((val) >> (B18)) & 1) << 18) | \
		 ((((val) >> (B17)) & 1) << 17) | \
		 ((((val) >> (B16)) & 1) << 16) | \
		 ((((val) >> (B15)) & 1) << 15) | \
		 ((((val) >> (B14)) & 1) << 14) | \
		 ((((val) >> (B13)) & 1) << 13) | \
		 ((((val) >> (B12)) & 1) << 12) | \
		 ((((val) >> (B11)) & 1) << 11) | \
		 ((((val) >> (B10)) & 1) << 10) | \
		 ((((val) >> ( B9)) & 1) <<  9) | \
		 ((((val) >> ( B8)) & 1) <<  8) | \
		 ((((val) >> ( B7)) & 1) <<  7) | \
		 ((((val) >> ( B6)) & 1) <<  6) | \
		 ((((val) >> ( B5)) & 1) <<  5) | \
		 ((((val) >> ( B4)) & 1) <<  4) | \
		 ((((val) >> ( B3)) & 1) <<  3) | \
		 ((((val) >> ( B2)) & 1) <<  2) | \
		 ((((val) >> ( B1)) & 1) <<  1) | \
		 ((((val) >> ( B0)) & 1) <<  0))

extern int (STDCALL *m1ui_message)(void *instance, int message, char *txt, int iparm);
extern void *m1ui_this;

typedef struct m168k_t
{
	unsigned int (*read8)(unsigned int address);
	unsigned int (*read16)(unsigned int address);
	unsigned int (*read32)(unsigned int address);
	void (*write8)(unsigned int address, unsigned int data);
	void (*write16)(unsigned int address, unsigned int data);
	void (*write32)(unsigned int address, unsigned int data);
} M168KT;

typedef struct m132031_t
{
	unsigned int (*read32)(unsigned int address);
	void (*write32)(unsigned int address, unsigned int data);
} M132031T;

typedef struct m16800_t
{
	unsigned int (*read)(unsigned int address);
	void (*write)(unsigned int address, unsigned int data);
	unsigned int (*readport)(unsigned int address);
	void (*writeport)(unsigned int address, unsigned int data);
} M16800T;

typedef struct m16809_t
{
	unsigned int (*read)(unsigned int address);
	void (*write)(unsigned int address, unsigned int data);
} M16809T;

typedef struct m16502_t
{
	unsigned int (*read)(unsigned int address);
	unsigned int (*readop)(unsigned int address);
	void (*write)(unsigned int address, unsigned int data);
} M16502T;

typedef struct m16280_t
{
	unsigned int (*read)(unsigned int address);
	void (*write)(unsigned int address, unsigned int data);
	void (*writeport)(unsigned int address, unsigned int data);
} M16280T;

typedef struct m18039_t
{
	unsigned int (*readmem)(unsigned int address);
	void (*writemem)(unsigned int address, unsigned int data);
	unsigned int (*readport)(unsigned int address);
	void (*writeport)(unsigned int address, unsigned int data);
} M18039T;

typedef struct m1nec_t
{
	unsigned int (*read)(unsigned int address);
	unsigned int (*readop)(unsigned int address);
	void (*write)(unsigned int address, unsigned int data);
	unsigned int (*readport)(unsigned int address);
	void (*writeport)(unsigned int address, unsigned int data);
} M1NECT;

typedef struct m1adsp_t
{
	unsigned int (*read8)(unsigned int address);
	unsigned int (*read16)(unsigned int address);
	unsigned int (*read32)(unsigned int address);
	void (*write8)(unsigned int address, unsigned int data);
	void (*write16)(unsigned int address, unsigned int data);
	void (*write32)(unsigned int address, unsigned int data);
	unsigned char *op_rom;	// opcode rom
} M1ADSPT;

typedef struct m1z80_t
{
	unsigned int (*read)(unsigned int address);
	unsigned int (*readop)(unsigned int address);
	void (*write)(unsigned int address, unsigned int data);
	unsigned int (*readport)(unsigned int address);
	void (*writeport)(unsigned int address, unsigned int data);
} M1Z80T;

typedef struct m137710_t
{
	unsigned int (*read)(unsigned int address);
	void (*write)(unsigned int address, unsigned int data);
	unsigned int (*read16)(unsigned int address);
	void (*write16)(unsigned int address, unsigned int data);
} M137710T;

typedef struct m1h83k2_t
{
	unsigned int (*read8)(unsigned int address);
	unsigned int (*read16)(unsigned int address);
	void (*write8)(unsigned int address, unsigned int data);
	void (*write16)(unsigned int address, unsigned int data);
	unsigned int (*ioread8)(unsigned int address);
	void (*iowrite8)(unsigned int address, unsigned int data);
} M1H83K2T;

// globals from m1snd.cpp that are deprecated
extern unsigned char *prgrom, *prgrom2, *workram;

extern int curgame;
extern unsigned int cmd1;

// no-endian longword read/write helpers
static unsigned long inline mem_readlong_noswap(unsigned int *addr)
{
	unsigned long retval;

	retval = *addr;

	return retval;
}

static void inline mem_writelong_noswap(unsigned int *addr, unsigned int value)
{
	*addr = value;
}

// INLINE FUNCTIONS TO SWAP ENDIAN

#if LSB_FIRST
static unsigned short inline mem_readword_swap(unsigned short *addr)
{
	return ((*addr&0x00ff)<<8)|((*addr&0xff00)>>8);
}

static unsigned short inline mem_readword_swap_le(unsigned short *addr)
{
	return *addr;
}

static unsigned int inline mem_readlong_swap_le(unsigned int *addr)
{
	unsigned int retval;

	retval = *addr;

	return retval;
}

static unsigned int inline mem_readlong_swap(unsigned int *addr)
{
	unsigned int res = (((*addr&0xff000000)>>24) |
		 ((*addr&0x00ff0000)>>8) |
		 ((*addr&0x0000ff00)<<8) |
		 ((*addr&0x000000ff)<<24));

	return res;
}

static void inline mem_writelong_swap(unsigned int *addr, unsigned int value)
{
	*addr = (((value&0xff000000)>>24) |
		 ((value&0x00ff0000)>>8)  |
		 ((value&0x0000ff00)<<8)  |
		 ((value&0x000000ff)<<24));
}

static void inline mem_writeword_swap(unsigned short *addr, unsigned short value)
{
	*addr = ((value&0x00ff)<<8)|((value&0xff00)>>8);
}

static void inline mem_writeword_swap_le(unsigned short *addr, unsigned short value)
{
	*addr = value;
}

static void inline mem_writelong_swap_le(unsigned int *addr, unsigned int value)
{
	*addr = value;
}
#else	// big endian
static unsigned short inline mem_readword_swap_le(unsigned short *addr)
{
	return ((*addr&0x00ff)<<8)|((*addr&0xff00)>>8);
}

static unsigned short inline mem_readword_swap(unsigned short *addr)
{
	unsigned long retval;

	retval = *addr;

	return retval;
}

static unsigned int inline mem_readlong_swap(unsigned int *addr)
{
	unsigned int retval;

	retval = *addr;

	return retval;
}

static void inline mem_writelong_swap(unsigned int *addr, unsigned int value)
{
	*addr = value;
}

static void inline mem_writeword_swap(unsigned short *addr, unsigned short value)
{
	*addr = value;
}

static void inline mem_writeword_swap_le(unsigned short *addr, unsigned short value)
{
	*addr = ((value&0x00ff)<<8)|((value&0xff00)>>8);
}

static unsigned int inline mem_readlong_swap_le(unsigned int *addr)
{
	unsigned int res = (((*addr&0xff000000)>>24) |
		 ((*addr&0x00ff0000)>>8) |
		 ((*addr&0x0000ff00)<<8) |
		 ((*addr&0x000000ff)<<24));

	return res;
}

static void inline mem_writelong_swap_le(unsigned int *addr, unsigned int value)
{
	*addr = (((value&0xff000000)>>24) |
		((value&0x00ff0000)>>8) |
		((value&0x0000ff00)<<8) |
		((value&0x000000ff)<<24));

}
#endif

static unsigned short inline mem_readword(unsigned short *addr)
{
	unsigned short retval;

	retval = *addr;

	return retval;
}

static unsigned int inline mem_readlong(unsigned int *addr)
{
	unsigned int retval;

	retval = *addr;

	return retval;
}

static void inline mem_writelong(unsigned int *addr, unsigned int value)
{
	*addr = value;
}

static void inline mem_writeword(unsigned short *addr, unsigned short value)
{
	*addr = value;
}

static unsigned int inline mem_readlong_swap_always(unsigned int *addr)
{
	unsigned int res = (((*addr&0xff000000)>>24) |
		 ((*addr&0x00ff0000)>>8) |
		 ((*addr&0x0000ff00)<<8) |
		 ((*addr&0x000000ff)<<24));

	return res;
}

// z80 trick interrupt constants
#define Z80_RST_38 0xff
#define Z80_RST_30 0xf7
#define Z80_RST_28 0xef
#define Z80_RST_20 0xe7
#define Z80_RST_18 0xdf
#define Z80_RST_10 0xd7
#define Z80_RST_08 0xcf
#define Z80_RST_00 0xc7

// some mame utility macros
#if LSB_FIRST
	#define BYTE_XOR_BE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE_XOR_LE(a)  	(a)
	#define BYTE4_XOR_BE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define BYTE4_XOR_LE(a) 	(a)
	#define WORD_XOR_BE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
	#define WORD_XOR_LE(a)  	(a)
#else
	#define BYTE_XOR_BE(a)  	(a)
	#define BYTE_XOR_LE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE4_XOR_BE(a) 	(a)
	#define BYTE4_XOR_LE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define WORD_XOR_BE(a)  	(a)
	#define WORD_XOR_LE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
#endif

// utility function protos
void m1snd_addz80(long clock, void *handlers);	// Zilog Z80 (assembly core)
void m1snd_add6280(long clock, void *handlers);	// Hudson Hu6280
void m1snd_add6502(long clock, void *handlers);	// Rockwell/WDC 6502
void m1snd_add37710(long clock, void *handlers); // Mitsu M37710
void m1snd_add6803(long clock, void *handlers);	// Motorola 6803
void m1snd_add63701(long clock, void *handlers); // Hitachi 63701 MCU
void m1snd_add6309(long clock, void *handlers);	// Hitachi HD6309
void m1snd_add6809(long clock, void *handlers);	// Motorola 6809
void m1snd_add8039(long clock, void *handlers);	// Intel 8039
void m1snd_add68k(long clock, void *handlers);	// Motorola 68000
void m1snd_addv30(long clock, void *handlers);	// NEC V30
void m1snd_addadsp2105(long clock, void *handlers);	// Analog Devices ADSP-2105
void m1snd_addadsp2115(long clock, void *handlers);	// Analog Devices ADSP-2115
void m1snd_addz80c(long clock, void *handlers);	// Zilog Z80 (C core)
void m1snd_addh83002(long clock, void *handlers);	// Hitachi H8/3002
void m1snd_add32031(long clock, void *handlers);	// TMS32031

BoardT *m1snd_getCurBoard(void);
void m1snd_initNormalizeState(void);

void m1snd_setPostVolume(float fPost);
void m1snd_resetSilenceCount(void);

// cpu utilities
void nec_set_irqvec(int vector);

// hook for the timer system to cause a dsp update
void m1snd_update_dsps(long newpos, long totalpos);

// get a custom tag from m1data (returns NULL if not found)
char *m1snd_get_custom_tag(const char *tagname);

// get the value of a custom tag from m1data (use 0x prefix in the XML for hex)
UINT32 m1snd_get_custom_tag_value(const char *tagname);

// special transition helpers
#define adpcm_update(x, y, z)

#ifdef __cplusplus
}
#endif

#endif
