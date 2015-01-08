/* Konami Hot Rock / Rock n' Rage */

#include "m1snd.h"

static void RR_SendCmd(int cmda, int cmdb);

static int cmd_latch;

static READ_HANDLER( latch_r )
{
	cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
	return cmd_latch;
}

static READ_HANDLER( rockrage_VLM5030_busy_r ) 
{
	return ( VLM5030_BSY() ? 1 : 0 );
}

static WRITE_HANDLER( rockrage_speech_w ) 
{
	/* bit2 = data bus enable */
	VLM5030_RST( ( data >> 1 ) & 0x01 );
	VLM5030_ST(  ( data >> 0 ) & 0x01 );
}

static MEMORY_READ_START( rockrage_readmem_sound )
	{ 0x3000, 0x3000, rockrage_VLM5030_busy_r },	/* VLM5030 */
	{ 0x5000, 0x5000, latch_r },			/* soundlatch_r */
	{ 0x6001, 0x6001, YM2151_status_port_0_r },	/* YM 2151 */
	{ 0x7000, 0x77ff, MRA_RAM },		       	/* RAM */
	{ 0x8000, 0xffff, MRA_ROM },		       	/* ROM */
MEMORY_END

static MEMORY_WRITE_START( rockrage_writemem_sound )
	{ 0x2000, 0x2000, VLM5030_data_w }, 	       	/* VLM5030 */
	{ 0x4000, 0x4000, rockrage_speech_w },	       	/* VLM5030 */
	{ 0x6000, 0x6000, YM2151_register_port_0_w },	/* YM 2151 */
	{ 0x6001, 0x6001, YM2151_data_port_0_w },      	/* YM 2151 */
	{ 0x7000, 0x77ff, MWA_RAM },		       	/* RAM */
	{ 0x8000, 0xffff, MWA_ROM },		       	/* ROM */
MEMORY_END

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ 0 },
	{ 0 }
};

#if 1
static struct VLM5030interface vlm5030_interface =
{
	3579545,	/* 3.579545 MHz */
	60,			/* volume */
	REGION_SOUND1,	/* memory region of speech rom */
	0
};
#endif

static void RR_Init(long sr)
{
	m1snd_addToCmdQueue(0x3c);
	m1snd_addToCmdQueue(0);
	m1snd_addToCmdQueue(0x36);
	m1snd_addToCmdQueue(0);
}

M1_BOARD_START( hotrock )
	MDRV_NAME("Rock n' Rage")
	MDRV_HWDESC("M6809, YM2151, VLM5030")
	MDRV_DELAYS( 500, 50 )
	MDRV_INIT( RR_Init )
	MDRV_SEND( RR_SendCmd )

	MDRV_CPU_ADD(M6809B, 3000000)
	MDRV_CPU_MEMORY(rockrage_readmem_sound, rockrage_writemem_sound)

	MDRV_SOUND_ADD(YM2151, &ym2151_interface)
	MDRV_SOUND_ADD(VLM5030, &vlm5030_interface)
M1_BOARD_END

static void RR_SendCmd(int cmda, int cmdb)
{
	if (cmda == 0xffff) return;
	cmd_latch = cmda;
	cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
}

WRITE_HANDLER( hotchase_sound_control_w )
{
	int reg[8];

	reg[offset] = data;

	switch (offset)
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		  /* change volume
			 offset 00000xxx----- channel select (0:channel 0, 1:channel 1)
			 ++------ chip select ( 0:chip 1, 1:chip2, 2:chip3)
			 data&0x0f left volume  (data>>4)&0x0f right volume
		  */
		  K007232_set_volume( offset>>1, offset&1,  (data&0x0f) * 0x08, (data>>4) * 0x08 );
		  break;
//Shica
		case 0x06:      /* Bankswitch for chips 0 & 1 */
		{
			int bank0_a = (data >> 1) & 1;
			int bank1_a = (data >> 2) & 1;
			int bank0_b = (data >> 3) & 1;
			int bank1_b = (data >> 4) & 1;
			// bit 6: chip 2 - ch0 ?
			// bit 7: chip 2 - ch1 ?

			K007232_set_bank( 0, bank0_a, bank0_b );
			K007232_set_bank( 1, bank1_a, bank1_b );
		}
		break;

		case 0x07:      /* Bankswitch for chip 2 */
		{
			int bank2_a = (data >> 0) & 7;
			int bank2_b = (data >> 3) & 7;

			K007232_set_bank( 2, bank2_a, bank2_b );
		}
		break;
	}
}

/* Read and write handlers for one K007232 chip:
   even and odd register are mapped swapped */

#define HOTCHASE_K007232_RW(_chip_) \
READ_HANDLER( hotchase_K007232_##_chip_##_r ) \
{ \
	return K007232_read_port_##_chip_##_r(offset ^ 1); \
} \
WRITE_HANDLER( hotchase_K007232_##_chip_##_w ) \
{ \
	K007232_write_port_##_chip_##_w(offset ^ 1, data); \
} \

/* 3 x K007232 */
HOTCHASE_K007232_RW(0)
HOTCHASE_K007232_RW(1)
HOTCHASE_K007232_RW(2)

static MEMORY_READ_START( hotchase_sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM                       },      // RAM
	{ 0x1000, 0x100d, hotchase_K007232_0_r          },      // 3 x  K007232
	{ 0x2000, 0x200d, hotchase_K007232_1_r          },
	{ 0x3000, 0x300d, hotchase_K007232_2_r          },
	{ 0x6000, 0x6000, latch_r      		        },      // From main CPU (Read on IRQ)
	{ 0x8000, 0xffff, MRA_ROM                       },      // ROM
MEMORY_END

static MEMORY_WRITE_START( hotchase_sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM                       },      // RAM
	{ 0x1000, 0x100d, hotchase_K007232_0_w          },      // 3 x K007232
	{ 0x2000, 0x200d, hotchase_K007232_1_w          },
	{ 0x3000, 0x300d, hotchase_K007232_2_w          },
	{ 0x4000, 0x4007, hotchase_sound_control_w      },      // Sound volume, banking, etc.
	{ 0x5000, 0x5000, MWA_NOP                       },      // ? (written with 0 on IRQ, 1 on FIRQ)
	{ 0x7000, 0x7000, MWA_NOP                       },      // Command acknowledge ?
	{ 0x8000, 0xffff, MWA_ROM                       },      // ROM
MEMORY_END

static struct K007232_interface hotchase_k007232_interface =
{
	3,
	3579545,	/* clock */
	{ REGION_SOUND1, REGION_SOUND2, REGION_SOUND3 },
	{ K007232_VOL( 33,MIXER_PAN_CENTER, 33,MIXER_PAN_CENTER ),
	  K007232_VOL( 33,MIXER_PAN_LEFT,   33,MIXER_PAN_RIGHT  ),
	  K007232_VOL( 33,MIXER_PAN_LEFT,   33,MIXER_PAN_RIGHT  ) },
	{ 0,0,0 }
};

static void timer(int refcon)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);

	timer_set(1.0/(489.0), 0, timer);
}

static int hc_sfx[] = { 0, -1 };

static void HC_Init(long sr)
{
	timer_set(1.0/(489.0), 0, timer);

	m1snd_setCmdSuffixStr(hc_sfx);
}

static void HC_StopCmd(int cmda)
{
	m1snd_addToCmdQueueRaw(0);
	m1snd_addToCmdQueueRaw(0);
}

M1_BOARD_START( hotchase )
	MDRV_NAME("Hot Chase")
	MDRV_HWDESC("M6809, K007232(x3)")
	MDRV_DELAYS( 500, 50 )
	MDRV_INIT( HC_Init )
	MDRV_SEND( RR_SendCmd )
	MDRV_STOP( HC_StopCmd )

	MDRV_CPU_ADD(M6809B, 3579545 / 2)
	MDRV_CPU_MEMORY(hotchase_sound_readmem,hotchase_sound_writemem)

	MDRV_SOUND_ADD(K007232, &hotchase_k007232_interface)
M1_BOARD_END

