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

#ifndef SCANF_H
#define SCANF_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define ATTR_scanf     __attribute__ ((format (scanf, 1, 2)))
#define ATTR_sscanf    __attribute__ ((format (scanf, 2, 3)))
#define ATTR_spscanf   __attribute__ ((format (scanf, 2, 3)))
#define ATTR_fctscanf  __attribute__ ((format (scanf, 4, 5)))
#define ATTR_vscanf    __attribute__ ((format (scanf, 1, 0)))
#define ATTR_vsscanf   __attribute__ ((format (scanf, 2, 0)))
#define ATTR_vspscanf  __attribute__ ((format (scanf, 2, 0)))
#define ATTR_vfctscanf __attribute__ ((format (scanf, 4, 0)))
#else
#define ATTR_scanf
#define ATTR_sscanf
#define ATTR_spscanf
#define ATTR_fctscanf
#define ATTR_vscanf
#define ATTR_vsscanf
#define ATTR_vspscanf
#define ATTR_vfctscanf
#endif

ATTR_scanf int scanf_(const char *format, ...);
ATTR_sscanf int sscanf_(const char *s, const char *format, ...);
ATTR_spscanf int spscanf_(const char **sp, const char *format, ...);
ATTR_fctscanf int fctscanf_(int (*getch)(void *data),
                         void (*ungetch)(int c, void *data),
                         void *data, const char *format, ...);

ATTR_vscanf int vscanf_(const char *format, va_list arg);
ATTR_vsscanf int vsscanf_(const char *s, const char *format, va_list arg);
ATTR_vspscanf int vspscanf_(const char **sp, const char *format, va_list arg);
ATTR_vfctscanf int vfctscanf_(int (*getch)(void *data),
                              void (*ungetch)(int c, void *data),
                              void *data, const char *format, va_list arg);

int getch_(void);
void ungetch_(int);

#ifndef SCANF_NODEFINE
#ifndef SCANF_NOCOLLIDE
#define scanf scanf_
#define sscanf sscanf_
#define vscanf vscanf_
#define vsscanf vsscanf_
#endif

#define spscanf spscanf_
#define fctscanf fctscanf_
#define vspscanf vspscanf_
#define vfctscanf vfctscanf_
#endif

#ifdef __cplusplus
}
#endif

#endif /* SCANF_H */
