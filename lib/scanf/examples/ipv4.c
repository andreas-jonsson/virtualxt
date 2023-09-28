/*

scanf implementation (example scnext_ for IPv4 addresses)
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

#include "../scanf.h"
#include <stdint.h>

/* see EXTENSIONS for more details. this is an example implementation of a
   custom format specifiers for reading an IPv4 address and returning it as
   a uint32_t. */

enum rescanf_state { RESCANF_BUFFER, RESCANF_STREAM, RESCANF_EOF };

struct rescanf_tmp {
    int (*getch)(void *data);
    void *udata;
    int next;
    enum rescanf_state state;
};

int rescanf_getch_(void *data) {
    struct rescanf_tmp *rsf = (struct rescanf_tmp *)data;
    int c;
    switch (rsf->state) {
    case RESCANF_BUFFER:
        c = rsf->next;
        rsf->state = RESCANF_STREAM;
        break;
    case RESCANF_STREAM:
        c = rsf->getch(rsf->udata);
        if (c < 0)
            rsf->state = RESCANF_EOF;
        break;
    case RESCANF_EOF:
        c = -1;
    }
    return c;
}

void rescanf_ungetch_(int ch, void *data) {
    struct rescanf_tmp *rsf = (struct rescanf_tmp *)data;
    rsf->next = ch;
}

/* for calling scanf within scnext_ */
int rescanf(int (*getch)(void *data), void *data, int *next, 
            const char* format, ...) {
    int r;
    va_list va;
    struct rescanf_tmp tmp;
    va_start(va, format);
    tmp.udata = data;
    tmp.getch = getch;
    tmp.next = *next;
    tmp.state = tmp.next < 0 ? RESCANF_EOF : RESCANF_BUFFER;
    r = vfctscanf(&rescanf_getch_, &rescanf_ungetch_, &tmp, format, va);
    va_end(va);
    *next = tmp.next;
    return r;
}

/* IPv4: %!I4 */
int scnext_(int (*getch)(void *data), void *data, const char **format,
            int *buffer, int length, int nostore, void *destination) {
    const char *f = *format;
    int next = *buffer;
    int ret = 0;

    if (f[0] == 'I' && f[1] == '4') {
        unsigned char i1, i2, i3, i4;
        /* use rescanf to call scanf in scnext_ */
        int r = rescanf(getch, data, &next, "%hhu.%hhu.%hhu.%hhu",
                        &i1, &i2, &i3, &i4);
        f += 2; /* consume I4 in format string */
        if (r < 0)
            ret = r; /* input error */
        else if (r < 4)
            ret = 1; /* matching error */
        else if (!nostore) {
            uint32_t ip = (i1 << 24) | (i2 << 16) | (i3 << 8) | (i4);
            *(uint32_t *)destination = ip;
        }
        ret = 0; /* OK */
    } else
        ret = 1; /* unknown specifier, matching error */

    *format = f;
    *buffer = next;
    return ret;
}
