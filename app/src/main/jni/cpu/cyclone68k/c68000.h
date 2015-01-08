#ifndef C68000_H
#define C68000_H

extern int *cyclone_cycles;
#define cyclone_ICount (*cyclone_cycles)

#define cyclone_INT_NONE 0							  
#define cyclone_IRQ_1    1
#define cyclone_IRQ_2    2
#define cyclone_IRQ_3    3
#define cyclone_IRQ_4    4
#define cyclone_IRQ_5    5
#define cyclone_IRQ_6    6
#define cyclone_IRQ_7    7
#define cyclone_INT_ACK_AUTOVECTOR    -1
#define cyclone_STOP     0x10

#define M68K_IRQ_1 cyclone_IRQ_1
#define M68K_IRQ_2 cyclone_IRQ_2
#define M68K_IRQ_3 cyclone_IRQ_3
#define M68K_IRQ_4 cyclone_IRQ_4
#define M68K_IRQ_5 cyclone_IRQ_5
#define M68K_IRQ_6 cyclone_IRQ_6
#define M68K_IRQ_7 cyclone_IRQ_7
#define M68K_IRQ_NONE cyclone_INT_NONE

enum
{
	/* NOTE: M68K_SP fetches the current SP, be it USP, ISP, or MSP */
	M68K_PC=1, M68K_SP, M68K_ISP, M68K_USP, M68K_MSP, M68K_SR, M68K_VBR,
	M68K_SFC, M68K_DFC, M68K_CACR, M68K_CAAR, M68K_PREF_ADDR, M68K_PREF_DATA,
	M68K_D0, M68K_D1, M68K_D2, M68K_D3, M68K_D4, M68K_D5, M68K_D6, M68K_D7,
	M68K_A0, M68K_A1, M68K_A2, M68K_A3, M68K_A4, M68K_A5, M68K_A6, M68K_A7
};

#define M68K_REG_SP M68K_SP
#define M68K_REG_PC M68K_PC

extern void cyclone_reset(void);                      
extern int  cyclone_execute(int cycles);                     
extern void cyclone_set_context(void *src);
extern unsigned cyclone_get_context(void *dst);
extern unsigned int cyclone_get_pc(void);                      
extern void cyclone_exit(void);
extern void cyclone_set_pc(unsigned val);
extern unsigned cyclone_get_sp(void);
extern void cyclone_set_sp(unsigned val);
extern unsigned cyclone_get_reg(int regnum);
extern void cyclone_set_reg(int regnum, unsigned val);
extern void cyclone_set_nmi_line(int state);
extern void cyclone_set_irq_line(int irqline, int state);
extern void cyclone_set_irq_callback(int (*callback)(int irqline));
extern const char *cyclone_info(void *context, int regnum);
extern unsigned cyclone_dasm(char *buffer, unsigned pc);

#define m68k_execute(x) cyclone_execute(x)
#define m68k_set_irq(x) cyclone_set_irq_line(x, ASSERT_LINE)
//#define m68k_get_reg(x, y) NULL
#define m68k_get_reg(x, y) cyclone_get_reg(y)
#define m68k_set_reg(x, y) cyclone_set_reg(x, y)

void cyclone_yield(void);
int cyclone_getcycles(void);
#endif
