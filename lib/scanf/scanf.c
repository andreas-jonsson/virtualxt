/*

scanf implementation
Copyright (C) 2021 Sampo Hippeläinen (hisahi)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <limits.h>
#include <stddef.h>

/* =============================== *
 *        defines &  checks        *
 * =============================== */

/* C99/C++11 */
#ifndef SCANF_C99
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
    || (defined(__cplusplus) && __cplusplus >= 201103L)
#define SCANF_C99 1
#else
#define SCANF_C99 0
#endif
#endif

/* long long? */
#ifndef LLONG_MAX
#undef SCANF_DISABLE_SUPPORT_LONG_LONG
#define SCANF_DISABLE_SUPPORT_LONG_LONG 1
#endif

/* stdint.h? */
#ifndef SCANF_STDINT
#if SCANF_C99
#define SCANF_STDINT 1
#elif HAVE_STDINT_H
#define SCANF_STDINT 1
#endif
#endif

#if SCANF_STDINT
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif
#include <stdint.h>
#if SCANF_DISABLE_SUPPORT_LONG_LONG && defined(UINTMAX_MAX)                    \
    && UINTMAX_MAX == ULLONG_MAX
/* replace (u)intmax_t with (unsigned) long if long long is disabled */
#undef intmax_t
#define intmax_t long
#undef uintmax_t
#define uintmax_t unsigned long
#undef INTMAX_MIN
#define INTMAX_MIN LONG_MIN
#undef INTMAX_MAX
#define INTMAX_MAX LONG_MAX
#undef UINTMAX_MAX
#define UINTMAX_MAX ULONG_MAX
#endif
#else
/* (u)intmax_t, (U)INTMAX_(MIN_MAX) */
#if SCANF_C99 && !SCANF_DISABLE_SUPPORT_LONG_LONG
#ifndef intmax_t
#define intmax_t long long int
#endif
#ifndef uintmax_t
#define uintmax_t unsigned long long int
#endif
#ifndef INTMAX_MIN
#define INTMAX_MIN LLONG_MIN
#endif
#ifndef INTMAX_MAX
#define INTMAX_MAX LLONG_MAX
#endif
#ifndef UINTMAX_MAX
#define UINTMAX_MAX ULLONG_MAX
#endif
#else
#ifndef intmax_t
#define intmax_t long int
#endif
#ifndef uintmax_t
#define uintmax_t unsigned long int
#endif
#ifndef INTMAX_MIN
#define INTMAX_MIN LONG_MIN
#endif
#ifndef INTMAX_MAX
#define INTMAX_MAX LONG_MAX
#endif
#ifndef UINTMAX_MAX
#define UINTMAX_MAX ULONG_MAX
#endif
#endif
#endif

/* maximum precision floating point type */
#ifndef floatmax_t
#define floatmax_t long double
#endif

#ifndef INLINE
#if SCANF_C99
#define INLINE inline
#else
#define INLINE
#endif
#endif

/* boolean type */
#ifndef BOOL
#if defined(__cplusplus)
#define BOOL bool
#elif SCANF_C99
#define BOOL _Bool
#else
#define BOOL char
#endif
#endif

/* freestanding? */
#ifndef SCANF_FREESTANDING
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 0
#define SCANF_FREESTANDING 1
#else
#define SCANF_FREESTANDING 0
#endif
#endif

#ifndef SCANF_NOMATH
#define SCANF_NOMATH SCANF_FREESTANDING
#endif

#ifndef SCANF_INTERNAL_CTYPE
#define SCANF_INTERNAL_CTYPE SCANF_FREESTANDING
#endif

#ifndef SCANF_BINARY
#define SCANF_BINARY 1
#endif

/* this is technically UB, as the character encodings used by the preprocessor
   and the actual compiled code might be different. in practice this is hardly
   ever an issue, and if it is, you can just define SCANF_ASCII manually */
#ifndef SCANF_ASCII
#if 'B' - 'A' ==  1 && 'K' - 'A' == 10 && 'Z' - 'A' == 25                      \
 && 'a' - 'A' == 32 && 'n' - 'N' == 32 && 'v' - 'V' == 32 && 'z' - 'Z' == 32   \
 && '3' - '0' ==  3 && '9' - '0' ==  9
#define SCANF_ASCII 1
#else
#define SCANF_ASCII 0
#endif
#endif

#ifndef SCANF_FAST_SCANSET
#if CHAR_BIT == 8
#define SCANF_FAST_SCANSET 1
#else
#define SCANF_FAST_SCANSET 0
#endif
#endif

/* fast scanset only supported with multibyte/narrow characters */
#undef SCANF_CAN_FAST_SCANSET
#if SCANF_WIDE
#define SCANF_CAN_FAST_SCANSET 0
#else
#define SCANF_CAN_FAST_SCANSET SCANF_FAST_SCANSET
#endif

#ifndef SCANF_NOPOW
#define SCANF_NOPOW 1
#endif

#ifndef SCANF_LOGN_POW
#define SCANF_LOGN_POW 1
#endif

/* wide setup */
#ifndef SCANF_WIDE
#define SCANF_WIDE 0
#endif

#if SCANF_WIDE < 0 || SCANF_WIDE > 3
#error invalid value for SCANF_WIDE (valid: 0, 1, 2, 3)
#endif

#if SCANF_WIDE == 3
#define SCANF_WIDE_CONVERT 1
#endif

#define SCANF_NODEFINE 1
#if SCANF_WIDE
/* also includes wide.h */
#include "wscanf.h"
#else
#include "scanf.h"
#endif

/* include more stuff */
#if !SCANF_INTERNAL_CTYPE
#if SCANF_WIDE
#include <wctype.h>
#define isdigitw_ iswdigit
#define isspacew_ iswspace
#define isalnumw_ iswalnum
#define isalphaw_ iswalpha
#define tolowerw_ towlower
#else
#include <ctype.h>
#endif
#endif

/* type & literal defines */
#undef C_
#undef S_
#undef F_
#undef CHAR
#undef UCHAR
#undef CINT

#if SCANF_WIDE
/* character */
#ifdef MAKE_WCHAR
#define C_(x) MAKE_WCHAR(x)
#else
#define C_(x) L##x
#endif
/* string */
#ifdef MAKE_WSTRING
#define S_(x) MAKE_WSTRING(x)
#else
#define S_(x) L##x
#endif
/* function */
#define F_(x) x##w_
#define CHAR WCHAR
#define UCHAR WCHAR
#define CINT WINT
#else
/* character */
#define C_(x) x
/* string */
#define S_(x) x
/* function */
#define F_(x) x
#define CHAR char
#define UCHAR unsigned char
#define CINT int
#endif

#if !SCANF_DISABLE_SUPPORT_FLOAT
#include <float.h>
#endif

#if !SCANF_NOMATH
#include <math.h>
#endif

#ifndef SCANF_INFINITE
#if SCANF_C99 && !SCANF_NOMATH && defined(NAN) && defined(INFINITY)
#define SCANF_INFINITE 1
#else
#define SCANF_INFINITE 0
#endif
#endif

#if SCANF_ASCII
#define INLINE_IF_ASCII INLINE
#else
#define INLINE_IF_ASCII
#endif

#ifndef EOF
#define EOF -1
#endif

/* WEOF from wide.h */

/* try to map size_t to unsigned long long, unsigned long, or int */
#if defined(SIZE_MAX)
#if defined(ULLONG_MAX) && SIZE_MAX == ULLONG_MAX
#if SCANF_DISABLE_SUPPORT_LONG_LONG
#define SIZET_DISABLE 1
#pragma message("size_t == long long (disabled), so it will be disabled too")
#else
#define SIZET_ALIAS ll
#endif
#elif defined(ULONG_MAX) && SIZE_MAX == ULONG_MAX
#define SIZET_ALIAS l
#elif defined(UINT_MAX) && SIZE_MAX == UINT_MAX
#define SIZET_ALIAS
/* intentionally empty -> maps to int */
#endif
#endif

/* this is intentionally after the previous check */
#ifndef SIZE_MAX
#define SIZE_MAX (size_t)(-1)
#endif

/* try to map ptrdiff_t to long long, long, or int */
#if defined(PTRDIFF_MAX)
#if defined(LLONG_MAX) && PTRDIFF_MAX == LLONG_MAX && PTRDIFF_MIN == LLONG_MIN
#if SCANF_DISABLE_SUPPORT_LONG_LONG
#define PTRDIFFT_DISABLE 1
#pragma message("ptrdiff_t == long long (disabled), so it will be disabled too")
#else
#define PTRDIFFT_ALIAS ll
#endif
#elif defined(LONG_MAX) && PTRDIFF_MAX == LONG_MAX && PTRDIFF_MIN == LONG_MIN
#define PTRDIFFT_ALIAS l
#elif defined(INT_MAX) && PTRDIFF_MAX == INT_MAX && PTRDIFF_MIN == INT_MIN
#define PTRDIFFT_ALIAS
/* intentionally empty -> maps to int */
#endif
#endif

/* try to map intmax_t to unsigned long long or unsigned long */
#if defined(INTMAX_MAX) && defined(UINTMAX_MAX)
#if !SCANF_DISABLE_SUPPORT_LONG_LONG &&                                        \
    (defined(LLONG_MAX) && INTMAX_MAX == LLONG_MAX && INTMAX_MIN == LLONG_MIN) \
    && (defined(ULLONG_MAX) && UINTMAX_MAX == ULLONG_MAX)
#define INTMAXT_ALIAS ll
#elif (defined(LONG_MAX) && INTMAX_MAX == LONG_MAX && INTMAX_MIN == LONG_MIN)  \
    && (defined(ULONG_MAX) && UINTMAX_MAX == ULONG_MAX)
#define INTMAXT_ALIAS l
#endif
#endif

/* check if short == int */
#if defined(INT_MIN) && defined(SHRT_MIN) && INT_MIN == SHRT_MIN               \
 && defined(INT_MAX) && defined(SHRT_MAX) && INT_MAX == SHRT_MAX               \
 && defined(UINT_MAX) && defined(USHRT_MAX) && UINT_MAX == USHRT_MAX
#define SHORT_IS_INT 1
#endif

/* check if long == int */
#if defined(INT_MIN) && defined(LONG_MIN) && INT_MIN == LONG_MIN               \
 && defined(INT_MAX) && defined(LONG_MAX) && INT_MAX == LONG_MAX               \
 && defined(UINT_MAX) && defined(ULONG_MAX) && UINT_MAX == ULONG_MAX
#define LONG_IS_INT 1
#endif

/* check if long long == long */
#if defined(LONG_MIN) && defined(LLONG_MIN) && LONG_MIN == LLONG_MIN           \
 && defined(LONG_MAX) && defined(LLONG_MAX) && LONG_MAX == LLONG_MAX           \
 && defined(ULONG_MAX) && defined(ULLONG_MAX) && ULONG_MAX == ULLONG_MAX
#define LLONG_IS_LONG 1
#endif

#if !SCANF_DISABLE_SUPPORT_FLOAT
/* check if double == float */
#if defined(FLT_MANT_DIG) && defined(DBL_MANT_DIG)                             \
                  && FLT_MANT_DIG == DBL_MANT_DIG                              \
 && defined(FLT_MIN_EXP) && defined(DBL_MIN_EXP)                               \
                  && FLT_MIN_EXP == DBL_MIN_EXP                                \
 && defined(FLT_MAX_EXP) && defined(DBL_MAX_EXP)                               \
                  && FLT_MAX_EXP == DBL_MAX_EXP
#define DOUBLE_IS_FLOAT 1
#endif

/* check if long double == double */
#if defined(DBL_MANT_DIG) && defined(LDBL_MANT_DIG)                            \
                  && DBL_MANT_DIG == LDBL_MANT_DIG                             \
 && defined(DBL_MIN_EXP) && defined(LDBL_MIN_EXP)                              \
                  && DBL_MIN_EXP == LDBL_MIN_EXP                               \
 && defined(DBL_MAX_EXP) && defined(LDBL_MAX_EXP)                              \
                  && DBL_MAX_EXP == LDBL_MAX_EXP
#define LDOUBLE_IS_DOUBLE 1
#endif
#endif

#ifdef UINTPTR_MAX
#define INT_TO_PTR(x) ((void*)(uintptr_t)(uintmax_t)(x))
#else
#define INT_TO_PTR(x) ((void*)(uintmax_t)(x))
#endif

#if SCANF_CLAMP
/* try to figure out values for PTRDIFF_MAX and PTRDIFF_MIN */
#ifndef PTRDIFF_MAX
#define PTRDIFF_MAX ptrdiff_max_
#define PTRDIFF_MAX_COMPUTE 1
#endif
#ifndef PTRDIFF_MIN
#define PTRDIFF_MIN ptrdiff_min_
#define PTRDIFF_MIN_COMPUTE 1
#endif
#endif

/* =============================== *
 *        digit  conversion        *
 * =============================== */

static INLINE_IF_ASCII int F_(ctodn_)(CINT c) {
#if SCANF_ASCII
    return c - C_('0');
#else
    switch (c) {
    case C_('0'): return 0;
    case C_('1'): return 1;
    case C_('2'): return 2;
    case C_('3'): return 3;
    case C_('4'): return 4;
    case C_('5'): return 5;
    case C_('6'): return 6;
    case C_('7'): return 7;
    case C_('8'): return 8;
    case C_('9'): return 9;
    default:      return -1;
    }
#endif
}

static INLINE_IF_ASCII int F_(ctoon_)(CINT c) {
#if SCANF_ASCII
    return c - C_('0');
#else
    switch (c) {
    case C_('0'): return 0;
    case C_('1'): return 1;
    case C_('2'): return 2;
    case C_('3'): return 3;
    case C_('4'): return 4;
    case C_('5'): return 5;
    case C_('6'): return 6;
    case C_('7'): return 7;
    default:      return -1;
    }
#endif
}

static int F_(ctoxn_)(CINT c) {
#if SCANF_ASCII
    if (c >= C_('a'))
        return c - C_('a') + 10;
    else if (c >= C_('A'))
        return c - C_('A') + 10;
    return c - C_('0');
#else
    switch (c) {
    case C_('0'): return 0;
    case C_('1'): return 1;
    case C_('2'): return 2;
    case C_('3'): return 3;
    case C_('4'): return 4;
    case C_('5'): return 5;
    case C_('6'): return 6;
    case C_('7'): return 7;
    case C_('8'): return 8;
    case C_('9'): return 9;
    case C_('A'): case C_('a'): return 10;
    case C_('B'): case C_('b'): return 11;
    case C_('C'): case C_('c'): return 12;
    case C_('D'): case C_('d'): return 13;
    case C_('E'): case C_('e'): return 14;
    case C_('F'): case C_('f'): return 15;
    default:      return -1;
    }
#endif
}

#if SCANF_BINARY
static INLINE_IF_ASCII int F_(ctobn_)(CINT c) {
#if SCANF_ASCII
    return c - C_('0');
#else
    switch (c) {
    case C_('0'): return 0;
    case C_('1'): return 1;
    default:      return -1;
    }
#endif
}
#endif

static int F_(ctorn_)(CINT c, int b) {
    switch (b) {
    case 8:
        return F_(ctoon_)(c);
    case 16:
        return F_(ctoxn_)(c);
#if SCANF_BINARY
    case 2:
        return F_(ctobn_)(c);
#endif
    default: /* 10 */
        return F_(ctodn_)(c);
    }
}

/* =============================== *
 *         character checks        *
 * =============================== */

#if SCANF_INTERNAL_CTYPE
static int F_(isspace)(CINT c) {
    switch (c) {
    case C_(' '):
    case C_('\t'):
    case C_('\n'):
    case C_('\v'):
    case C_('\f'):
    case C_('\r'):
        return 1;
    }
    return 0;
}

static INLINE int F_(isdigit)(CINT c) {
#if SCANF_ASCII
    return C_('0') <= c && c <= C_('9');
#else
    return F_(ctodn_)(c) >= 0;
#endif
}

#if SCANF_INFINITE
#if SCANF_ASCII
static int F_(isalpha)(CINT c) {
    return (C_('A') <= c && c <= C_('Z')) || (C_('a') <= c && c <= C_('z'));
}

static INLINE CINT F_(tolower)(CINT c) {
    return F_(isalpha)(c) ? c | 0x20 : c;
}
#else
static int F_(isalpha)(CINT c) {
    switch (c) {
    case _C('A'): case _C('N'): case _C('a'): case _C('n'):
    case _C('B'): case _C('O'): case _C('b'): case _C('o'):
    case _C('C'): case _C('P'): case _C('c'): case _C('p'):
    case _C('D'): case _C('Q'): case _C('d'): case _C('q'):
    case _C('E'): case _C('R'): case _C('e'): case _C('r'):
    case _C('F'): case _C('S'): case _C('f'): case _C('s'):
    case _C('G'): case _C('T'): case _C('g'): case _C('t'):
    case _C('H'): case _C('U'): case _C('h'): case _C('u'):
    case _C('I'): case _C('V'): case _C('i'): case _C('v'):
    case _C('J'): case _C('W'): case _C('j'): case _C('w'):
    case _C('K'): case _C('X'): case _C('k'): case _C('x'):
    case _C('L'): case _C('Y'): case _C('l'): case _C('y'):
    case _C('M'): case _C('Z'): case _C('m'): case _C('z'):
        return 1;
    }
    return 0;
}

static CINT F_(tolower)(CINT c) {
    switch (c) {
    case _C('A'):   return _C('a');     case _C('N'):   return _C('n');
    case _C('B'):   return _C('b');     case _C('O'):   return _C('o');
    case _C('C'):   return _C('c');     case _C('P'):   return _C('p');
    case _C('D'):   return _C('d');     case _C('Q'):   return _C('q');
    case _C('E'):   return _C('e');     case _C('R'):   return _C('r');
    case _C('F'):   return _C('f');     case _C('S'):   return _C('s');
    case _C('G'):   return _C('g');     case _C('T'):   return _C('t');
    case _C('H'):   return _C('h');     case _C('U'):   return _C('u');
    case _C('I'):   return _C('i');     case _C('V'):   return _C('v');
    case _C('J'):   return _C('j');     case _C('W'):   return _C('w');
    case _C('K'):   return _C('k');     case _C('X'):   return _C('x');
    case _C('L'):   return _C('l');     case _C('Y'):   return _C('y');
    case _C('M'):   return _C('m');     case _C('Z'):   return _C('z');
    }
    return c;
}
#endif /* SCANF_ASCII */

static INLINE int F_(isalnum)(CINT c) {
    return F_(isdigit)(c) || F_(isalpha)(c);
}
#endif /* SCANF_INFINITE */

#endif /* SCANF_INTERNAL_CTYPE */

/* case-insensitive character comparison. compares character c to
   upper (uppercase) and lower (lowercase). only defined if upper and lower
   are letters and it applies that tolower(upper) == lower */
#ifndef SCANF_REPEAT
#if SCANF_ASCII
#define ICASEEQ(c, upper, lower) ((((CINT)(c)) & ~0x20U) == (CINT)(C_(upper)))
#else
#define ICASEEQ(c, upper, lower) (((c) == C_(upper)) || ((c) == C_(lower)))
#endif
#endif

static INLINE int F_(isdigo_)(CINT c) {
#if SCANF_ASCII
    return C_('0') <= c && c <= C_('7');
#else
    return F_(ctoon_)(c) >= 0;
#endif
}

static INLINE int F_(isdigx_)(CINT c) {
#if SCANF_ASCII
    return F_(isdigit)(c) || (C_('A') <= c && c <= C_('F'))
                          || (C_('a') <= c && c <= C_('f'));
#else
    return F_(ctoxn_)(c) >= 0;
#endif
}

#if SCANF_BINARY
static INLINE int F_(isdigb_)(CINT c) {
    return c == C_('0') || c == C_('1');
}
#endif

static INLINE int F_(isdigr_)(CINT c, int b) {
    switch (b) {
    case 8:
        return F_(isdigo_)(c);
    case 16:
        return F_(isdigx_)(c);
#if SCANF_BINARY
    case 2:
        return F_(isdigb_)(c);
#endif
    default: /* 10 */
        return F_(isdigit)(c);
    }
}

/* =============================== *
 *          integer  math          *
 * =============================== */

#if SCANF_CLAMP && !defined(SCANF_REPEAT)

static INLINE intmax_t clamps_(intmax_t m0, intmax_t v, intmax_t m1) {
    return v < m0 ? m0 : v > m1 ? m1 : v;
}

static INLINE uintmax_t clampu_(uintmax_t m0, uintmax_t v, uintmax_t m1) {
    return v < m0 ? m0 : v > m1 ? m1 : v;
}

#endif /* SCANF_CLAMP && !defined(SCANF_REPEAT) */

/* =============================== *
 *       floating point math       *
 * =============================== */

#ifndef SCANF_REPEAT

#if !SCANF_DISABLE_SUPPORT_FLOAT

#if !SCANF_NOPOW
static INLINE floatmax_t powi_(floatmax_t x, intmax_t y) {
    return pow(x, y);
}
#elif SCANF_LOGN_POW
static floatmax_t powi_(floatmax_t x, intmax_t y) {
    floatmax_t r = (floatmax_t)1;
    for (; y > 0; y >>= 1) {
        if (y & 1) r *= x;
        x *= x;
    }
    return r;
}
#else
static floatmax_t powi_(floatmax_t x, intmax_t y) {
    floatmax_t r = (floatmax_t)1;
    for (; y > 0; --y)
        r *= x;
    return r;
}
#endif

#endif /* !SCANF_DISABLE_SUPPORT_FLOAT */

#endif /* SCANF_REPEAT */

/* =============================== *
 *        scanset functions        *
 * =============================== */

#if !SCANF_DISABLE_SUPPORT_SCANSET && !SCANF_CAN_FAST_SCANSET

static BOOL F_(inscan_)(const UCHAR *begin, const UCHAR *end, UCHAR c) {
    BOOL hyphen = 0;
    UCHAR prev = 0, f;
    const UCHAR *p = begin;
    while (p < end) {
        f = *p++;
        if (hyphen) {
            if (prev <= c && c <= f)
                return 1;
            hyphen = 0;
            prev = f;
        } else if (f == C_('-') && prev) {
            hyphen = 1;
        } else if (f == c)
            return 1;
        else
            prev = f;
    }
    return hyphen && c == C_('-');
}

#endif

/* =============================== *
 *      conversion  functions      *
 * =============================== */

#ifndef SCANF_REPEAT
        /* still characters to read? (not EOF and width not exceeded) */
#define KEEP_READING() (nowread < maxlen && !GOT_EOF())
        /* read next char and increment counter */
#define NEXT_CHAR(counter) (next = getch(p), ++counter)
#endif

#undef IS_EOF
#undef GCEOF
/* EOF check */
#if SCANF_WIDE
#define IS_EOF(c) ((c) == WEOF)
#define GCEOF WEOF
#else
#define IS_EOF(c) ((c) < 0)
#define GCEOF EOF
#endif

#undef GOT_EOF
#define GOT_EOF() (IS_EOF(next))

/* convert stream to integer
    getch: stream read function
    p: pointer to pass to above
    nextc: pointer to next character in buffer
    readin: pointer to number of characters read
    maxlen: value that *readin should be at most
    base: 10 for decimal, 16 for hexadecimal, etc.
    unsign: whether the result should be unsigned
    negative: whether there was a - sign
    zero: if no digits, allow zero (i.e. read zero before iaton_)
    dest: intmax_t* or uintmax_t*, where result is stored

    return value: 1 if conversion OK, 0 if not
                  (if 0, dest guaranteed to not be modified)
*/
static INLINE BOOL F_(iaton_)(CINT (*getch)(void *p), void *p, CINT *nextc,
                size_t *readin, size_t maxlen, int base, BOOL unsign,
                BOOL negative, BOOL zero, void *dest) {
    CINT next = *nextc;
    size_t nowread = *readin;
    uintmax_t r = 0, pr = 0;
    /* read digits? overflow? */
    BOOL digit = 0, ovf = 0;

#if !SCANF_MINIMIZE
    /* skip initial zeros */
    while (KEEP_READING() && next == C_('0')) {
        digit = 1;
        NEXT_CHAR(nowread);
    }
#endif

    /* read digits and convert to integer */
    while (KEEP_READING() && F_(isdigr_)(next, base)) {
        if (!ovf) {
            digit = 1;
            r = r * base + F_(ctorn_)(next, base);
            if (pr > r)
                ovf = 1;
            else
                pr = r; 
        }
        NEXT_CHAR(nowread);
    }

    /* if no digits read? */
    if (digit) {
        /* overflow detection, negation, etc. */
        if (unsign) {
            if (ovf)
                r = (intmax_t)UINTMAX_MAX;
            else if (negative)
#if SCANF_CLAMP
                r = 0;
#else
                r = (uintmax_t)-r;
#endif
            *(uintmax_t *)dest = r;
        } else {
            intmax_t sr;
            if (ovf || (negative ? (intmax_t)r < 0
                                 : r > (uintmax_t)INTMAX_MAX))
                sr = negative ? INTMAX_MIN : INTMAX_MAX;
            else {
                sr = negative ? -(intmax_t)r : (intmax_t)r;
            }
            *(intmax_t *)dest = sr;
        }
    } else if (zero) {
        if (unsign)
            *(uintmax_t *)dest = 0;
        else
            *(intmax_t *)dest = 0;
        digit = 1;
    }

    *nextc = next;
    *readin = nowread;
    return digit;
}

#if !SCANF_DISABLE_SUPPORT_FLOAT
/* convert stream to floating point
    getch: stream read function
    p: pointer to pass to above
    nextc: pointer to next character in buffer
    readin: pointer to number of characters read
    maxlen: value that *readin should be at most
    hex: whether in hex mode
    negative: whether there was a - sign
    zero: if no digits, allow zero (i.e. read zero before iatof_)
    dest: floatmax_t*, where result is stored

    return value: 1 if conversion OK, 0 if not
                  (if 0, dest guaranteed to not be modified)
*/
static INLINE BOOL F_(iatof_)(CINT (*getch)(void *p), void *p, CINT *nextc,
                       size_t *readin, size_t maxlen, BOOL hex, BOOL negative,
                       BOOL zero, floatmax_t *dest) {
    CINT next = *nextc;
    size_t nowread = *readin;
    floatmax_t r = 0, pr = 0;
    /* saw dot? saw digit? was there an overflow? */
    BOOL dot = 0, digit = 0, ovf = 0;
    /* exponent, offset (with decimals, etc.) */
    intmax_t exp = 0, off = 0;
    /* how much to subtract from off with every digit */
    int sub = 0;
    /* base */
    int base = hex ? 16 : 10;
    /* exponent character */
    CHAR expuc = hex ? 'P' : 'E', explc = hex ? 'p' : 'e';

#if !SCANF_MINIMIZE
    while (KEEP_READING() && next == C_('0')) {
        digit = 1;
        NEXT_CHAR(nowread);
    }
#endif

    /* read digits and convert */
    while (KEEP_READING()) {
        if (F_(isdigr_)(next, base)) {
            if (!ovf) {
                digit = 1;
                r = r * base + F_(ctorn_)(next, base);
                if (r > 0 && pr == r) {
                    ovf = 1;
                } else {
                    pr = r;
                    off += sub;
                }
            }
        } else if (next == C_('.')) {
            if (dot)
                break;
            dot = 1, sub = hex ? 4 : 1;
        } else
            break;
        NEXT_CHAR(nowread);
    }

    if (zero && !digit)
        digit = 1;
        /* r == 0 should already apply */

    if (digit) {
        /* exponent? */
        if (KEEP_READING() && (next == explc || next == expuc)) {
            BOOL eneg = 0;
            NEXT_CHAR(nowread);
            if (KEEP_READING()) {
                switch (next) {
                case C_('-'):
                    eneg = 1;
                    /* fall-through */
                case C_('+'):
                    NEXT_CHAR(nowread);
                }
            }

            if (!F_(iaton_)(getch, p, &next, &nowread, maxlen, 10,
                            0, eneg, 0, &exp))
                digit = 0;
        }
    }

    if (digit) {
        if (dot) {
            intmax_t oexp = exp;
            exp -= off;
            if (exp > oexp) exp = INTMAX_MIN; /* underflow protection */
        }

        if (r != 0) {
            if (exp > 0) {
#ifdef INFINITY
#if FLT_RADIX == 2
                if (exp > (hex ? LDBL_MAX_EXP : LDBL_MAX_10_EXP))
#else
                /* FLT_RADIX > 2 means LDBL_MAX_EXP is defined as e
                    in b^e for some b > 2; we'd need to compute
                    LDBL_MAX_EXP * log(b) / log(2)
                    at compile time, which is hardly an option.
                    so instead we'll do the next best thing */
                if (exp > (hex ? (LDBL_MAX_EXP * FLT_RADIX + 1)
                                : LDBL_MAX_10_EXP))
#endif
                    r = INFINITY;
                else
#endif
                    r *= powi_(hex ? 2 : 10, exp);
            } else if (exp < 0) {
#if FLT_RADIX == 2
                if (exp < (hex ? LDBL_MIN_EXP : LDBL_MIN_10_EXP))
#else
                if (exp < (hex ? (LDBL_MIN_EXP * FLT_RADIX - 1)
                                : LDBL_MIN_10_EXP))
#endif
                    r = 0;
                else
                    r /= powi_(hex ? 2 : 10, -exp);
            }
        }

        if (negative) r = -r;
        *dest = r;
    }

    *nextc = next;
    *readin = nowread;
    return digit;
}
#endif /* !SCANF_DISABLE_SUPPORT_FLOAT */

#ifndef SCANF_REPEAT
enum iscans_type { A_CHAR, A_STRING, A_SCANSET };
#endif /* SCANF_REPEAT */

#if !SCANF_DISABLE_SUPPORT_SCANSET

struct F_(scanset_) {
#if SCANF_CAN_FAST_SCANSET
    const BOOL *mask;
#else
    const UCHAR *set_begin;
    const UCHAR *set_end;
#endif
    BOOL invert;
};

#define STRUCT_SCANSET struct F_(scanset_)

static INLINE BOOL F_(insset_)(const struct F_(scanset_) *set, UCHAR c) {
#if SCANF_CAN_FAST_SCANSET
    return set->mask[c] != set->invert;
#else
    return F_(inscan_)(set->set_begin, set->set_end, c) != set->invert;
#endif
}

#else /* !SCANF_DISABLE_SUPPORT_SCANSET */

#define STRUCT_SCANSET void

#endif /* !SCANF_DISABLE_SUPPORT_SCANSET */

/* read char(s)/string from stream without char conversion
    getch: stream read function
    p: pointer to pass to above
    nextc: pointer to next character in buffer
    readin: pointer to number of characters read
    maxlen: value that *readin should be at most
    ctype: one of the values of iscans_type
    set: a struct scanset_, only used with A_SCANSET
    nostore: whether nostore was specified
    outp: output CHAR pointer (not dereferenced if nostore=1)

    return value: 1 if conversion OK, 0 if not
*/
static INLINE BOOL F_(iscans_)(CINT (*getch)(void *p), void *p, CINT *nextc,
                    size_t *readin, size_t maxlen, enum iscans_type ctype,
#if !SCANF_DISABLE_SUPPORT_SCANSET
                    const STRUCT_SCANSET *set,
#endif
                    BOOL nostore, CHAR *outp) {
    CINT next = *nextc;
    size_t nowread = *readin;

    while (KEEP_READING()) {
        if (ctype == A_STRING && F_(isspace)(next))
            break;
#if !SCANF_DISABLE_SUPPORT_SCANSET
        if (ctype == A_SCANSET && !F_(insset_)(set, (UCHAR)next))
            break;
#endif /* !SCANF_DISABLE_SUPPORT_SCANSET */
        if (!nostore) *outp++ = (CHAR)(UCHAR)next;
        NEXT_CHAR(nowread);
    }

    *nextc = next;
    *readin = nowread;
    switch (ctype) {
    case A_CHAR:
        if (nowread < maxlen)
            return 0;
        break;
    case A_STRING:
#if !SCANF_DISABLE_SUPPORT_SCANSET
    case A_SCANSET:
#endif /* !SCANF_DISABLE_SUPPORT_SCANSET */
        if (!nowread)
            return 0;
        if (!nostore)
            *outp = C_('\0');
#if SCANF_DISABLE_SUPPORT_SCANSET
    default: ; /* inhibit compiler warning for missing case for enum */
#endif
    }
    return 1;
}

#if SCANF_WIDE_CONVERT
#undef CVTCHAR
#if SCANF_WIDE
#define CVTCHAR char
#else
#define CVTCHAR WCHAR
#endif

/* read char(s)/string from stream with conversion
    getch: stream read function
    p: pointer to pass to above
    nextc: pointer to next character in buffer
    readin: pointer to number of characters read
    maxlen: value that *readin should be at most
    ctype: one of the values of iscans_type
    set: a struct scanset_, only used with A_SCANSET
    nostore: whether nostore was specified
    outp: output pointer of the other char type (not dereferenced if nostore=1)

    return value: 1 if conversion OK, 0 if not
*/
static INLINE BOOL F_(iscvts_)(CINT (*getch)(void *p), void *p, CINT *nextc,
                    size_t *readin, size_t maxlen, enum iscans_type ctype,
#if !SCANF_DISABLE_SUPPORT_SCANSET
                    const STRUCT_SCANSET *set,
#endif
                    BOOL nostore, CVTCHAR *outp) {
    CINT next = *nextc;
    size_t nowread = *readin, mbr;
    scanf_mbstate_t mbstate;
#if SCANF_WIDE
    char tmp[MB_LEN_MAX];
    if (nostore)
        outp = tmp;
#else
    char nc;
    WCHAR tmp;
    if (nostore)
        outp = &tmp;
#endif

    mbsetup_(&mbstate);
    while (KEEP_READING()) {
        if (ctype == A_STRING && F_(isspace)(next))
            break;
#if !SCANF_DISABLE_SUPPORT_SCANSET
        if (ctype == A_SCANSET && !F_(insset_)(set, (UCHAR)next))
            break;
#endif /* !SCANF_DISABLE_SUPPORT_SCANSET */
#if SCANF_WIDE
        /* wide => narrow */
        mbr = wcrtomb_(outp, next, &mbstate);
        if (mbr == (size_t)(-1)) {
            *nextc = next;
            *readin = nowread;
            return 0;
        }
        else if (mbr > 0 && !nostore)
            outp += mbr;
#else
        /* narrow => wide */
        nc = (char)next;
        mbr = mbrtowc_(outp, &nc, 1, &mbstate);
        if (mbr == (size_t)(-1)) {
            *nextc = next;
            *readin = nowread;
            return 0;
        }
        else if (mbr != (size_t)(-2) && !nostore)
            ++outp;
#endif
        NEXT_CHAR(nowread);
    }

    *nextc = next;
    *readin = nowread;
    switch (ctype) {
    case A_CHAR:
        if (nowread < maxlen)
            return 0;
        break;
    case A_STRING:
#if !SCANF_DISABLE_SUPPORT_SCANSET
    case A_SCANSET:
#endif
        if (!nowread)
            return 0;
        if (!nostore)
            *outp = 0;
#if SCANF_DISABLE_SUPPORT_SCANSET
    default: ; /* inhibit compiler warning for missing case for enum */
#endif
    }
    return 1;
}
#endif

/* =============================== *
 *        extension support        *
 * =============================== */

/* there should almost never be a reason to change this */
#ifndef SCANF_EXT_CHAR
#define SCANF_EXT_CHAR '!'
#endif

#if SCANF_EXTENSIONS
#if SCANF_WIDE
int scnwext_(WINT (*getwch)(void *data), void *data, const WCHAR **format,
             WINT *buffer, int length, int nostore, void *destination);
#else
int scnext_(int (*getch)(void *data), void *data, const char **format,
            int *buffer, int length, int nostore, void *destination);
#endif

struct F_(scanf_ext_tmp) {
    CINT (*getch)(void *data);
    void *data;
    size_t len;
};

CINT F_(scanf_ext_getch_)(void *data) {
    struct F_(scanf_ext_tmp) *st = (struct F_(scanf_ext_tmp) *)data;
    if (!st->len)
        return GCEOF;
    else {
        --st->len;
        return st->getch(st->data);
    }
}
#endif /* SCANF_EXTENSIONS */

/* =============================== *
 *       main scanf function       *
 * =============================== */

#ifndef SCANF_REPEAT
        /* signal input failure and exit loop */
#define INPUT_FAILURE() do { goto read_failure; } while (0)
        /* signal match OK */
#define MATCH_SUCCESS() (noconv = 0)
        /* signal match failure and exit loop */
#define MATCH_FAILURE() do { if (!GOT_EOF()) MATCH_SUCCESS(); \
                             INPUT_FAILURE(); } while (0)
            /* store value to dst with cast */
#define STORE_DST(value, T) (*(T *)(dst) = (T)(value))
            /* store value to dst with cast and possible signed clamp */
#if SCANF_CLAMP
#define STORE_DSTI(v, T, minv, maxv) STORE_DST(clamps_(minv, v, maxv), T)
#else
#define STORE_DSTI(v, T, minv, maxv) STORE_DST(v, T)
#endif
            /* store value to dst with cast and possible unsigned clamp */
#if SCANF_CLAMP
#define STORE_DSTU(v, T, minv, maxv) STORE_DST(clampu_(minv, v, maxv), T)
#else
#define STORE_DSTU(v, T, minv, maxv) STORE_DST(v, T)
#endif
#endif

#ifndef SCANF_REPEAT
/* enum for possible data types */
enum dlength { LN_, LN_hh, LN_h, LN_l, LN_ll, LN_L, LN_j, LN_z, LN_t };

#define vLNa_(x) LN_##x
#define vLN_(x) vLNa_(x)

#if PTRDIFF_MAX_COMPUTE
static const int signed_padding_div_ = (int)(                                  \
        (sizeof(ptrdiff_t) > sizeof(uintmax_t) ? 1 :                           \
            INT_MAX < UINTMAX_MAX ?                                            \
                ((uintmax_t)1 << (CHAR_BIT * sizeof(int)))                     \
                    / ((uintmax_t)INT_MAX + 1) :                               \
            SHRT_MAX < UINTMAX_MAX ?                                           \
                ((uintmax_t)1 << (CHAR_BIT * sizeof(short)))                   \
                    / ((uintmax_t)SHRT_MAX + 1) : 2));
static const ptrdiff_t ptrdiff_max_ = (ptrdiff_t)                              \
        (sizeof(ptrdiff_t) > sizeof(intmax_t) ? 0 :                            \
            sizeof(ptrdiff_t) == sizeof(intmax_t)                              \
                ? INTMAX_MAX                                                   \
                : (((uintmax_t)1 << (CHAR_BIT * sizeof(ptrdiff_t)))            \
                    / signed_padding_div_ + UINTMAX_MAX));
#endif
#if PTRDIFF_MIN_COMPUTE
static const ptrdiff_t ptrdiff_min_ = (ptrdiff_t)                              \
        (sizeof(ptrdiff_t) > sizeof(intmax_t) ? 0 :                            \
            sizeof(ptrdiff_t) == sizeof(intmax_t)                              \
                ? INTMAX_MIN                                                   \
                : (~(intmax_t)-1 > ~(intmax_t)-2)                              \
                    ? -PTRDIFF_MAX : -PTRDIFF_MAX + ~(intmax_t)0);
#endif
#endif /* SCANF_REPEAT */

static int F_(iscanf_)(CINT (*getch)(void *p), void (*ungetch)(CINT c, void *p),
                       void *p, const CHAR *ff, va_list va) {
    /* fields = number of fields successfully read; this is the return value */
    int fields = 0;
    /* next = the "next" character to be processed */
    CINT next;
    /* total characters read, returned by %n */
    size_t read_chars = 0;
    /* there were attempts to convert? there were no conversions? */
    BOOL tryconv = 0, noconv = 1;
    const UCHAR *f = (const UCHAR *)ff;
    UCHAR c;

    /* empty format string always returns 0 */
    if (!*f) return 0;

    /* read and cache first character */
    next = getch(p);
    /* ++read_chars; intentionally left out, otherwise %n is off by 1 */
    while ((c = *f++)) {
        if (F_(isspace)(c)) {
            /* skip 0-N whitespace */
            while (!GOT_EOF() && F_(isspace)(next))
                NEXT_CHAR(read_chars);
        } else if (c != C_('%')) {
            if (GOT_EOF()) break;
            /* must match literal character */
            if (next != c) {
                INPUT_FAILURE();
                break;
            }
            NEXT_CHAR(read_chars);
        } else { /* % */
            /* nostore is %*, prevents a value from being stored */
            BOOL nostore;
            /* nowread = characters read for this format specifier
               maxlen = maximum number of characters to be read "field width" */
            size_t nowread = 0, maxlen = 0;
            /* length specifier (l, ll, h, hh...) */
            enum dlength dlen = LN_;
            /* where the value will be stored */
            void *dst;

            /* nostore */
            if (*f == C_('*')) {
                nostore = 1;
                dst = NULL;
                ++f;
            } else {
                nostore = 0;
                dst = va_arg(va, void *);
                /* A pointer to any incomplete or object type may be converted
                   to a pointer to void and back again; the result shall compare
                   equal to the original pointer. */
            }

            /* width specifier => maxlen */
            if (F_(isdigit)(*f)) {
                size_t pr = 0;
#if !SCANF_MINIMIZE
                /* skip initial zeros */
                while (*f == C_('0'))
                    ++f;
#endif
                while (F_(isdigit)(*f)) {
                    maxlen = maxlen * 10 + F_(ctodn_)(*f);
                    if (maxlen < pr) {
                        maxlen = SIZE_MAX;
                        while (F_(isdigit)(*f))
                            ++f;
                        break;
                    } else
                        pr = maxlen;
                    ++f;
                }
            }

#if SCANF_EXTENSIONS
            if (*f == C_(SCANF_EXT_CHAR)) {
                const CHAR *sf = (const CHAR *)(f + 1);
                BOOL hadlen = maxlen != 0;
                struct F_(scanf_ext_tmp) tmp;
                int ok;

                if (!hadlen)
                    maxlen = SIZE_MAX;
                tmp.getch = getch;
                tmp.data = p;
                tmp.len = maxlen;
#if SCANF_WIDE
                ok = scnwext_(&F_(scanf_ext_getch_), &tmp, &sf, &next,
                              hadlen, nostore, dst);
#else
                ok = scnext_(&F_(scanf_ext_getch_), &tmp, &sf, &next,
                             hadlen, nostore, dst);
#endif
                f = (const UCHAR *)sf;
                if (ok < 0)
                    INPUT_FAILURE();
                else if (ok > 0)
                    MATCH_FAILURE();
                else if (!nostore)
                    ++fields;
                read_chars += maxlen - tmp.len;
                continue;
            }
#endif

            /* length specifier */
            switch (*f++) {
            case C_('h'):
                if (*f == C_('h'))
                    dlen = LN_hh, ++f;
                else
                    dlen = LN_h;
                break;
            case C_('l'):
#if !SCANF_DISABLE_SUPPORT_LONG_LONG
                if (*f == C_('l'))
                    dlen = LN_ll, ++f;
                else
#endif
                    dlen = LN_l;
                break;
            case C_('j'):
#ifdef INTMAXT_ALIAS
                dlen = vLN_(INTMAXT_ALIAS);
#else
                dlen = LN_j;
#endif
                break;
            case C_('t'):
#if PTRDIFFT_DISABLE
                MATCH_FAILURE();
#else
#ifdef PTRDIFFT_ALIAS
                dlen = vLN_(PTRDIFFT_ALIAS);
#else
                if (sizeof(ptrdiff_t) > sizeof(intmax_t))
                    MATCH_FAILURE();
                dlen = LN_t;
#endif
                break;
#endif /* PTRDIFFT_DISABLE */
            case C_('z'):
#if SIZET_DISABLE
                MATCH_FAILURE();
#else
#ifdef SIZET_ALIAS
                dlen = vLN_(SIZET_ALIAS);
#else
                if (sizeof(size_t) > sizeof(uintmax_t))
                    MATCH_FAILURE();
                dlen = LN_z;
#endif
                break;
#endif /* SIZET_DISABLE */
            case C_('L'):
                dlen = LN_L;
                break;
            default:
                --f;
            }

            c = *f;
            switch (c) {
            default:
                /* skip whitespace. include in %n, but not elsewhere */
                while (!GOT_EOF() && F_(isspace)(next))
                    NEXT_CHAR(read_chars);
                /* fall-through */
            /* do not skip whitespace for... */
            case C_('['):
            case C_('c'):
                tryconv = 1;
                if (GOT_EOF()) INPUT_FAILURE();
                /* fall-through */
            /* do not even check EOF for... */
            case C_('n'):
                break;
            }

            /* format */
            switch (c) {
            case C_('%'):
                /* literal % */
                if (next != C_('%')) MATCH_FAILURE();
                NEXT_CHAR(nowread);
                break;
            { /* =========== READ INT =========== */
                /* variables for reading ints */
                /* decimal, hexadecimal, octal, binary */
                int base;
                /* is negative? unsigned? %p? */
                BOOL negative, unsign, isptr;
                /* result: i for signed integers, u for unsigned integers,
                           p for pointers */
                union {
                    intmax_t i;
                    uintmax_t u;
                    void *p;
                } r;

            case C_('n'): /* number of characters read */
                    if (nostore)
                        break;
                    r.i = (intmax_t)read_chars;
                    unsign = 0, isptr = 0;
                    goto storenum;
            case C_('p'): /* pointer */
                    isptr = 1;
#if !SCANF_DISABLE_SUPPORT_NIL
                    if (next == C_('(')) { /* handle (nil) */
                        int k;
                        const CHAR *rest = S_("nil)");
                        if (!maxlen) maxlen = SIZE_MAX;
                        NEXT_CHAR(nowread);
                        for (k = 0; k < 4; ++k) {
                            if (!KEEP_READING() || next != rest[k])
                                MATCH_FAILURE();
                            NEXT_CHAR(nowread);
                        }
                        MATCH_SUCCESS();
                        if (!nostore) {
                            ++fields;
                            r.p = NULL;
                            goto storeptr;
                        }
                        break;
                    }
#endif /* !SCANF_DISABLE_SUPPORT_NIL */
                    base = 10, unsign = 1, negative = 0;
                    goto readptr;
            case C_('o'): /* unsigned octal integer */
                    base = 8, unsign = 1;
                    goto readnum;
            case C_('x'): /* unsigned hexadecimal integer */
            case C_('X'):
                    base = 16, unsign = 1;
                    goto readnum;
#if SCANF_BINARY
            case C_('b'): /* non-standard: unsigned binary integer */
                    base = 2, unsign = 1;
                    goto readnum;
#endif
            case C_('u'): /* unsigned decimal integer */
            case C_('d'): /* signed decimal integer */
            case C_('i'): /* signed decimal/hex/binary integer */
                    base = 10, unsign = c == C_('u');
                    /* fall-through */
            readnum:
                    isptr = 0, negative = 0;

                    /* sign, read even for %u */
                    /* maxlen > 0, so this is always fine */
                    switch (next) {
                    case C_('-'):
                        negative = 1;
                        /* fall-through */
                    case C_('+'):
                        NEXT_CHAR(nowread);
                    }
                    /* fall-through */
            readptr:
                {
                    BOOL zero = 0;
                    if (!maxlen) maxlen = SIZE_MAX;

                    /* detect base from string for %i and %p, skip 0x for %x */
                    if (c == C_('i') || c == C_('p') || ICASEEQ(c, 'X', 'x')
#if SCANF_BINARY
                        || ICASEEQ(c, 'B', 'b')
#endif
                    ) {
                        if (KEEP_READING() && next == C_('0')) {
                            zero = 1;
                            NEXT_CHAR(nowread);
                            if (KEEP_READING() && ICASEEQ(next, 'X', 'x')) {
                                if (base == 10)
                                    base = 16;
                                else if (base != 16) {
                                    /* read 0b for %x, etc. */
                                    unsign ? (r.u = 0) : (r.i = 0);
                                    goto readnumok;
                                }
                                NEXT_CHAR(nowread);
                                /* zero = 1. "0x" should be read as 0, because
                                   0 is a valid strtol input, but we cannot
                                   unread x at this point, so it'll stay read */
#if SCANF_BINARY
                            } else if (KEEP_READING() &&
                                                  ICASEEQ(next, 'B', 'b')) {
                                if (base == 10)
                                    base = 2;
                                else if (base != 2) {
                                    /* read 0x for %b, etc. */
                                    unsign ? (r.u = 0) : (r.i = 0);
                                    goto readnumok;
                                }
                                NEXT_CHAR(nowread);
#endif
                            } else if (c == C_('i'))
                                base = 8;
                        }
                    }

                    /* convert */
                    if (!F_(iaton_)(getch, p, &next, &nowread, maxlen, base,
                                unsign, negative, zero, unsign ? (void *)&r.u
                                                               : (void *)&r.i))
                        MATCH_FAILURE();

            readnumok:
                    MATCH_SUCCESS();

                    if (nostore)
                        break;
                    ++fields;
                }
            storenum:
                /* store number, either as ptr, unsigned or signed */
                if (isptr) {
                    r.p = unsign ? INT_TO_PTR(r.u) : INT_TO_PTR(r.i);
            storeptr:
                    STORE_DST(r.p, void *);
                } else {
                    switch (dlen) {
                    case LN_hh:
                        if (unsign) STORE_DSTU(r.u, unsigned char,
                                               0, UCHAR_MAX);
                        else        STORE_DSTI(r.i, signed char,
                                               SCHAR_MIN, SCHAR_MAX);
                        break;
#if !SHORT_IS_INT
                    case LN_h: /* if SHORT_IS_INT, match fails => default: */
                        if (unsign) STORE_DSTU(r.u, unsigned short,
                                               0, USHRT_MAX);
                        else        STORE_DSTI(r.i, short,
                                               SHRT_MIN, SHRT_MAX);
                        break;
#endif
#ifndef INTMAXT_ALIAS
                    case LN_j:
                        if (unsign) STORE_DSTU(r.u, uintmax_t,
                                               0, UINTMAX_MAX);
                        else        STORE_DSTI(r.i, intmax_t,
                                               INTMAX_MIN, INTMAX_MAX);
                        break;
#endif /* INTMAXT_ALIAS */
#ifndef SIZET_ALIAS
                    case LN_z:
                        if (!unsign) {
#if SCANF_CLAMP
                            if (r.i < 0)
                                r.u = 0;
                            else
#endif
                                r.u = (uintmax_t)r.i;
                        }
                        STORE_DSTU(r.u, size_t, 0, SIZE_MAX);
                        break;
#endif /* SIZET_ALIAS */
#ifndef PTRDIFFT_ALIAS
                    case LN_t:
                        if (unsign) {
#if SCANF_CLAMP
                            if (r.u > (uintmax_t)INTMAX_MAX)
                                r.i = INTMAX_MAX;
                            else
#endif
                                r.i = (intmax_t)r.u;
                        }
#if PTRDIFF_MAX_COMPUTE
                        /* min/max are wrong, don't even try to clamp */
                        if (PTRDIFF_MIN >= PTRDIFF_MAX)
                            STORE_DST(r.i, ptrdiff_t);
                        else
#endif
                        STORE_DSTI(r.i, ptrdiff_t, PTRDIFF_MIN, PTRDIFF_MAX);
                        break;
#endif /* PTRDIFFT_ALIAS */
#if !SCANF_DISABLE_SUPPORT_LONG_LONG
                    case LN_ll:
#if !LLONG_IS_LONG
                        if (unsign) STORE_DSTU(r.u, unsigned long long,
                                                    0, ULLONG_MAX);
                        else        STORE_DSTI(r.i, long long,
                                                    LLONG_MIN, LLONG_MAX);
                        break;
#endif
#endif /* SCANF_DISABLE_SUPPORT_LONG_LONG */
                    case LN_l:
#if !LONG_IS_INT
                        if (unsign) STORE_DSTU(r.u, unsigned long,
                                                    0, ULONG_MAX);
                        else        STORE_DSTI(r.i, long,
                                                    LONG_MIN, LONG_MAX);
                        break;
#endif
                    default:
                        if (unsign) STORE_DSTU(r.u, unsigned,
                                                    0, UINT_MAX);
                        else        STORE_DSTI(r.i, int,
                                                    INT_MIN, INT_MAX);
                    }
                }
                break;
            } /* =========== READ INT =========== */

            case C_('e'): case C_('E'): /* scientific format float */
            case C_('f'): case C_('F'): /* decimal format float */
            case C_('g'): case C_('G'): /* decimal/scientific format float */
            case C_('a'): case C_('A'): /* hex format float */
                /* all treated equal by scanf, but not by printf */
#if SCANF_DISABLE_SUPPORT_FLOAT
                /* no support here */
                MATCH_FAILURE();
#else
            { /* =========== READ FLOAT =========== */
                floatmax_t r;
                /* negative? allow zero? hex mode? */
                BOOL negative = 0, zero = 0, hex = 0;
                if (!maxlen) maxlen = SIZE_MAX;

                switch (next) {
                case C_('-'):
                    negative = 1;
                    /* fall-through */
                case C_('+'):
                    NEXT_CHAR(nowread);
                }

#if SCANF_INFINITE
                if (KEEP_READING() && ICASEEQ(next, 'N', 'n')) {
                    NEXT_CHAR(nowread);
                    if (!KEEP_READING() || !ICASEEQ(next, 'A', 'a'))
                        MATCH_FAILURE();
                    NEXT_CHAR(nowread);
                    if (!KEEP_READING() || !ICASEEQ(next, 'N', 'n'))
                        MATCH_FAILURE();
                    NEXT_CHAR(nowread);
                    if (KEEP_READING() && next == C_('(')) {
                        while (KEEP_READING()) {
                            NEXT_CHAR(nowread);
                            if (next == C_(')')) {
                                NEXT_CHAR(nowread);
                                break;
                            } else if (next != C_('_') && !F_(isalnum)(next))
                                MATCH_FAILURE();
                        }
                    }
                    r = negative ? -NAN : NAN;
                    goto storefp;
                } else if (KEEP_READING() && ICASEEQ(next, 'I', 'i')) {
                    NEXT_CHAR(nowread);
                    if (!KEEP_READING() || !ICASEEQ(next, 'N', 'n'))
                        MATCH_FAILURE();
                    NEXT_CHAR(nowread);
                    if (!KEEP_READING() || !ICASEEQ(next, 'F', 'f'))
                        MATCH_FAILURE();
                    NEXT_CHAR(nowread);
                    /* try reading the rest */
                    if (KEEP_READING()) {
                        int k;
                        const CHAR *rest2 = S_("INITY");
                        for (k = 0; k < 5; ++k) {
                            if (!KEEP_READING() ||
                                    (next != rest2[k] &&
                                     next != F_(tolower)(rest2[k])))
                                break;
                            NEXT_CHAR(nowread);
                        }
                    }
                    r = negative ? -INFINITY : INFINITY;
                    goto storefp;
                }
#endif /* SCANF_INFINITE */

                /* 0x for hex floats */
                if (KEEP_READING() && next == C_('0')) {
                    zero = 1;
                    NEXT_CHAR(nowread);
                    if (KEEP_READING() && ICASEEQ(next, 'X', 'x')) {
                        hex = 1;
                        NEXT_CHAR(nowread);
                    }
                }

                /* convert */
                if (!F_(iatof_)(getch, p, &next, &nowread, maxlen, hex,
                                negative, zero, &r))
                    MATCH_FAILURE();

#if SCANF_INFINITE
            storefp:
#endif
                MATCH_SUCCESS();
                if (nostore)
                    break;
                ++fields;
                switch (dlen) {
                case LN_L:
#if !LDOUBLE_IS_DOUBLE
                    STORE_DST(r, long double);
                    break;
#endif
                case LN_l:
#if !DOUBLE_IS_FLOAT
                    STORE_DST(r, double);
                    break;
#endif
                default:
                    STORE_DST(r, float);
                }
                break;
            } /* =========== READ FLOAT =========== */
#endif /* SCANF_DISABLE_SUPPORT_FLOAT */

            case C_('c'):
            { /* =========== READ CHAR =========== */
                CHAR *outp;
#if SCANF_WIDE_CONVERT
                BOOL wide = dlen == LN_l;
#elif SCANF_WIDE /* SCANF_WIDE_CONVERT */
                if (dlen != LN_l) /* read narrow but not supported */
                    MATCH_FAILURE();
#else /* SCANF_WIDE_CONVERT */
                if (dlen == LN_l) /* read wide but not supported */
                    MATCH_FAILURE();
#endif /* SCANF_WIDE_CONVERT */
                outp = (CHAR *)dst;
                if (!maxlen) maxlen = 1;
#if SCANF_WIDE_CONVERT
#if SCANF_WIDE
                if (!wide) { /* convert wide => narrow */
#else /* SCANF_WIDE */
                if (wide) { /* convert narrow => wide */
#endif /* SCANF_WIDE */
                    if (!F_(iscvts_)(getch, p, &next, &nowread, maxlen,
                                A_CHAR,
#if !SCANF_DISABLE_SUPPORT_SCANSET
                                NULL,
#endif
                                nostore, (CVTCHAR *)dst))
                        MATCH_FAILURE();
                } else
#endif /* SCANF_WIDE_CONVERT */
                if (!F_(iscans_)(getch, p, &next, &nowread, maxlen,
                                A_CHAR,
#if !SCANF_DISABLE_SUPPORT_SCANSET
                                NULL,
#endif
                                nostore, outp))
                    MATCH_FAILURE();
                if (!nostore) ++fields;
                MATCH_SUCCESS();
                break;
            } /* =========== READ CHAR =========== */

            case C_('s'):
            { /* =========== READ STR =========== */
                CHAR *outp;
#if SCANF_WIDE_CONVERT
                BOOL wide = dlen == LN_l;
#elif SCANF_WIDE /* SCANF_WIDE_CONVERT */
                if (dlen != LN_l) /* read narrow but not supported */
                    MATCH_FAILURE();
#else /* SCANF_WIDE_CONVERT */
                if (dlen == LN_l) /* read wide but not supported */
                    MATCH_FAILURE();
#endif /* SCANF_WIDE_CONVERT */
                outp = (CHAR *)dst;
                if (!maxlen) {
#if SCANF_SECURE
                    if (!nostore) MATCH_FAILURE();
#endif
                    maxlen = SIZE_MAX;
                }
#if SCANF_WIDE_CONVERT
#if SCANF_WIDE
                if (!wide) { /* convert wide => narrow */
#else /* SCANF_WIDE */
                if (wide) { /* convert narrow => wide */
#endif /* SCANF_WIDE */
                    if (!F_(iscvts_)(getch, p, &next, &nowread, maxlen,
                                A_STRING,
#if !SCANF_DISABLE_SUPPORT_SCANSET
                                NULL,
#endif
                                nostore, (CVTCHAR *)dst))
                        MATCH_FAILURE();
                } else
#endif /* SCANF_WIDE_CONVERT */
                if (!F_(iscans_)(getch, p, &next, &nowread, maxlen,
                                A_STRING,
#if !SCANF_DISABLE_SUPPORT_SCANSET
                                NULL,
#endif
                                nostore, outp))
                    MATCH_FAILURE();
                if (!nostore) ++fields;
                MATCH_SUCCESS();
                break;
            } /* =========== READ STR =========== */

            case C_('['):
#if SCANF_DISABLE_SUPPORT_SCANSET
                MATCH_FAILURE();
#else
            { /* =========== READ SCANSET =========== */
                CHAR *outp;
                BOOL invert = 0;
#if SCANF_CAN_FAST_SCANSET
                BOOL hyphen = 0;
                BOOL mention[UCHAR_MAX + 1] = { 0 };
                UCHAR prev = 0, c;
#else
                const UCHAR *set;
#endif
                struct F_(scanset_) scanset;
#if SCANF_WIDE_CONVERT
                BOOL wide = dlen == LN_l;
#elif SCANF_WIDE /* SCANF_WIDE_CONVERT */
                if (dlen != LN_l) /* read narrow but not supported */
                    MATCH_FAILURE();
#else /* SCANF_WIDE_CONVERT */
                if (dlen == LN_l) /* read wide but not supported */
                    MATCH_FAILURE();
#endif /* SCANF_WIDE_CONVERT */
                outp = (CHAR *)dst;
                if (!maxlen) {
#if SCANF_SECURE
                    if (!nostore) MATCH_FAILURE();
#endif
                    maxlen = SIZE_MAX;
                }
                ++f;
                if (*f == C_('^'))
                    invert = 1, ++f;
                if (*f == C_(']'))
                    ++f;
#if SCANF_CAN_FAST_SCANSET
                /* populate mention, 0 if character not listed, 1 if it is */
                while ((c = *f) && c != C_(']')) {
                    if (hyphen) {
                        int k;
                        for (k = prev; k <= c; ++k)
                            mention[k] = 1;
                        hyphen = 0;
                        prev = c;
                    } else if (c == C_('-') && prev)
                        hyphen = 1;
                    else
                        mention[c] = 1, prev = c;
                    ++f;
                }
                if (hyphen)
                    mention[C_('-')] = 1;
                scanset.mask = mention;
#else /* SCANF_CAN_FAST_SCANSET */
                set = f;
                while (*f && *f != C_(']'))
                    ++f;
                scanset.set_begin = set, scanset.set_end = f;
#endif /* SCANF_CAN_FAST_SCANSET */
                scanset.invert = invert;
#if SCANF_WIDE_CONVERT
#if SCANF_WIDE
                if (!wide) { /* convert wide => narrow */
#else /* SCANF_WIDE */
                if (wide) { /* convert narrow => wide */
#endif /* SCANF_WIDE */
                    if (!F_(iscvts_)(getch, p, &next, &nowread, maxlen,
                            A_SCANSET, &scanset, nostore, (CVTCHAR *)dst))
                        MATCH_FAILURE();
                } else
#endif /* SCANF_WIDE_CONVERT */
                {
                    if (!F_(iscans_)(getch, p, &next, &nowread, maxlen,
                            A_SCANSET, &scanset, nostore, outp))
                        MATCH_FAILURE();
                }
                if (!nostore) ++fields;
                MATCH_SUCCESS();
                break;
            } /* =========== READ SCANSET =========== */
#endif /* SCANF_DISABLE_SUPPORT_SCANSET */
            default:
                /* unrecognized specification */
                MATCH_FAILURE();
            }

            ++f; /* next fmt char */
            read_chars += nowread;
        }
    }
read_failure:
    /* if we have a leftover character, put it back into the stream */
    if (!GOT_EOF() && ungetch)
        ungetch(next, p);
    return tryconv && noconv ? EOF : fields;
}

/* =============================== *
 *        wrapper functions        *
 * =============================== */

static CINT F_(sscanw_)(void *arg) {
    const UCHAR **p = (const UCHAR **)arg;
    const UCHAR c = *(*p)++;
    return c ? c : GCEOF;
}

#if SCANF_WIDE

#if SCANF_SSCANF_ONLY

int vwscanf_(const WCHAR *format, va_list arg) {
    return *format ? EOF : 0;
}

int wscanf_(const WCHAR *format, ...) {
    return *format ? EOF : 0;
}

#else /* SCANF_SSCANF_ONLY */

static WINT getwchw_(void *arg) {
    (void)arg;
    return getwch_();
}

static void ungetwchw_(WINT c, void *arg) {
    (void)arg;
    ungetwch_(c);
}

int vwscanf_(const WCHAR *format, va_list arg) {
    return F_(iscanf_)(&getwchw_, &ungetwchw_, NULL, format, arg);
}

int wscanf_(const WCHAR *format, ...) {
    int r;
    va_list va;
    va_start(va, format);
    r = vwscanf_(format, va);
    va_end(va);
    return r;
}

#endif /* SCANF_SSCANF_ONLY */

int vspwscanf_(const WCHAR **sp, const WCHAR *format, va_list arg) {
    int i = F_(iscanf_)(&F_(sscanw_), NULL, sp, format, arg);
    /* ungetch = NULL, because see below */
    --*sp; /* back up by one character, even if it was EOF we want the pointer
              at the null terminator */
    return i;
}

int spwscanf_(const WCHAR **sp, const WCHAR *format, ...) {
    int r;
    va_list va;
    va_start(va, format);
    r = vspwscanf_(sp, format, va);
    va_end(va);
    return r;
}

int vswscanf_(const WCHAR *s, const WCHAR *format, va_list arg) {
    return F_(iscanf_)(&F_(sscanw_), NULL, &s, format, arg);
}

int swscanf_(const WCHAR *s, const WCHAR *format, ...) {
    int r;
    va_list va;
    va_start(va, format);
    r = vswscanf_(s, format, va);
    va_end(va);
    return r;
}

int vfctwscanf_(WINT (*getwch)(void *data),
                void (*ungetwch)(WINT c, void *data),
                void *data, const WCHAR *format, va_list arg) {
    return F_(iscanf_)(getwch, ungetwch, data, format, arg);
}

int fctwscanf_(WINT (*getwch)(void *data),
                void (*ungetwch)(WINT c, void *data),
                void *data, const WCHAR *format, ...) {
    int r;
    va_list va;
    va_start(va, format);
    r = vfctwscanf_(getwch, ungetwch, data, format, va);
    va_end(va);
    return r;
}

#else /* SCANF_WIDE */

#if SCANF_SSCANF_ONLY

int vscanf_(const char *format, va_list arg) {
    return *format ? EOF : 0;
}

int scanf_(const char *format, ...) {
    return *format ? EOF : 0;
}

#else /* SCANF_SSCANF_ONLY */

static int getchw_(void *arg) {
    (void)arg;
    return getch_();
}

static void ungetchw_(int c, void *arg) {
    (void)arg;
    ungetch_(c);
}

int vscanf_(const char *format, va_list arg) {
    return iscanf_(&getchw_, &ungetchw_, NULL, format, arg);
}

int scanf_(const char *format, ...) {
    int r;
    va_list va;
    va_start(va, format);
    r = vscanf_(format, va);
    va_end(va);
    return r;
}

#endif /* SCANF_SSCANF_ONLY */

int vspscanf_(const char **sp, const char *format, va_list arg) {
    int i = iscanf_(&F_(sscanw_), NULL, sp, format, arg);
    --*sp; /* back up by one character, even if it was EOF we want the pointer
              at the null terminator */
    return i;
}

int spscanf_(const char **sp, const char *format, ...) {
    int r;
    va_list va;
    va_start(va, format);
    r = vspscanf_(sp, format, va);
    va_end(va);
    return r;
}

int vsscanf_(const char *s, const char *format, va_list arg) {
    return iscanf_(&F_(sscanw_), NULL, &s, format, arg);
}

int sscanf_(const char *s, const char *format, ...) {
    int r;
    va_list va;
    va_start(va, format);
    r = vsscanf_(s, format, va);
    va_end(va);
    return r;
}

int vfctscanf_(int (*getch)(void *data), void (*ungetch)(int c, void *data),
                void *data, const char *format, va_list arg) {
    return iscanf_(getch, ungetch, data, format, arg);
}

int fctscanf_(int (*getch)(void *data), void (*ungetch)(int c, void *data),
                void *data, const char *format, ...) {
    int r;
    va_list va;
    va_start(va, format);
    r = vfctscanf_(getch, ungetch, data, format, va);
    va_end(va);
    return r;
}

#endif /* SCANF_WIDE */

#if SCANF_WIDE >= 2
/* reinclude with SCANF_WIDE=0 to get narrow/multibyte functions */
#undef SCANF_WIDE
#define SCANF_WIDE 0
#define SCANF_REPEAT
#include "scanf.c"
#endif
