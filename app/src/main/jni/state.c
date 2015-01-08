/* State save/load stubs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "driver.h"
#include "zlib.h"

void state_save_register_func_presave(void (*func)(void))
{
}

void state_save_register_func_postload(void (*func)(void))
{
}

void state_save_register_UINT8 (const char *module, int instance,
								const char *name, UINT8 *val, unsigned size)
{
}

void state_save_register_INT8  (const char *module, int instance,
								const char *name, INT8 *val, unsigned size)
{
}

void state_save_register_UINT16(const char *module, int instance,
								const char *name, UINT16 *val, unsigned size)
{
}

void state_save_register_INT16 (const char *module, int instance,
								const char *name, INT16 *val, unsigned size)
{
}

void state_save_register_UINT32(const char *module, int instance,
								const char *name, UINT32 *val, unsigned size)
{
}

void state_save_register_INT32 (const char *module, int instance,
								const char *name, INT32 *val, unsigned size)
{
}

void state_save_register_int   (const char *module, int instance,
								const char *name, int *val)
{
}

void state_save_register_double(const char *module, int instance,
								const char *name, double *val, unsigned size)
{
}

