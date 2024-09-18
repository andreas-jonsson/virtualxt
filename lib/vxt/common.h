// Copyright (c) 2019-2024 Andreas T Jonsson <mail@andreasjonsson.se>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    This product make use of the VirtualXT software emulator.
//    Visit https://virtualxt.org for more information.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#ifndef _COMMON_H_
#define _COMMON_H_

#include <vxt/vxt.h>

#define INT64 long long
#define UINT64 unsigned long long

_Static_assert(sizeof(INT64) == 8 && sizeof(UINT64) == 8, "invalid (u)int64 integer size");

#ifdef _MSC_VER
   #define LIKELY(x) (x)
   #define UNLIKELY(x) (x)
#else
   #define LIKELY(x) __builtin_expect((x), 1)
   #define UNLIKELY(x) __builtin_expect((x), 0)
#endif

#ifdef VXT_NO_LIBC
   #define ABORT() { static volatile int *_null = NULL; *_null = 0; for(;;); }
#else
   #define ABORT() { abort(); }
#endif

// Host to 8088 endian conversion.
#if !defined(VXT_BIGENDIAN) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
   #define HBYTE(w)     ((vxt_byte)(((vxt_word)(w))>>8))
   #define LBYTE(w)     ((vxt_byte)(((vxt_word)(w))&0xFF))
   #define WORD(h, l)   ((((vxt_word)((vxt_byte)(h)))<<8)|((vxt_word)((vxt_byte)(l))))
#else
   #error Bigendian is not supported!
#endif

#define UNUSED(v) ( (void)(v) )

#define CONSTP(t)    t * const
#define CONSTSP(t)   struct CONSTP(t)

#define PRINT(...)         { VXT_PRINT(__VA_ARGS__); VXT_PRINT("\n"); }
#define ENSURE(e)          if (UNLIKELY(!(e))) { VXT_PRINT("( " #e " ) "); ABORT(); }
#define ASSERT(e, ...)     if (UNLIKELY(!(e))) { VXT_PRINT("( " #e " ) "); PRINT(__VA_ARGS__); ABORT(); }
#define PANIC(...)         { PRINT(__VA_ARGS__); ABORT(); }
#define UNREACHABLE(...)   { PANIC("unreachable"); return __VA_ARGS__; }

#endif
