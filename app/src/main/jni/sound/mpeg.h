#ifndef _MPEG_H_
#define _MPEG_H_

#ifdef __cplusplus
extern "C" {
#endif

int MPEG_sh_start( const struct MachineSound *msound );
void MPEG_sh_stop( void );
void MPEG_Play_File(char *filename);
void MPEG_Play_Memory(char *sa, int length);
void MPEG_Set_Loop(char *loopstart, int loopend);
void MPEG_Stop_Playing(void);
int MPEG_Get_Progress(void);

#ifdef __cplusplus
}
#endif
#endif
