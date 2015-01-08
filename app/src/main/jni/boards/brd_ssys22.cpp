// Namco (Super) System 22 and friends driver

#include "m1snd.h"

static void SS22_Init(long srate);
static void SS22_SendCmd(int cmda, int cmdb);

static unsigned int ss22_read(unsigned int address);
static void ss22_write(unsigned int address, unsigned int data);
static unsigned int ss22_read16(unsigned int address);
static void ss22_write16(unsigned int address, unsigned int data);

static int phase, is_super, is_c7x;

M137710T ss22_rw =
{
	ss22_read,
	ss22_write,
	ss22_read16,
	ss22_write16
};

static struct C352interface c352_interface =
{
	YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT),
	YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT),
	REGION_SOUND1,
	16384000
};

static unsigned int ss22_read(unsigned int address)
{
	if (address <= 0x7f)
	{
		return m37710_internal_r(address);
	}

	if (address >= 0x80 && address <= 0x27f)
	{
		return workram[address];	// 37710 internal RAM
	}

	if (address >= 0x4000 && address <= 0xbfff)
	{
		return workram[address];	// shared RAM
	}

	if (address >= 0xc000 && address <= 0xffff)
	{
		if (is_super)
			return prgrom[address];
		else
			if (is_c7x)
				return prgrom2[address];
			else
				return prgrom2[address-0xc000];
	}

	if (address >= 0x80000 && address <= 0x100000)
	{
		return prgrom[(address & 0x7ffff)];
	}

	if (address >= 0x200000 && address <= 0x2fffff)
	{
		return prgrom[(address & 0x7ffff)];
	}

	return 0;
}

static unsigned int ss22_read16(unsigned int address)
{
	if (address <= 0x7f)
	{
		return m37710_internal_r(address) | m37710_internal_r(address+1)<<8;
	}

	if (address >= 0x80 && address <= 0x27f)
	{
		return workram[address] | workram[address+1]<<8;	// 37710 internal RAM
	}

	if (address >= 0x2000 && address <= 0x2fff)
	{
		return c352_0_r((address-0x2000)>>1, 0);
	}

	if (address >= 0x4000 && address <= 0xbfff)
	{
		return workram[address] | workram[address+1]<<8;	// shared RAM
	}

	if (address >= 0xc000 && address <= 0xffff)
	{
		if (is_super)
			return prgrom[address] | prgrom[address+1]<<8;
		else
			if (is_c7x)
				return prgrom2[address] | prgrom2[address+1]<<8; 
			else
				return prgrom2[address-0xc000] | prgrom2[address-0xc000+1]<<8; 
	}

	if (address >= 0x200000 && address <= 0x27ffff)
	{
		return prgrom[(address & 0x7ffff)] | prgrom[(address & 0x7ffff) + 1]<<8;
	}
	return 0;
}

static void ss22_write(unsigned int address, unsigned int data)
{
	if (address <= 0x7f)
	{
		m37710_internal_w(address, data);
		return;
	}

	if (address >= 0x80 && address <= 0x27f)
	{
		workram[address] = data & 0xff;
		return;
	}

	if (address >= 0x4000 && address <= 0xbfff)
	{
		workram[address] = data & 0xff;
		return;
	}

	if (address >= 0x308000 && address <= 0x308003) return;	// ???
}

static void ss22_write16(unsigned int address, unsigned int data)
{
	if (address <= 0x7f)
	{
		m37710_internal_w(address, data);
		m37710_internal_w(address+1, (data>>8)&0xff);
		return;
	}

	if (address >= 0x80 && address <= 0x27f)
	{
		workram[address] = data & 0xff;
		workram[address+1] = (data>>8)&0xff;
		return;
	}

	if (address >= 0x2000 && address <= 0x2fff)
	{
		c352_0_w((address-0x2000)>>1, data, 0);
		return;
	}

	if (address >= 0x4000 && address <= 0xbfff)
	{
		workram[address] = data & 0xff;
		workram[address+1] = (data>>8)&0xff;
		return;
	}

	if (address >= 0x308000 && address <= 0x308003) return;	// ???
	if (address == 0x301000) return;	// ???
}

static void s22_timer(int refcon)
{
	if (is_c7x) return;

	switch (phase)
	{
		case 0:
			m37710_set_irq_line(M37710_LINE_IRQ0, ASSERT_LINE);
			break;
		case 1:
			m37710_set_irq_line(M37710_LINE_IRQ2, ASSERT_LINE);
			break;
	}

	phase++;
	if (phase == 2) phase = 0;
	timer_set(1.0/60.0*2.0, 0, s22_timer);
}

static void SS22_Init(long srate)
{
	timer_setnoturbo();

	phase = 0;
	is_c7x = 0;

	timer_set(1.0/60.0*2.0, 0, s22_timer);

	// clear all RAM
	memset(&workram[0x0000], 0, 0xc000);

	// patch out the startup sound
	if (prgrom2)
	{
		// check for start of prop cycle program
		if (prgrom2[0xc000] == 0x53) is_c7x = 1;

		if (is_c7x)
		{
		     	prgrom2[0xd616] = 0xea;
		     	prgrom2[0xd617] = 0xea;
		     	prgrom2[0xd618] = 0xea;
		     	prgrom2[0xd619] = 0xea;
			prgrom2[0xd61a] = 0xea;
		}
	}
	else
	{
//		printf("version [%s]\n", &prgrom[0xc000]);

		if (!strncmp("S22-BIOS ver1.41", (const char *)&prgrom[0xc000], 16))
		{
			prgrom[0xd616] = 0xea;
			prgrom[0xd617] = 0xea;
			prgrom[0xd618] = 0xea;
			prgrom[0xd619] = 0xea;
			prgrom[0xd61a] = 0xea;
//			printf("Prop cycle 1.41\n");
		}
		else if (!strncmp("S22-BIOS ver1.30", (const char *)&prgrom[0xc000], 16))
		{
			prgrom[0xd631] = 0xea;
			prgrom[0xd632] = 0xea;
			prgrom[0xd633] = 0xea;
			prgrom[0xd634] = 0xea;
			prgrom[0xd635] = 0xea;
//			printf("Alpine Racer 1.30\n");
		}
		else if (!strncmp("S22-BIOS ver1.20", (const char *)&prgrom[0xc000], 16))
		{
			prgrom[0xd631] = 0xea;
			prgrom[0xd632] = 0xea;
			prgrom[0xd633] = 0xea;
			prgrom[0xd634] = 0xea;
			prgrom[0xd635] = 0xea;
//			printf("Air Combat 22b 1.20\n");
		}
	}

	is_super = 1;
}

static void SS22_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xff00) return;

	if (is_super || is_c7x)
	{
		workram[0x4001] = ((0x40) | (cmda >>8)) & 0xf;
		workram[0x4000] = cmda;

		// set all volumes to max
		workram[0x4022] = 0x3f;
		workram[0x4023] = 0x3f;
		workram[0x4024] = 0x3f;
		workram[0x4025] = 0x3f;
	}
	else
	{			  
		workram[0x5001] = (0x40 | (cmda >>8)) & 0xf;
		workram[0x5000] = cmda;

		// set all volumes to max
		workram[0x5022] = 0x3f;
		workram[0x5023] = 0x3f;
		workram[0x5024] = 0x3f;
		workram[0x5025] = 0x3f;
	}
}

M1_BOARD_START( ss22 )
	MDRV_NAME("Namco Super System 22")
	MDRV_HWDESC("M37710, C352")
	MDRV_INIT( SS22_Init )
	MDRV_SEND( SS22_SendCmd )

	MDRV_CPU_ADD(M37710, 16384000)
	MDRV_CPUMEMHAND(&ss22_rw)

	MDRV_SOUND_ADD(C352, &c352_interface)
M1_BOARD_END

static void S22_Init(long srate)
{
	UINT8 *rom1 = rom_getregion(RGN_CPU1);

	SS22_Init(srate);

	// if c7x mode, kill the per-program special code, it crashes the SS22 BIOS 
	if (is_c7x)
	{
		memset(&rom1[0x0000], 0xff, 0x10);
	}

	is_super = 0;
}

M1_BOARD_START( s22 )
	MDRV_NAME("Namco System 22")
	MDRV_HWDESC("M37702, C352")
	MDRV_INIT( S22_Init )
	MDRV_SEND( SS22_SendCmd )

	MDRV_CPU_ADD(M37710, 16384000)
	MDRV_CPUMEMHAND(&ss22_rw)

	MDRV_SOUND_ADD(C352, &c352_interface)
M1_BOARD_END

