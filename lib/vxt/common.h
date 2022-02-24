/*
Copyright (c) 2019-2022 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <vxt/vxt.h>

#ifdef _MSC_VER
   #define LIKELY(x) (x)
   #define UNLIKELY(x) (x)
#else
   #define LIKELY(x) __builtin_expect((x), 1)
   #define UNLIKELY(x) __builtin_expect((x), 0)
#endif

#ifdef VXT_LIBC
   #include <string.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <stddef.h>
   #include <assert.h>

   #define memclear(p, s) (memset((p), 0, (int)(s)))
   #define ABORT() { assert(0); } // { abort(); }
#else
   typedef unsigned long long int uintptr_t;

   static void memclear(void *p, int s) {
      vxt_byte *bp = (vxt_byte*)p;
      for (int i = 0; i < s; i++)
         bp[i] = 0;
   }

   static void ABORT(void) {
      for(;;);
   }
#endif

// Host to 8088 endian conversion.
#if !defined(VXT_BIGENDIAN) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
   #define HBYTE(w)     ((vxt_byte)(((vxt_word)(w))>>8))
   #define LBYTE(w)     ((vxt_byte)(((vxt_word)(w))&0xFF))
   #define WORD(h, l)   ((((vxt_word)((vxt_byte)(h)))<<8)|((vxt_word)((vxt_byte)(l))))
#else
   #error Bigendian is not supported!
#endif

#define _LOG(...)    { logger(__VA_ARGS__); }
#define LOG(...)     { _LOG(__VA_ARGS__); _LOG("\n"); }

#define UNUSED(v)    ( (void)(v) )
#define CONSTP(t)    t * const
#define CONSTSP(t)   struct CONSTP(t)

#define ENSURE(e)          if (!(e)) { _LOG("( " #e " ) "); ABORT(); }
#define ASSERT(e, ...)     if (!(e)) { _LOG("( " #e " ) "); LOG(__VA_ARGS__); ABORT(); }
#define PANIC(...)         { LOG(__VA_ARGS__); ABORT(); }
#define UNREACHABLE(...)   { PANIC("unreachable"); return __VA_ARGS__; }

extern int (*logger)(const char*, ...);

#endif
