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

#ifndef WSCANF_H
#define WSCANF_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIDE_H
#include "wide.h"
#endif

#ifndef WCHAR
#define WCHAR wchar_t
#endif

#ifndef WINT
#define WINT wint_t
#endif

#if 0
/* #ifdef __GNUC__ // does not currently support wide characters for this */
#define ATTR_wscanf     __attribute__ ((format (scanf, 1, 2)))
#define ATTR_swscanf    __attribute__ ((format (scanf, 2, 3)))
#define ATTR_spwscanf   __attribute__ ((format (scanf, 2, 3)))
#define ATTR_fctwscanf  __attribute__ ((format (scanf, 4, 5)))
#define ATTR_vwscanf    __attribute__ ((format (scanf, 1, 0)))
#define ATTR_vswscanf   __attribute__ ((format (scanf, 2, 0)))
#define ATTR_vspwscanf  __attribute__ ((format (scanf, 2, 0)))
#define ATTR_vfctwscanf __attribute__ ((format (scanf, 4, 0)))
#else
#define ATTR_wscanf
#define ATTR_swscanf
#define ATTR_spwscanf
#define ATTR_fctwscanf
#define ATTR_vwscanf
#define ATTR_vswscanf
#define ATTR_vspwscanf
#define ATTR_vfctwscanf
#endif

ATTR_wscanf int wscanf_(const WCHAR *format, ...);
ATTR_swscanf int swscanf_(const WCHAR *s, const WCHAR *format, ...);
ATTR_spwscanf int spwscanf_(const WCHAR **sp, const WCHAR *format, ...);
ATTR_fctwscanf int fctwscanf_(WINT (*getwch)(void *data),
                         void (*ungetwch)(WINT c, void *data),
                         void *data, const wchar_t *format, ...);

ATTR_vwscanf int vwscanf_(const WCHAR *format, va_list arg);
ATTR_vswscanf int vswscanf_(const WCHAR *s, const WCHAR *format, va_list arg);
ATTR_vspwscanf int vspwscanf_(const WCHAR **sp, const WCHAR *format, va_list arg);
ATTR_vfctwscanf int vfctwscanf_(WINT (*getwch)(void *data),
                              void (*ungetwch)(WINT c, void *data),
                              void *data, const WCHAR *format, va_list arg);

WINT getwch_(void);
void ungetwch_(WINT);

/* for converting values, these are used by scanf */
void mbsetup_(scanf_mbstate_t *ps);
size_t mbrtowc_(WCHAR *pwc, const char *s, size_t n, scanf_mbstate_t *ps);
size_t wcrtomb_(char *s, WCHAR wc, scanf_mbstate_t *ps);

#ifndef SCANF_NODEFINE
#ifndef SCANF_NOCOLLIDE
#define wscanf wscanf_
#define swscanf swscanf_
#define vwscanf vwscanf_
#define vswscanf vswscanf_
#endif

#define spwscanf spwscanf_
#define fctwscanf fctwscanf_
#define vspwscanf vspwscanf_
#define vfctwscanf vfctwscanf_
#endif

#ifdef __cplusplus
}
#endif

#endif /* WSCANF_H */
