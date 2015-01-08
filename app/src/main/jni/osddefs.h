#ifndef _OSDDEFS_H_
#define _OSDDEFS_H_

/* platform specific stuff */

#if (__linux__ || WIN32)
typedef signed   char      INT8;
typedef signed   short     INT16;
typedef signed   int       INT32;
#ifdef PTR64
typedef signed   long      INT64;
#else
typedef signed   long long INT64;
#endif
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
#ifdef PTR64
typedef unsigned long 	   UINT64;
#else
typedef unsigned long long UINT64;
#endif

#endif

#if (MACOS)
typedef signed   char      INT8;
typedef signed   short     INT16;
typedef signed   int       INT32;
typedef signed   long long INT64;
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;

#endif

#endif
