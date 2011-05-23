/* Namco Rally-X and friends */

#include "m1snd.h"

#define Z80_CLOCK (18432000/6)

#define Ral_TIMER_PERIOD (1.0/60.606060)

WRITE_HANDLER( pengo_sound_w );
extern unsigned char *pengo_soundregs;

static int irqmask = 0, bank = 0;
static void mspacman_decode(void);

static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	100,		/* playback volume */
	RGN_SAMP1	/* memory region */
};

static READ_HANDLER( Ral_Read )
{
	if (offset <= 0x3fff)
	{
		return prgrom[offset];
	}

	if (offset >= 0x8000 && offset <= 0x9fff)
	{
//		if (offset >= 0x89f4 && offset <= 0x89f6) printf("read sound @ %x\n", offset);
		return workram[offset];
	}

	if (offset == 0xa000)
	{
		return 0x01;	// service mode on
	}

	if (offset == 0xa080 || offset == 0xa100)
	{
		return 0x00;
	}

//	printf("Unmapped read at %x\n", address);
	return 0;
}

static WRITE_HANDLER( Ral_Write )
{		
	if (offset >= 0xa100 && offset <= 0xa11f)
	{
//		printf("%02x to sound reg @ %x\n", data, offset);
		pengo_sound_w(offset-0xa100, data);
		return;
	}			    

	if (offset == 0xa181)
	{
//		printf("NMI ack/lock: %x (PC=%x)\n", data, z80_get_reg(REG_PC));
		if ((data&1) == 0)
		{
//			printf("IRQ clear\n");
			cpu_set_irq_line(0, 0, CLEAR_LINE);
		}
		irqmask = (data & 1);
		return;
	}

	if (offset >= 0x8000 && offset <= 0x9fff)
	{
		workram[offset] = data;

//		if (offset >= 0x89f4 && offset <= 0x89f6) printf("%02x to sound @ %x\n", data, offset);
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static READ_HANDLER( Pac_Read )
{
	if (offset <= 0x3fff)
	{
//		if (bank) printf("[%04x]\n", offset);
		return prgrom[offset+bank];
	}

	if (offset >= 0x4000 && offset <= 0x4fff)
	{
		return workram[offset];
	}

	if (offset == 0x5040)
	{
		return 0xef;	// service mode on
	}

	if (offset == 0x5000 || offset == 0x5080 || offset == 0x50c0)
	{
		return 0xff;
	}

	if (offset >= 0x8000)
	{
		if (Machine->refcon == 1)
		{
			return prgrom[offset-0x8000+bank];
		}
		
		return prgrom[offset];
	}
	return 0;
}

static WRITE_HANDLER( Pac_Write )
{		
	if (offset >= 0x5040 && offset <= 0x505f)
	{
		pengo_sound_w(offset-0x5040, data);
		return;
	}			    

	if ((offset == 0x5006) && (Machine->refcon == 1))
	{
		bank = 0x10000;
		return;
	}

	if (offset == 0x5000)
	{
//		printf("NMI ack/lock: %x (PC=%x)\n", data, z80_get_reg(REG_PC));
		if ((data&1) == 0)
		{
			cpu_set_irq_line(0, 0, CLEAR_LINE);
		}
		irqmask = (data & 1);
		return;
	}

	if (offset >= 0x4000 && offset <= 0x4fff)
	{
		workram[offset] = data;
		return;
	}

//	printf("Unmapped write %x to %x\n", data, address);
}

static WRITE_HANDLER(irqv_w)
{
	z80_set_irqvec(data);
 	cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void gal_timer(int refcon)
{
//	printf("------------------------VBL\n");
	if (irqmask)
	{
//		printf("VBL\n");
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	}

	// set up for next time
	timer_set(Ral_TIMER_PERIOD, 0, gal_timer);
}

static void Ral_Init(long srate)
{
	pengo_soundregs = &workram[0xa100];

	timer_set(Ral_TIMER_PERIOD, 0, gal_timer);

	irqmask = 0;
}

static void Pac_Init(long srate)
{
	static const struct {
	    int count;
	    int value;
	} table[] =
	{
		{ 0x00C1, 0x00 },{ 0x0002, 0x80 },{ 0x0004, 0x00 },{ 0x0006, 0x80 },
		{ 0x0003, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0004, 0x80 },
		{ 0x9968, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0001, 0x80 },
		{ 0x00AF, 0x00 },{ 0x000E, 0x04 },{ 0x0002, 0x00 },{ 0x0004, 0x04 },
		{ 0x001E, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0002, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0002, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0083, 0x00 },{ 0x0001, 0x04 },
		{ 0x0001, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x05 },{ 0x0001, 0x00 },
		{ 0x0003, 0x04 },{ 0x0003, 0x01 },{ 0x0002, 0x00 },{ 0x0001, 0x04 },
		{ 0x0003, 0x01 },{ 0x0003, 0x00 },{ 0x0003, 0x04 },{ 0x0001, 0x01 },
		{ 0x002E, 0x00 },{ 0x0078, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },
		{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },
		{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0001, 0x05 },{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },{ 0x0001, 0x00 },
		{ 0x01B0, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x01 },{ 0x00AD, 0x00 },
		{ 0x0031, 0x01 },{ 0x005C, 0x00 },{ 0x0005, 0x01 },{ 0x604E, 0x00 },
	    { 0,0 }
	};
	int i, j, A;

	pengo_soundregs = &workram[0x5040];

	timer_set(Ral_TIMER_PERIOD, 0, gal_timer);

	irqmask = 0;
	bank = 0;

	if (Machine->refcon == 2)
	{
		for (i = A = 0; table[i].count; i++)
			for (j = 0; j < table[i].count; j++)
				prgrom[A++] ^= table[i].value;
	}
	else if (Machine->refcon == 1)
	{
		mspacman_decode();
	}
}

static void Ral_SendCmd(int cmda, int cmdb)
{
	pengo_sound_enable_w(0, 1);

	if (cmda == 0xffff) return;

	workram[0x89f4 + (cmda/8)] = 1<<(cmda%8);
}

static void Pac_SendCmd(int cmda, int cmdb)
{
	pengo_sound_enable_w(0, 1);

	if (Machine->refcon == 2)	// Jr. Pac Man
	{
		if (cmda == 0xffff)
		{
			workram[0x4e9c] = 0;
			workram[0x4eac] = 0;
			workram[0x4ebc] = 0;
			workram[0x4ecc] = 0;
			workram[0x4eec] = 0;
			return;
		}
		switch (cmda/8)
		{
			case 0:
				if (cmda == 2)
				{
					workram[0x4e0a] = 0x81;
					workram[0x4e13] = 0x04;
					cmda = 1;
				}
				else if (cmda == 3)
				{
					workram[0x4e0a] = 0x7f;
					workram[0x4e13] = 0x02;
					cmda = 1;
				}
				else
				{
					workram[0x4e0a] = 0x7d;
					workram[0x4e13] = 0x00;
				}
				workram[0x4ecc] = 1<<(cmda%8);
				workram[0x4eec] = 1<<(cmda%8);
				break;

			case 1:
				workram[0x4e9c] = 1<<(cmda%8);
				break;
			case 2:
				workram[0x4eac] = 1<<(cmda%8);
				break;
			case 3:
				workram[0x4ebc] = 1<<(cmda%8);
				break;
		}
	}
	else if (Machine->refcon == 1)	// Ms. Pac
	{
		if (cmda == 0xffff)
		{
			workram[0x4e9c] = 0;
			workram[0x4eac] = 0;
			workram[0x4ebc] = 0;
			workram[0x4ecd] = 0;
			workram[0x4edd] = 0;
			return;
		}
		switch (cmda/8)
		{
			case 0:
			#if 0
				if (cmda == 2)
				{
					workram[0x4e0a] = 0x81;
					workram[0x4e13] = 0x04;
					cmda = 1;
				}
				else if (cmda == 3)
				{
					workram[0x4e0a] = 0x82;
					workram[0x4e13] = 0x05;
					cmda = 1;
				}
				else
				{
					workram[0x4e0a] = 0;
					workram[0x4e13] = 0;
				}
			#endif
				workram[0x4ecc] = 1<<(cmda%8);
				workram[0x4edc] = 1<<(cmda%8);
				break;

			case 1:
				workram[0x4e9c] = 1<<(cmda%8);
				break;
			case 2:
				workram[0x4eac] = 1<<(cmda%8);
				break;
			case 3:
				workram[0x4ebc] = 1<<(cmda%8);
				break;
		}
	}
	else
	{
		if (cmda == 0xffff)
		{
			workram[0x4e9c] = 0;
			workram[0x4eac] = 0;
			workram[0x4ebc] = 0;
			workram[0x4ecc] = 0;
			workram[0x4edc] = 0;
			return;
		}
		switch (cmda/8)
		{
			case 0:
				workram[0x4ecc] = 1<<(cmda%8);
				workram[0x4edc] = 1<<(cmda%8);
				break;

			case 1:
				workram[0x4e9c] = 1<<(cmda%8);
				break;
			case 2:
				workram[0x4eac] = 1<<(cmda%8);
				break;
			case 3:
				workram[0x4ebc] = 1<<(cmda%8);
				break;
		}
	}
}

static MEMORY_READ_START( readmem_cpu3 )
	{ 0x0000, 0xffff, Ral_Read },
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu3 )
	{ 0x0000, 0xffff, Ral_Write },
MEMORY_END

static PORT_READ_START( snd_readport )
PORT_END

static PORT_WRITE_START( snd_writeport )
	{ 0x00, 0x00, irqv_w },
PORT_END

M1_BOARD_START( rallyx )
	MDRV_NAME("Rally-X")
	MDRV_HWDESC("Z80, Namco WSG")
	MDRV_INIT( Ral_Init )
	MDRV_SEND( Ral_SendCmd )
	MDRV_DELAYS( 4000, 15 )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(readmem_cpu3,writemem_cpu3)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)
	
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
M1_BOARD_END

static MEMORY_READ_START( readmem_pac )
	{ 0x0000, 0xffff, Pac_Read },
MEMORY_END

static MEMORY_WRITE_START( writemem_pac )
	{ 0x0000, 0xffff, Pac_Write },
MEMORY_END

M1_BOARD_START( pacman )
	MDRV_NAME("Pacman")
	MDRV_HWDESC("Z80, Namco WSG")
	MDRV_INIT( Pac_Init )
	MDRV_SEND( Pac_SendCmd )
	MDRV_DELAYS( 2000, 15 )

	MDRV_CPU_ADD(Z80C, Z80_CLOCK)
	MDRV_CPU_MEMORY(readmem_pac,writemem_pac)
	MDRV_CPU_PORTS(snd_readport,snd_writeport)
	
	MDRV_SOUND_ADD(NAMCO, &namco_interface)
M1_BOARD_END

static UINT8 decryptd(UINT8 e)
{
	UINT8 d;

	d  = (e & 0x80) >> 3;
	d |= (e & 0x40) >> 3;
	d |= (e & 0x20)     ;
	d |= (e & 0x10) << 2;
	d |= (e & 0x08) >> 1;
	d |= (e & 0x04) >> 1;
	d |= (e & 0x02) >> 1;
	d |= (e & 0x01) << 7;

	return d;
}

static UINT32 decrypta1(UINT32 e)
{
	UINT32 d;

	d  = (e & 0x800)     ;
	d |= (e & 0x400) >> 7;
	d |= (e & 0x200) >> 2;
	d |= (e & 0x100) << 1;
	d |= (e & 0x80) << 3;
	d |= (e & 0x40) << 2;
	d |= (e & 0x20) << 1;
	d |= (e & 0x10) << 1;
	d |= (e & 0x08) << 1;
	d |= (e & 0x04)     ;
	d |= (e & 0x02)     ;
	d |= (e & 0x01)     ;

	return d;
}

static UINT32 decrypta2(UINT32 e)
{
	UINT32 d;
	d  = (e & 0x800)     ;
	d |= (e & 0x400) >> 2;
	d |= (e & 0x200) >> 2;
	d |= (e & 0x100) >> 3;
	d |= (e & 0x80) << 2;
	d |= (e & 0x40) << 4;
	d |= (e & 0x20) << 1;
	d |= (e & 0x10) >> 1;
	d |= (e & 0x08) << 1;
	d |= (e & 0x04)     ;
	d |= (e & 0x02)     ;
	d |= (e & 0x01)     ;

	return d;
}




static void mspacman_decode(void)
{
	int i;

	/* CPU ROMs */
	for (i = 0; i < 0x1000; i++)
	{
	prgrom[0x10000+i] = prgrom[0x0000+i];
	prgrom[0x11000+i] = prgrom[0x1000+i];
	prgrom[0x12000+i] = prgrom[0x2000+i];
	prgrom[0x1a000+i] = prgrom[0x2000+i];  /*not needed but it's there*/
	prgrom[0x1b000+i] = prgrom[0x3000+i];  /*not needed but it's there*/

	}


	for (i = 0; i < 0x1000; i++)
	{
		prgrom[decrypta1(i)+0x13000] = decryptd(prgrom[0xb000+i]);	/*u7*/
		prgrom[decrypta1(i)+0x19000] = decryptd(prgrom[0x9000+i]);	/*u6*/
	}

	for (i = 0; i < 0x800; i++)
	{
		prgrom[decrypta2(i)+0x18000] = decryptd(prgrom[0x8000+i]);  	/*u5*/
		prgrom[0x18800+i] = prgrom[0x19800+i];
	}



	for (i = 0; i < 8; i++)
	{
		prgrom[0x10410+i] = prgrom[0x18008+i];
		prgrom[0x108E0+i] = prgrom[0x181D8+i];
		prgrom[0x10A30+i] = prgrom[0x18118+i];
		prgrom[0x10BD0+i] = prgrom[0x180D8+i];
		prgrom[0x10C20+i] = prgrom[0x18120+i];
		prgrom[0x10E58+i] = prgrom[0x18168+i];
		prgrom[0x10EA8+i] = prgrom[0x18198+i];

		prgrom[0x11000+i] = prgrom[0x18020+i];
		prgrom[0x11008+i] = prgrom[0x18010+i];
		prgrom[0x11288+i] = prgrom[0x18098+i];
		prgrom[0x11348+i] = prgrom[0x18048+i];
		prgrom[0x11688+i] = prgrom[0x18088+i];
		prgrom[0x116B0+i] = prgrom[0x18188+i];
		prgrom[0x116D8+i] = prgrom[0x180C8+i];
		prgrom[0x116F8+i] = prgrom[0x181C8+i];
		prgrom[0x119A8+i] = prgrom[0x180A8+i];
		prgrom[0x119B8+i] = prgrom[0x181A8+i];

		prgrom[0x12060+i] = prgrom[0x18148+i];
		prgrom[0x12108+i] = prgrom[0x18018+i];
		prgrom[0x121A0+i] = prgrom[0x181A0+i];
		prgrom[0x12298+i] = prgrom[0x180A0+i];
		prgrom[0x123E0+i] = prgrom[0x180E8+i];
		prgrom[0x12418+i] = prgrom[0x18000+i];
		prgrom[0x12448+i] = prgrom[0x18058+i];
		prgrom[0x12470+i] = prgrom[0x18140+i];
		prgrom[0x12488+i] = prgrom[0x18080+i];
		prgrom[0x124B0+i] = prgrom[0x18180+i];
		prgrom[0x124D8+i] = prgrom[0x180C0+i];
		prgrom[0x124F8+i] = prgrom[0x181C0+i];
		prgrom[0x12748+i] = prgrom[0x18050+i];
		prgrom[0x12780+i] = prgrom[0x18090+i];
		prgrom[0x127B8+i] = prgrom[0x18190+i];
		prgrom[0x12800+i] = prgrom[0x18028+i];
		prgrom[0x12B20+i] = prgrom[0x18100+i];
		prgrom[0x12B30+i] = prgrom[0x18110+i];
		prgrom[0x12BF0+i] = prgrom[0x181D0+i];
		prgrom[0x12CC0+i] = prgrom[0x180D0+i];
		prgrom[0x12CD8+i] = prgrom[0x180E0+i];
		prgrom[0x12CF0+i] = prgrom[0x181E0+i];
		prgrom[0x12D60+i] = prgrom[0x18160+i];
	}
}

