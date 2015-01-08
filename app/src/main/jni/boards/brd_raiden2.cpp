// Seibu sound system stuff

#include "m1snd.h"

static int decrypt_offs;

enum
{
	VECTOR_INIT,
	RST10_ASSERT,
	RST10_CLEAR,
	RST18_ASSERT,
	RST18_CLEAR
};

static void update_irq_lines(int param)
{
	static int irq1,irq2;

	switch(param)
	{
		case VECTOR_INIT:
			irq1 = irq2 = 0xff;
			return;
			break;

		case RST10_ASSERT:
			irq1 = 0xd7;
			break;

		case RST10_CLEAR:
			irq1 = 0xff;
			break;

		case RST18_ASSERT:
			irq2 = 0xdf;
			break;

		case RST18_CLEAR:
			irq2 = 0xff;
			break;
	}

	if ((irq1 & irq2) == 0xff)	/* no IRQs pending */
	{
//		printf("clear\n");
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
	else	/* IRQ pending */
	{
		cpu_set_irq_line(0, 0, CLEAR_LINE);

		if (irq1 != 0xff)
		{
//			printf("RST10\n");
			z80_set_irqvec(Z80_RST_10);
			cpu_set_irq_line(0, 0, ASSERT_LINE);
		}
		else
		{
//			printf("RST18\n");
			z80_set_irqvec(Z80_RST_18);
			cpu_set_irq_line(0, 0, ASSERT_LINE);
		}
	}
}

static void irq_handler(int irq)
{
//	printf("YMIRQ %d\n", irq);
	if (irq)
	{
		update_irq_lines(RST10_ASSERT);
	}
	else
	{
		update_irq_lines(RST10_CLEAR);
	}
}

static struct YM3812interface ym3812_interface =
{																	
	1,
	3579545,
	{ 85 },
	{ irq_handler },
};					

static struct YM2151interface ym2151_interface =					
{																	
	1,																
	3579545,
	{ YM3012_VOL(70,MIXER_PAN_LEFT,70,MIXER_PAN_RIGHT) },
	{ irq_handler },
};																	
																	
static struct OKIM6295interface okim6295_interface =
{																	
	2,
	{ 8000, 8000 },
	{ RGN_SAMP1, RGN_SAMP2 },														
	{ 40, 40 }			
};

static struct OKIM6295interface LGokim6295_interface =
{																	
	2,
	{ 8000, 8000 },
	{ RGN_SAMP1, RGN_SAMP2 },														
	{ 35, 35 }
};

static struct OKIM6295interface R1okim6295_interface =
{																	
	2,
	{ 8000, 8000 },
	{ RGN_SAMP1, RGN_SAMP2 },														
	{ 60, 60 }			
};

static UINT8 main2sub[2],sub2main[2];
static int main2sub_pending,sub2main_pending;

WRITE_HANDLER( seibu_bank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	if (Machine->refcon == 1)
	{
		rom += 0x8000;

		if (data & 1)
		{
			cpu_setbank(1, rom + 0x8000);
		}
		else
		{
			cpu_setbank(1, rom);
		}
	}
	else
	{
		if (data & 1)
		{
			cpu_setbank(1, rom);
		}
		else
		{
			cpu_setbank(1, rom + 0x8000);
		}
	}
}

WRITE_HANDLER( seibu_coin_w )
{
}

READ_HANDLER( seibu_soundlatch_r )
{
//	printf("Rd %x latch ofs %x\n", main2sub[offset], offset);
	return main2sub[offset];
}

READ_HANDLER( seibu_main_data_pending_r )
{
	return 0; //sub2main_pending ? 1 : 0;
}

WRITE_HANDLER( seibu_main_data_w )
{
	sub2main[offset] = data;
}

WRITE_HANDLER( seibu_pending_w )
{
	/* just a guess */
	main2sub_pending = 0;
	sub2main_pending = 1;
}

WRITE_HANDLER( seibu_irq_clear_w )
{
}

WRITE_HANDLER( seibu_rst10_ack_w )
{
}

WRITE_HANDLER( seibu_rst18_ack_w )
{
//	printf("RST18 clear\n");
	update_irq_lines(RST18_CLEAR);
}

static READ_HANDLER( ss_readop )
{
	return prgrom[offset + decrypt_offs];
}

MEMORY_READ_START( seibu_sound_readop )
	{ 0x0000, 0x1fff, ss_readop },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

MEMORY_READ_START( seibu_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4008, 0x4008, YM3812_status_port_0_r },
	{ 0x4010, 0x4011, seibu_soundlatch_r },
	{ 0x4012, 0x4012, seibu_main_data_pending_r },
	{ 0x4013, 0x4013, MRA_NOP },
	{ 0x6000, 0x6000, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

MEMORY_WRITE_START( seibu_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x4000, seibu_pending_w },
	{ 0x4001, 0x4001, seibu_irq_clear_w },
	{ 0x4002, 0x4002, seibu_rst10_ack_w },
	{ 0x4003, 0x4003, seibu_rst18_ack_w },
	{ 0x4007, 0x4007, seibu_bank_w },
	{ 0x4008, 0x4008, YM3812_control_port_0_w },
	{ 0x4009, 0x4009, YM3812_write_port_0_w },
	{ 0x4018, 0x4019, seibu_main_data_w },
	{ 0x401a, 0x401a, MWA_NOP },
	{ 0x401b, 0x401b, seibu_coin_w },
	{ 0x6000, 0x6000, OKIM6295_data_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MEMORY_READ_START( seibu2_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4009, 0x4009, YM2151_status_port_0_r },
	{ 0x4010, 0x4011, seibu_soundlatch_r },
	{ 0x4012, 0x4012, seibu_main_data_pending_r },
	{ 0x4013, 0x4013, MRA_NOP },	// input port
	{ 0x6000, 0x6000, OKIM6295_status_0_r },
	{ 0x6002, 0x6002, OKIM6295_status_1_r },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

MEMORY_WRITE_START( seibu2_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4000, 0x4000, seibu_pending_w },
	{ 0x4001, 0x4001, seibu_irq_clear_w },
	{ 0x4002, 0x4002, seibu_rst10_ack_w },
	{ 0x4003, 0x4003, seibu_rst18_ack_w },
	{ 0x4004, 0x4004, MWA_NOP },	// written once on init
	{ 0x4007, 0x4007, MWA_NOP },
	{ 0x4008, 0x4008, YM2151_register_port_0_w },
	{ 0x4009, 0x4009, YM2151_data_port_0_w },
	{ 0x4018, 0x4019, seibu_main_data_w },
	{ 0x401a, 0x401a, seibu_bank_w },	// watchdog?
	{ 0x401b, 0x401b, seibu_coin_w },
	{ 0x6000, 0x6000, OKIM6295_data_0_w },
	{ 0x6002, 0x6002, OKIM6295_data_1_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

#define BIT(x,n) (((x)>>(n))&1)

static UINT8 decrypt_data(int a,int src)
{
	if ( BIT(a,9)  &  BIT(a,8))             src ^= 0x80;
	if ( BIT(a,11) &  BIT(a,4) &  BIT(a,1)) src ^= 0x40;
	if ( BIT(a,11) & ~BIT(a,8) &  BIT(a,1)) src ^= 0x04;
	if ( BIT(a,13) & ~BIT(a,6) &  BIT(a,4)) src ^= 0x02;
	if (~BIT(a,11) &  BIT(a,9) &  BIT(a,2)) src ^= 0x01;

	if (BIT(a,13) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,3,2,0,1);
	if (BIT(a, 8) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,2,3,1,0);

	return src;
}

static UINT8 decrypt_opcode(int a,int src)
{
	if ( BIT(a,9)  &  BIT(a,8))             src ^= 0x80;
	if ( BIT(a,11) &  BIT(a,4) &  BIT(a,1)) src ^= 0x40;
	if (~BIT(a,13) & BIT(a,12))             src ^= 0x20;
	if (~BIT(a,6)  &  BIT(a,1))             src ^= 0x10;
	if (~BIT(a,12) &  BIT(a,2))             src ^= 0x08;
	if ( BIT(a,11) & ~BIT(a,8) &  BIT(a,1)) src ^= 0x04;
	if ( BIT(a,13) & ~BIT(a,6) &  BIT(a,4)) src ^= 0x02;
	if (~BIT(a,11) &  BIT(a,9) &  BIT(a,2)) src ^= 0x01;

	if (BIT(a,13) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,3,2,0,1);
	if (BIT(a, 8) &  BIT(a,4)) src = BITSWAP8(src,7,6,5,4,2,3,1,0);
	if (BIT(a,12) &  BIT(a,9)) src = BITSWAP8(src,7,6,4,5,3,2,1,0);
	if (BIT(a,11) & ~BIT(a,6)) src = BITSWAP8(src,6,7,5,4,3,2,1,0);

	return src;
}

void seibu_sound_decrypt(int cpu_region,int length)
{
	UINT8 *rom = memory_region(cpu_region);
	int diff =  length;
	int i;

	decrypt_offs = diff;

	for (i = 0;i < length;i++)
	{
		UINT8 src = rom[i];

		rom[i]      = decrypt_data(i,src);
		rom[i+diff] = decrypt_opcode(i,src);
	}
}

static void RD2_Init(long srate)
{
	decrypt_offs = 0;

	if (Machine->refcon == 1)
	{
		seibu_sound_decrypt(REGION_CPU1, 0x2000);
	}
	if (Machine->refcon == 2)
	{
		seibu_sound_decrypt(REGION_CPU1, 0x20000);
	}

	seibu_bank_w(0, 0);
	update_irq_lines(VECTOR_INIT);
}

static void RD2_SendCmd(int cmda, int cmdb)
{
	static int funcno[]={0x80,0x81,0x84,0x82,0xb0,0xb1,0xa0,0xa1,0xd0,0x90};

	main2sub[0]=cmda&255;
	main2sub[1]=funcno[cmda>>8];
	sub2main_pending = 0;

	update_irq_lines(RST18_ASSERT);
}

static void RD_SendCmd(int cmda, int cmdb)
{
	static int funcno[]={0x80,0x84,0x81,0x82,0xb0,0xb1,0xa0,0x90};

	main2sub[0]=cmda&255;
	main2sub[1]=funcno[cmda>>8];
	sub2main_pending = 0;

	update_irq_lines(RST18_ASSERT);
}

M1_BOARD_START( seiburai2 )
	MDRV_NAME( "Raiden II" )
	MDRV_HWDESC( "Z80, YM2151, MSM-6295(x2)")
	MDRV_DELAYS( 240, 15 )
	MDRV_INIT( RD2_Init )
	MDRV_SEND( RD2_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY( seibu2_sound_readmem, seibu2_sound_writemem )

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, &okim6295_interface)
M1_BOARD_END

M1_BOARD_START( raiden )
	MDRV_NAME( "Raiden" )
	MDRV_HWDESC( "Z80, YM3812, MSM-6295")
	MDRV_DELAYS( 240, 15 )
	MDRV_INIT( RD2_Init )
	MDRV_SEND( RD_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY( seibu_sound_readmem, seibu_sound_writemem )
	MDRV_CPU_READOP( seibu_sound_readop )

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, &R1okim6295_interface)
M1_BOARD_END

M1_BOARD_START( legionnaire )
	MDRV_NAME( "Legionnaire" )
	MDRV_HWDESC( "Z80, YM3812, MSM-6295")
	MDRV_DELAYS( 240, 15 )
	MDRV_INIT( RD2_Init )
	MDRV_SEND( RD_SendCmd )

	MDRV_CPU_ADD(Z80C, 4000000)
	MDRV_CPU_MEMORY( seibu_sound_readmem, seibu_sound_writemem )

	MDRV_SOUND_ADD(YM3812, &ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, &LGokim6295_interface)
M1_BOARD_END

