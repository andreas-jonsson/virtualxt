/*

scanf implementation -- wide type implementations for C95/C99
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

#include <string.h>
#include <wchar.h>
#include "wide.h"

void mbsetup_(scanf_mbstate_t *ps) {
    memset(ps, 0, sizeof(scanf_mbstate_t));
}

size_t mbrtowc_(wchar_t *pwc, const char *s, size_t n, scanf_mbstate_t *ps) {
    return mbrtowc(pwc, s, n, ps);
}

size_t wcrtomb_(char *s, wchar_t wc, scanf_mbstate_t *ps) {
    return wcrtomb(s, wc, ps);
}
