// HLE system22 using the VivaNonno engine

#include "m1snd.h"

#include "viva/SoundSequencer.h"
#include "viva/PCMDevice.h"

static unsigned int sk_readmem(unsigned int address);
static void sk_writemem(unsigned int address, unsigned int data);

static int seq_timer = 0, last_play = -1;

static PCMDevice *myDevice;
static SoundSequencer sseq[64];

#define TIMER_RATE	(1.0/7800.0)

static M16502T dcrw = 
{
	sk_readmem,
	sk_readmem,
	sk_writemem,
};

static void vn_timer(int refcon)
{
	int j;

        for (j = 0 ; j < 64 ; j++)
	{
                sseq[j].update(1);
	}

	timer_set(TIMER_RATE, 0, vn_timer);
}

static void hc352_update(int num, short **buf, int samples)
{
	int i;

	for (i = 0; i < samples; i++)
	{
		myDevice->run(62);
		buf[0][i] = myDevice->out(0);
		buf[1][i] = myDevice->out(1); 
#if 0
                if (seq_timer++ >= (44100/(60*130)))
                {
                        for (j = 0 ; j < 64 ; j++)
			{
	                        sseq[j].update(1);
			}
                        seq_timer = 0;
                }
#endif
	}
}

static int sh_start(const struct MachineSound *msound)
{
	char buf[2][40];
	const char *name[2];
	int vol[2];

	sprintf(buf[0], "C352 L"); 
	sprintf(buf[1], "C352 R"); 
	name[0] = buf[0];
	name[1] = buf[1];
	vol[0]=100;
	vol[1]=100;
	stream_init_multi(2, name, vol, 44100, 0, hc352_update);

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

static unsigned int sk_readmem(unsigned int address)
{
	return prgrom[address];
}

static void sk_writemem(unsigned int address, unsigned int data)
{
}

static void S22HLE_Init(long srate)
{
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

	myDevice = new PCMDevice;
	myDevice->setPCMData((int8_t *)memory_region(RGN_SAMP1));

	for (i = 0; i < 64; i++)
	{
                sseq[i].setWorkMemory(workram, 65536);
                sseq[i].setSequenceData(memory_region(RGN_CPU2));
                sseq[i].setPCMDevice(myDevice);
                sseq[i].setSequencerGroup(sseq);
		sseq[i].reset();
	}

	for (i = 0; i < 2; i++)
	{
		myDevice->setMasterLevel(i, 0.14f);
	}

	seq_timer = 0;

	timer_set(TIMER_RATE, 0, vn_timer);
}

static void S22HLE_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xffff)
	{
		if (last_play != -1)
		{
			sseq[last_play].stop();
			last_play = -1;
		}
	}
	else
	{
		// ace driver
		if (Machine->refcon == 1 && (cmda == 67 || cmda == 68 || cmda == 64 || cmda == 208))
		{
			last_play = -1;
		}	// tekken 2
		else if (Machine->refcon == 2 && (cmda == 101))
		{
			last_play = -1;
		}	// xevi3dg
		else if (Machine->refcon == 3 && (cmda == 129))
		{
			last_play = -1;
		}
		else
		{
			sseq[4].play(cmda&0xfff);
			last_play = 4;
		}
	}
}

static void S22HLE_Shutdown(void)
{
	if (last_play != -1)
	{
		sseq[last_play].stop();
	}
	last_play = -1;

	delete myDevice;
}

M1_BOARD_START( viva )
	MDRV_NAME( "Viva Nonno HLE driver" )
	MDRV_HWDESC( "HLE, C352" )
	MDRV_DELAYS( 60, 60 )
	MDRV_INIT( S22HLE_Init )
	MDRV_SHUTDOWN( S22HLE_Shutdown )
	MDRV_SEND( S22HLE_SendCmd )

	MDRV_SOUND_ADD(CUSTOM, &custom_interface)
M1_BOARD_END

