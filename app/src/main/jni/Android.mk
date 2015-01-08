LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
#LOCAL_ARM_NEON := true
#TARGET_PLATFORM = android-9
LOCAL_MODULE    := M1

LOCAL_CFLAGS = -c -O2 -fno-strict-aliasing -fPIC -D__GNU__
LOCAL_CFLAGS += -DSOUND_OUTPUT=1 -DHAS_YM2612=1 -DHAS_YM3438=1 -DHAS_YM2203=1 -DHAS_YM2610=1 -DHAS_YM2610B=1 -DINLINE="static __inline__"
LOCAL_CFLAGS += -DHAS_YM3812=1 -DHAS_YM3526=1 -DHAS_M65C02=1 -DLSB_FIRST=1 -DHAS_M6803=1 -DHAS_M6808=1 -DHAS_ADSP2105=1
LOCAL_CFLAGS += -DHAS_ES5505=1 -DHAS_ES5506=1 -DHAS_K005289=1 -DHAS_SN76496=1 -DHAS_K007232=1 -DHAS_NAMCO=1
LOCAL_CFLAGS += -DHAS_CEM3394=1 -DHAS_YMZ280B=1 -DHAS_AY8910=1 -DHAS_DAC=1 -DHAS_SEGAPCM=1 -DHAS_OKIM6295=1
LOCAL_CFLAGS += -DHAS_TMS5220=1 -DHAS_ADPCM=1 -DHAS_K051649=1 -DHAS_YM2151_ALT=1 -DHAS_RF5C68=1
LOCAL_CFLAGS += -DHAS_QSOUND=1 -DHAS_K054539=1 -DHAS_UPD7759=1 -DHAS_MULTIPCM=1 -DHAS_YMF278B=1 -DHAS_MSM5232=1
LOCAL_CFLAGS += -DHAS_K053260=1 -DHAS_POKEY=1 -DHAS_HC55516=1 -DHAS_IREMGA20=1 -DHAS_MSM5205=1 -DHAS_C140=1
LOCAL_CFLAGS += -DHAS_BSMT2000=1 -DHAS_HD63701=1 -DHAS_CUSTOM=1 -DHAS_ADSP2100=1 -DHAS_ADSP2101=1 -DHAS_ADSP2115=1
LOCAL_CFLAGS += -DHAS_YMF262=1 -DHAS_YM2413=1 -DHAS_YM2608=1 -DHAS_VLM5030=1 -DHAS_MPEG=1 -DHAS_N7751=1
LOCAL_CFLAGS += -DHAS_PIC16C54=1 -DHAS_PIC16C55=1 -DHAS_PIC16C56=1 -DHAS_PIC16C57=1 -DHAS_PIC16C58=1
LOCAL_CFLAGS += -DHAS_C352=1 -DHAS_YMF271=1 -DHAS_SCSP=1 -DHAS_Y8950=1 -DHAS_ADSP2104=1 -DPATHSEP="/" 
#-DPTR64
# non-"core" defines
LOCAL_CFLAGS += -DPS2=0 -DM1=1 -DUNIX=1 -DNDEBUG=1 -Wall 
#LOCAL_CFLAGS += -DUSE_DRZ80
LOCAL_CFLAGS += -DUSE_Z80


# for native asset manager

# for logcat
LOCAL_LDLIBS 	+= -llog


LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/cpu $(LOCAL_PATH)/cpu/drz80 $(LOCAL_PATH)/cpu/cyclone68k $(LOCAL_PATH)/sound $(LOCAL_PATH)/boards $(LOCAL_PATH)/mpeg $(LOCAL_PATH)/expat $(LOCAL_PATH)/zlib 

# m1 core objects
LOCAL_SRC_FILES = m1snd.cpp unzip.c timer.c wavelog.cpp rom.c irem_cpu.cpp
LOCAL_SRC_FILES += 6821pia.c cpuintrf.c sndintrf.c state.c taitosnd.c kabuki.cpp memory.c
LOCAL_SRC_FILES += trklist.cpp m1queue.cpp m1filter.cpp xmlout.cpp 
LOCAL_SRC_FILES += chd.c chdcd.c harddisk.c md5.c sha1.c gamelist.cpp

# MPEG decoder objects

LOCAL_SRC_FILES += mpeg/dump.c mpeg/getbits.c mpeg/getdata.c mpeg/huffman.c mpeg/layer2.c 
LOCAL_SRC_FILES += mpeg/layer3.c mpeg/misc2.c mpeg/position.c mpeg/transform.c mpeg/util.c mpeg/audio.c

# Zlib objects (avoids dynamic link, allowing use with .NET / Mono)

LOCAL_SRC_FILES += zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/gzio.c zlib/uncompr.c zlib/deflate.c zlib/trees.c
LOCAL_SRC_FILES += zlib/zutil.c zlib/inflate.c zlib/infback.c zlib/inftrees.c zlib/inffast.c

# Expat XML parser lib objects

LOCAL_SRC_FILES += expat/xmlparse.c expat/xmlrole.c expat/xmltok.c



# Boards (drivers)
LOCAL_SRC_FILES += boards/brd_raiden2.cpp boards/brd_segapcm.cpp boards/brd_taifx1.cpp boards/brd_multi32.cpp 
LOCAL_SRC_FILES += boards/brd_sys1832.cpp boards/brd_hcastle.cpp boards/brd_segamodel1.cpp boards/brd_cps1.cpp boards/brd_gradius3.cpp 
LOCAL_SRC_FILES += boards/brd_twin16.cpp boards/brd_qsound.cpp boards/brd_xexex.cpp boards/brd_bubblebobble.cpp boards/brd_parodius.cpp
LOCAL_SRC_FILES += boards/brd_namsys21.cpp boards/brd_overdrive.cpp boards/brd_contra.cpp boards/brd_gradius.cpp boards/brd_gx.cpp
LOCAL_SRC_FILES += boards/brd_gyruss.cpp boards/brd_btime.cpp boards/brd_atarisy1.cpp boards/brd_atarisy2.cpp boards/brd_itech32.cpp
LOCAL_SRC_FILES += boards/brd_f3.cpp boards/brd_gauntlet.cpp boards/brd_gng.cpp boards/brd_starwars.cpp boards/brd_mpatrol.cpp 
LOCAL_SRC_FILES += boards/brd_macrossplus.cpp boards/brd_braveblade.cpp boards/brd_s1945.cpp
LOCAL_SRC_FILES += boards/brd_dbz2.cpp boards/brd_null.cpp boards/brd_sharrier.cpp boards/brd_endurobl2.cpp
LOCAL_SRC_FILES += boards/brd_neogeo.cpp boards/brd_megasys1.cpp boards/brd_ssio.cpp
LOCAL_SRC_FILES += boards/brd_1942.cpp boards/brd_bjack.cpp boards/brd_88games.cpp boards/brd_sys16.cpp
LOCAL_SRC_FILES += boards/brd_m72.cpp boards/brd_m92.cpp boards/brd_dcs.cpp boards/brd_chipsqueakdeluxe.cpp
LOCAL_SRC_FILES += boards/brd_deco8.cpp boards/brd_scsp.cpp boards/brd_wmscvsd.cpp boards/brd_wmsadpcm.cpp 
LOCAL_SRC_FILES += boards/brd_btoads.cpp boards/brd_lemmings.cpp boards/brd_sidepck.cpp
LOCAL_SRC_FILES += boards/brd_segasys1.cpp boards/brd_atarijsa.cpp boards/brd_cavez80.cpp boards/brd_sf1.cpp
LOCAL_SRC_FILES += boards/brd_darius.cpp boards/brd_namsys1.cpp boards/brd_ms32.cpp boards/brd_sun16.cpp
LOCAL_SRC_FILES += boards/brd_frogger.cpp boards/brd_blzntrnd.cpp boards/brd_ddragon.cpp
LOCAL_SRC_FILES += boards/brd_magiccat.cpp boards/brd_raizing.cpp boards/brd_ddragon3.cpp boards/brd_tatass.cpp
LOCAL_SRC_FILES += boards/brd_aquarium.cpp boards/brd_djboy.cpp boards/brd_deco32.cpp boards/brd_skns.cpp
LOCAL_SRC_FILES += boards/brd_fcombat.cpp boards/brd_legion.cpp boards/brd_dooyong.cpp boards/brd_afega.cpp
LOCAL_SRC_FILES += boards/brd_nmk16.cpp boards/brd_namsys86.cpp boards/brd_sshang.cpp boards/brd_mappy.cpp
LOCAL_SRC_FILES += boards/brd_galaga.cpp boards/brd_airbustr.cpp boards/brd_toaplan1.cpp boards/brd_segac2.cpp
LOCAL_SRC_FILES += boards/brd_cischeat.cpp boards/brd_harddriv.cpp boards/brd_flower.cpp boards/brd_oneshot.cpp
LOCAL_SRC_FILES += boards/brd_rastan.cpp boards/brd_tecmosys.cpp boards/brd_ssys22.cpp boards/brd_tail2nose.cpp
LOCAL_SRC_FILES += boards/brd_ajax.cpp boards/brd_nslash.cpp boards/brd_njgaiden.cpp boards/brd_jedi.cpp
LOCAL_SRC_FILES += boards/brd_dsb.cpp boards/brd_wecleman.cpp boards/brd_dsbz80.cpp boards/brd_bottom9.cpp
LOCAL_SRC_FILES += boards/brd_tnzs.cpp boards/brd_rushcrash.cpp boards/brd_tecmo16.cpp boards/brd_combatsc.cpp
LOCAL_SRC_FILES += boards/brd_circusc.cpp boards/brd_bladestl.cpp boards/brd_renegade.cpp boards/brd_rygar.cpp
LOCAL_SRC_FILES += boards/brd_namh8.cpp  boards/brd_hotrock.cpp boards/brd_psychic5.cpp boards/brd_spi.cpp
LOCAL_SRC_FILES += boards/brd_fuuki32.cpp boards/brd_slapfight.cpp boards/brd_douni.cpp 
LOCAL_SRC_FILES += boards/brd_cage.cpp boards/brd_airgallet.cpp boards/brd_gott3.cpp boards/brd_hatch.cpp
LOCAL_SRC_FILES += boards/brd_psycho.cpp boards/brd_mnight.cpp boards/brd_logicpro.cpp boards/brd_gladiator.cpp
LOCAL_SRC_FILES += boards/brd_thunder.cpp boards/brd_taitosj.cpp boards/brd_beatmania.cpp boards/brd_pizza.cpp
LOCAL_SRC_FILES += boards/brd_rallyx.cpp boards/brd_yunsun.cpp boards/brd_bbusters.cpp boards/brd_snk68k.cpp
LOCAL_SRC_FILES += boards/brd_buggyboy.cpp boards/brd_spacegun.cpp boards/brd_hyperduel.cpp
LOCAL_SRC_FILES += boards/brd_equites.cpp boards/brd_taito84.cpp boards/brd_tatsumi.cpp boards/brd_namcona.cpp
LOCAL_SRC_FILES += boards/brd_genesis.cpp boards/brd_jaleco.cpp boards/brd_panicr.cpp boards/brd_mitchell.cpp
LOCAL_SRC_FILES += boards/brd_arkanoid.cpp boards/brd_hexion.cpp

# Sound cores
LOCAL_SRC_FILES += sound/fm.c sound/multipcm.c sound/scsp.c sound/segapcm.c sound/scspdsp.c
LOCAL_SRC_FILES += sound/ym2151.c sound/rf5c68.c sound/ay8910.c sound/ymdeltat.c sound/fmopl.c
LOCAL_SRC_FILES += sound/k054539.c sound/k053260.c sound/ymf278b.c sound/c140.c sound/tms57002.c 
LOCAL_SRC_FILES += sound/upd7759.c sound/samples.c sound/dac.c sound/pokey.c sound/es5506.c
LOCAL_SRC_FILES += sound/adpcm.c sound/k007232.c sound/qsound.c sound/msm5205.c sound/tms5220.c
LOCAL_SRC_FILES += sound/5220intf.c sound/iremga20.c sound/streams.c sound/hc55516.c
LOCAL_SRC_FILES += sound/bsmt2000.c sound/k005289.c sound/sn76496.c sound/namco.c sound/cem3394.c
LOCAL_SRC_FILES += sound/ymz280b.c sound/2203intf.c sound/2610intf.c sound/2612intf.c sound/3812intf.c
LOCAL_SRC_FILES += sound/k051649.c sound/2151intf.c sound/flower.c sound/ym2413.c sound/2413intf.c
LOCAL_SRC_FILES += sound/2608intf.c sound/vlm5030.c sound/262intf.c sound/ymf262.c sound/c352.c sound/ymf271.c
LOCAL_SRC_FILES += sound/dmadac.c sound/rf5c400.c sound/msm5232.c

# CPU cores
LOCAL_SRC_FILES += cpu/m68kcpu.c cpu/m68kops.c
LOCAL_SRC_FILES += cpu/m6800.c cpu/m6809.c cpu/m6502.c cpu/h6280.c cpu/i8039.c cpu/nec.c
LOCAL_SRC_FILES += cpu/adsp2100.c cpu/m37710.c cpu/m37710o0.c cpu/m37710o1.c
LOCAL_SRC_FILES += cpu/m37710o2.c cpu/m37710o3.c cpu/hd6309.c cpu/tms32010.c cpu/pic16c5x.c
LOCAL_SRC_FILES += cpu/h83002.c cpu/h8periph.c cpu/tms32031.c cpu/2100dasm.c
LOCAL_SRC_FILES += cpu/i8085.c
LOCAL_SRC_FILES += cpu/z80.c 

# asm cores
#LOCAL_SRC_FILES += cpu/drz80/drz80.S cpu/drz80/drz80_z80.cpp
#LOCAL_SRC_FILES += cpu/cyclone68k/cyclone.S cpu/cyclone68k/c68000.cpp

# android stuff
LOCAL_SRC_FILES += M1Native.c
LOCAL_SRC_FILES += m1sdr_android.cpp 

include $(BUILD_SHARED_LIBRARY)
