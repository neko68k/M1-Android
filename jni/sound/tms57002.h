#ifndef __TMS57002
#define __TMS57002

void tms57002_init(void);

READ_HANDLER( tms57002_data_r );
READ_HANDLER( tms57002_status_r );
WRITE_HANDLER( tms57002_control_w );
WRITE_HANDLER( tms57002_data_w );

READ16_HANDLER( tms57002_data_word_r );
READ16_HANDLER( tms57002_status_word_r );
WRITE16_HANDLER( tms57002_control_word_w );
WRITE16_HANDLER( tms57002_data_word_w );

#endif
