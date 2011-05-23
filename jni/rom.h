/* m1 rom loading system */

#ifndef _M1ROM_H_
#define _M1ROM_H_

// ROM region defs
typedef enum
{
	RGN_CPU1 = 0,
	RGN_CPU2,
	RGN_CPU3,
	RGN_CPU4,

	RGN_SAMP1,
	RGN_SAMP2,
	RGN_SAMP3,
	RGN_SAMP4,

	RGN_USER1,
	RGN_USER2,

	RGN_DISK,

	RGN_MAX,
} MemRegionsE;

// mame compatibility
#define REGION_CPU1 RGN_CPU1
#define REGION_CPU2 RGN_CPU2
#define REGION_CPU3 RGN_CPU3
#define REGION_CPU4 RGN_CPU4

#define REGION_SOUND1 RGN_SAMP1
#define REGION_SOUND2 RGN_SAMP2
#define REGION_SOUND3 RGN_SAMP3
#define REGION_SOUND4 RGN_SAMP4

#define REGION_USER1	RGN_USER1
#define REGION_USER2	RGN_USER2

// region flags

#define RGN_MASK	0x0000003f	// mask for region flags

#define RGN_NORMAL	0x00000000  	// normal region
#define RGN_DISPOSE	0x00000001  	// region is deallocated after driver init
#define RGN_CLEAR	0x00000002  	// clear region to some value
#define RGN_LE		0x00000004	// region is little-endian
#define RGN_BE		0x00000008	// region is big-endian
#define RGN_32BIT	0x00000010	// region's width is 32 bit (else 16)
#define RGN_DISK	0x00000020	// region is a disk
#define RGN_CLEARVAL(x)	((x&0xff)<<8)	// how to set the clear value

// two obvious defaults for what to clear a region to
#define RGN_CLEAR0	RGN_CLEAR | RGN_CLEARVAL(0)
#define RGN_CLEARF	RGN_CLEAR | RGN_CLEARVAL(0xff)

// region tracking struct

typedef struct RomRegionT 
{
	char *memory;
	long size;
	int flags;
} RomRegionT;

// ROM entry type
typedef struct RomEntryT
{
	char *name;
	long loadadr;
	long length;
	long crc;
	unsigned long flags;
	char sha1[41];
	char md5[33];
} RomEntryT;

// max ROMs per game
#define ROM_MAX			(24)

// ROM flags definitions
#define ROM_RGNDEF		0x80000000	// set if rom entry is really a region def
#define ROM_ENDLIST		0x40000000	// set at the end of a rom list

#define ROM_WIDTHMASK		0x00000300	// width units for the ROM

#define ROM_BYTE	  	0x00000000	// bytes
#define ROM_WORD		0x00000100	// words
#define ROM_DWORD		0x00000200	// dwords

#define ROM_SKIPMASK		0x000ff000	// skip per unit in bytes.  allow up to 256.
#define ROM_SKIP(x)		(((x)<<12) & ROM_SKIPMASK)
#define ROM_NOSKIP		ROM_SKIP(0)

#define ROM_REVERSEMASK		0x00100000	
#define ROM_REVERSE		0x00100000	// reverse byte order in a group 

// MAME-compatible macros using the above stuff
// base macro
#define ROMX_LOAD(name, adr, size, crc, flags)	{ name, adr, size, crc, flags },

// standard all-purpose no-frills load
#define ROM_LOAD(name, adr, size, crc) ROMX_LOAD(name, adr, size, crc, 0)

// 16-bit-wide ROM loads
#define ROM_LOAD16_BYTE(name, adr, length, crc) ROMX_LOAD(name, adr, length, crc, ROM_SKIP(1))
#define ROM_LOAD16_WORD(name, adr, length, crc) ROMX_LOAD(name, adr, length, crc, 0)
#define ROM_LOAD16_WORD_SWAP(name, adr, length, crc) ROMX_LOAD(name, adr, length, crc, ROM_WORD | ROM_REVERSE)

// 32-bit-wide ROM loads
#define ROM_LOAD32_BYTE(name, adr, length, crc) ROMX_LOAD(name, adr, length, crc, ROM_SKIP(3))
#define ROM_LOAD32_WORD(name, adr, length, crc) ROMX_LOAD(name, adr, length, crc, ROM_WORD | ROM_SKIP(2))
#define ROM_LOAD32_WORD_SWAP(name, adr, length, crc) ROMX_LOAD(name, adr, length, crc, ROM_WORD | ROM_REVERSE | ROM_SKIP(2))

// region start
#define ROM_REGION(size, rgn, rgn_flags)	ROMX_LOAD(NULL, rgn, size, 0, ROM_RGNDEF|rgn_flags)
#endif

#define ROM_START {
#define ROM_END	ROMX_LOAD(NULL, 0, 0, 0, ROM_ENDLIST) },

// public functions
int rom_loadgame(void);
void rom_postinit(void);
unsigned char *rom_getregion(int rgn);
long rom_getregionsize(int rgn);
void rom_shutdown(void);
char *rom_getfilename(int game, int romnum);
int rom_getfilesize(int game, int romnum);
int rom_getfilecrc(int game, int romnum);
int rom_getnum(int game);
void rom_setpath(char *newpath);
struct chd_file *rom_get_disk_handle(int disk);

