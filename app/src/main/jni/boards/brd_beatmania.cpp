// BeatMania

#include "m1snd.h"
#include "chd.h"

extern "C" {
#include "harddisk.h"
}

static UINT8 buffers[4][512];
static int curblk, bstat;
static UINT32 curlba;
static struct hard_disk_file *ourhd;

static void BM_SendCmd(int cmda, int cmdb)
{
	curlba = cmda;
}

static unsigned int sk_readmem(unsigned int address)
{
	return prgrom[address];
}

static void sk_writemem(unsigned int address, unsigned int data)
{
}

static void reload_buf(void)
{
	UINT8 *src = (UINT8 *)&buffers[curblk][0];
	#if LSB_FIRST
	int i;
	UINT8 temp;
	#endif

	hard_disk_read(ourhd, curlba, 1, src);
	curlba++;

	#if LSB_FIRST	// byte-swap on little-endian
	for (i = 0; i < 512; i += 2)
	{
		temp = src[i];
		src[i] = src[i+1];
		src[i+1] = temp;
	}
	#endif

	if (!(curlba % 64)) printf("LBA: %d\n", curlba);
}

static void fillblk(INT16 *outp, INT16 *outp2, int samples)
{
	UINT8 *src = &buffers[curblk][512-(bstat*2)];
	INT16 *src2;
	int i;

	src2 = (INT16 *)src;

	for (i = 0; i < samples; i++)
	{
		*outp++ = *src2;
		*outp2++ = *src2;
		src2++;
	}
}

static void bm_update(int num, short **buf, int samples)
{
	int tofill;
	INT16 *outp = buf[0];
	INT16 *outp2 = buf[1];

	while (samples)
	{
		tofill = (bstat > samples) ? samples : bstat;
		fillblk(outp, outp2, tofill);
		outp += tofill;
		outp2 += tofill;
		bstat -= tofill;
		samples -= tofill;

		if (!bstat)
		{
			reload_buf();
			curblk++;
			if (curblk >= 4)
			{
				curblk = 0;
			}
			bstat = 512/2;
		}
	}
}

static int sh_start(const struct MachineSound *msound)
{
	char buf[2][40];
	const char *name[2];
	int vol[2];
			
	sprintf(buf[0], "beatmania L"); 
	sprintf(buf[1], "beatmania R"); 
	name[0] = buf[0];
	name[1] = buf[1];
	vol[0]=50;
	vol[1]=50;
	stream_init_multi(2, name, vol, Machine->refcon*1000, 0, bm_update);
 	return 0;
}

static void sh_stop(void)
{
}

static struct CustomSound_interface custom_interface =
{
	sh_start,
	sh_stop,
	0
};

static M16502T dcrw = 
{
	sk_readmem,
	sk_readmem,
	sk_writemem,
};

static void BM_Init(long srate)
{
	struct chd_file *chd;
	int i;

	prgrom = rom_getregion(RGN_CPU1);

	// m1's core can't work without at least 1 CPU, so
	// construct a phony 6502 program that does nothing.
	// point all vectors to 0x2000
	prgrom[0xfff0] = 0x00;
	prgrom[0xfff1] = 0x20;
	prgrom[0xfff2] = 0x00;
	prgrom[0xfff3] = 0x20;
	prgrom[0xfff4] = 0x00;
	prgrom[0xfff5] = 0x20;
	prgrom[0xfff6] = 0x00;
	prgrom[0xfff7] = 0x20;
	prgrom[0xfff8] = 0x00;
	prgrom[0xfff9] = 0x20;
	prgrom[0xfffa] = 0x00;
	prgrom[0xfffb] = 0x20;
	prgrom[0xfffc] = 0x00;
	prgrom[0xfffd] = 0x20;
	prgrom[0xfffe] = 0x00;
	prgrom[0xffff] = 0x20;

	prgrom[0x2000] = 0x4c;	// JMP 2000
	prgrom[0x2001] = 0x00;
	prgrom[0x2002] = 0x20;

	m1snd_add6502(100000, &dcrw);	// 0.1 MHz, why waste time on it :)

	chd = rom_get_disk_handle(0);
	ourhd = hard_disk_open(chd);

	for (i = 0; i < 4; i++)
	{
		curblk = i;
		reload_buf();
	}

	curblk = 0;
	bstat = 512/2;
}

M1_BOARD_START( beatmania )
	MDRV_NAME("beatmania")
	MDRV_HWDESC("Hard disk, HLE")
	MDRV_INIT( BM_Init )
	MDRV_SEND( BM_SendCmd )

	MDRV_SOUND_ADD(CUSTOM, &custom_interface)
M1_BOARD_END

