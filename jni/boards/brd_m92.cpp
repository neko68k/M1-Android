/* Irem M92 */

#include "m1snd.h"
#include "irem_cpu.h"

#define YM_CLOCK (14318180/4)
#define V30_CLOCK (14318180)

static int cmd_latch;

static void YM2151_IRQ(int irq)
{
	nec_set_irqvec(0x18);
	if (irq)
		v30_set_irq_line(0, ASSERT_LINE);
	else
		v30_set_irq_line(0, CLEAR_LINE);
}

static unsigned int m92_read(unsigned int address)
{
	if (address < 0x1ffff)
	{
		return prgrom[address];
	}

	if (address >= 0xa0000 && address <= 0xa3fff)
	{
		return workram[address&0x3fff];
	}

	if (address >= 0xa8000 && address <= 0xa803f)
	{
		return IremGA20_r(address & 0x3f);
	}

	if (address >= 0xa8042 && address <= 0xa8043)
	{
		return YM2151ReadStatus(0); 
	}

	if (address >= 0xa8044 && address <= 0xa8045)
	{
		v30_set_irq_line(0, CLEAR_LINE);
		return cmd_latch;
	}

	// boot vectors
	if (address >= 0xffff0 && address <= 0xfffff)
	{
		return prgrom[(address&0x1ffff)];
	}

//	printf("unmapped read at %x\n", address);
	return 0;
}

static unsigned int m92_readop(unsigned int address)
{
	if (address < 0x1ffff)
	{
		return prgrom[address+(rom_getregionsize(RGN_CPU1)/2)];
	}

	if (address >= 0xffff0 && address <= 0xfffff)
	{
		return prgrom[(address&0x1ffff)+(rom_getregionsize(RGN_CPU1)/2)];
	}

	return 0;
}

static void m92_write(unsigned int address, unsigned int data)
{
	if (address >= 0x9ff00 && address <= 0x9ffff)
	{
		return;	// irq controller?
	}

	if (address >= 0xa0000 && address <= 0xa3fff)
	{
		workram[address&0x3fff] = data;
		return;
	}

	if (address >= 0xa8000 && address <= 0xa803f)
	{
		IremGA20_w(address&0x3f, data);
		return;	// GA20
	}

	if (address >= 0xa8040 && address <= 0xa8041)
	{
		YM2151_register_port_0_w(0, data);
		return;
	}

	if (address >= 0xa8042 && address <= 0xa8043)
	{
		YM2151_data_port_0_w(0, data);
		return;
	}

	if (address >= 0xa8044 && address <= 0xa8047)
	{
		return;	// irq ack and status stuff
	}

//	printf("unmapped write %x to %x\n", data, address);
}

static unsigned int m92_readport(unsigned int address)
{
//	printf("unmapped port read at %x\n", address);
	return 0;
}

static void m92_writeport(unsigned int address, unsigned int data)
{
//	printf("unmapped port write %x to %x\n", data, address);
}

static void M92_Init(long srate)
{
	// decrypt the program
	switch (Machine->refcon)
	{
		case 0:
			irem_cpu_decrypt(0, rtypeleo_decryption_table);
			break;		    
		case 1:
			irem_cpu_decrypt(0, inthunt_decryption_table);
			break;		    
		case 2:
			irem_cpu_decrypt(0, gunforce_decryption_table);
			break;		    
		case 3:
			irem_cpu_decrypt(0, bomberman_decryption_table);
			break;		    
		case 4:
			irem_cpu_decrypt(0, lethalth_decryption_table);
			break;		    
		case 5:
			irem_cpu_decrypt(0, dynablaster_decryption_table);
			break;		    
		case 6:
			irem_cpu_decrypt(0, mysticri_decryption_table);
			break;		    
		case 7:
			irem_cpu_decrypt(0, majtitl2_decryption_table);
			break;		    
		case 8:
			irem_cpu_decrypt(0, hook_decryption_table);
			break;		    
		case 9:
			irem_cpu_decrypt(0, leagueman_decryption_table);
			break;		    
		case 10:
			irem_cpu_decrypt(0, psoldier_decryption_table);
			break;		    
		case 11:
			irem_cpu_decrypt(0, lethalth_decryption_table);
			break;		    
	}


//	m1snd_addv30(V30_CLOCK, &m92_rw);

	m1snd_setCmdPrefix(0);
}

static void M92_SendCmd(int cmda, int cmdb)
{
	cmd_latch = cmda;
	nec_set_irqvec(0x19);
	v30_set_irq_line(0, ASSERT_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,
	14318180/4,
	{ YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },	// normally 40
	{ YM2151_IRQ }
};

static struct IremGA20_interface iremGA20_interface =
{
	14318180/4,
	RGN_SAMP1,
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) },
};

M1NECT m92_rw =
{
	m92_read,
	m92_readop,
	m92_write,
	m92_readport,
	m92_writeport
};

M1_BOARD_START( m92 )
	MDRV_NAME( "Irem M92" )
	MDRV_HWDESC( "V30, YM2151, GA20")
	MDRV_DELAYS( 500, 200 )
	MDRV_INIT( M92_Init )
	MDRV_SEND( M92_SendCmd )

	MDRV_CPU_ADD(V30, V30_CLOCK)
	MDRV_CPUMEMHAND(&m92_rw)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(IREMGA20, &iremGA20_interface)
M1_BOARD_END

