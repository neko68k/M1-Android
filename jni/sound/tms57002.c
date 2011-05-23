#include "driver.h"
#include "tms57002.h"

/* Accesses (fsizes = 118, 262, 756)
   400001 = c0
   400003 = ff
   40000b = 44
   580001 = 0 then 2 (reset ?)

   write() {
     control = ec, e8, f8
     write firmware to data
     control = fc, f0
     write tables to data
     control = fc
   }
   check() {
     wait !(status & 4) || timeout
     read data 4 times
     if(last != 0)
       yell
   }

   write(firm1)
   check()
   write(firm2)
   check()
   write(firm3)

   n times {
     wait status & 1
     control = f4
     400009 = 0
     write addr
     4*write data
     control = ff
     400009 = 1
   }

   400009 = 00
   400005 = a5
   400007 = 53
   400009 = 01
   control = ff
   400009 = 01

 */

static struct {
  unsigned char control;
  unsigned char program[1024];
  unsigned int tables[256];
  int curpos;
  int bytepos;
  unsigned int tmp;

} tms57002;

void tms57002_init(void)
{
  tms57002.control = 0;
}

int ldw = 0;

static void chk_ldw(void)
{
//  if(ldw > 1)
//    fprintf(stderr, "tms57002:    -> wrote %d bytes\n", ldw);
  ldw = 0;
}

WRITE_HANDLER( tms57002_control_w )
{
  chk_ldw();

  switch(tms57002.control) {
  case 0xf8: {
/*    int i, j;
    fprintf(stderr, "Firmware loaded:\n");
    for(i=0; i<tms57002.curpos; i+=16) {
      fprintf(stderr, "   %04x:", i);
      for(j=0; j<16 && i+j<tms57002.curpos; j++)
	fprintf(stderr, " %02x", tms57002.program[i+j]);
      fprintf(stderr, "\n");
    }*/
    break;
  }
  case 0xf0: {
/*    int i, j;
    fprintf(stderr, "Table loaded:\n");
    for(i=0; i<256; i+=8) {
      fprintf(stderr, "   %02x:", i);
      for(j=0; j<8 && i+j<256; j++)
	fprintf(stderr, " %08x", tms57002.tables[i+j]);
      fprintf(stderr, "\n");
    }			    */
    break;
  }
  }

  tms57002.control = data;
  switch(data) {
  case 0xf8: // Program write
  case 0xf0: // Table write
    tms57002.curpos = 0;
    tms57002.bytepos = 3;
    tms57002.tmp = 0;
    break;
  case 0xf4: // Entry write
    tms57002.curpos = -1;
    tms57002.bytepos = 3;
    tms57002.tmp = 0;
    break;
  case 0xfc: // Checksum (?) Status (?)
    tms57002.bytepos = 3;
    tms57002.tmp = 0;
    break;
  case 0xff: // Standby
    break;
  case 0xfe: // Irq
/* (place ack of timer IRQ here) */
    break;
  default:
//    fprintf(stderr, "tms57002:  control write %02x\n", data);
    break;
  }

}

READ_HANDLER( tms57002_status_r )
{
  chk_ldw();
//  if(tms57002.control != 0xfc && tms57002.control != 0xff)
//    fprintf(stderr, "tms57002:  status read\n");
  return 0x01;
}


WRITE_HANDLER( tms57002_data_w )
{
  switch(tms57002.control) {
  case 0xf8:
    tms57002.program[tms57002.curpos++] = data;
    break;
  case 0xf0:
    tms57002.tmp |= data << (8*tms57002.bytepos);
    tms57002.bytepos--;
    if(tms57002.bytepos < 0) {
      tms57002.bytepos = 3;
      tms57002.tables[tms57002.curpos++] =  tms57002.tmp;
      tms57002.tmp = 0;
    }
    break;
  case 0xf4:
    if(tms57002.curpos == -1)
      tms57002.curpos = data;
    else {
      tms57002.tmp |= data << (8*tms57002.bytepos);
      tms57002.bytepos--;
      if(tms57002.bytepos < 0) {
	tms57002.bytepos = 3;
//	fprintf(stderr, "Table patched %02x: %08x\n", tms57002.curpos, tms57002.tmp);
	
	tms57002.tables[tms57002.curpos] =  tms57002.tmp;
	tms57002.tmp = 0;
	tms57002.curpos = -1;
      }
    }
    break;
  default:
//    if(!ldw)
//      fprintf(stderr, "tms57002:  data write %02x\n", data);
    ldw++;
    break;
  }
}

READ_HANDLER( tms57002_data_r )
{
  unsigned char res;
  chk_ldw();
  switch(tms57002.control) {
  case 0xfc:
    res = tms57002.tmp >> (8*tms57002.bytepos);
    tms57002.bytepos--;
    if(tms57002.bytepos < 0)
      tms57002.bytepos = 3;
    return res;
  }
//  fprintf(stderr, "tms57002:  data read (%02x)\n", tms57002.control);
  return 0;
}


READ16_HANDLER( tms57002_data_word_r )
{
	return(tms57002_data_r(0));
}

READ16_HANDLER( tms57002_status_word_r )
{
	return(tms57002_status_r(0));
}

WRITE16_HANDLER( tms57002_control_word_w )
{
	tms57002_control_w(0, data);
}

WRITE16_HANDLER( tms57002_data_word_w )
{
	tms57002_data_w(0, data);
}

