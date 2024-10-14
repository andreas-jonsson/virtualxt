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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

#ifdef TESTING

#ifndef _TESTING_H_
#define _TESTING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static struct Test {
   const char *name;
   FILE *output;
} test_state = {
   .name = "",
   .output = NULL
};

#define TASSERT(e, ...)    if (!(e)) { fprintf(T.output, "TEST FAILED! -> %s (%s:%d)\n", T.name, __FILE__, __LINE__); fprintf(T.output, __VA_ARGS__); fputc('\n', T.output); return -1; }
#define TENSURE(e)         TASSERT((e), "%s", "ENSURE failed!")
#define TENSURE_NO_ERR(e)  { vxt_error __err = (e); TASSERT(__err == VXT_NO_ERROR, "%s", vxt_error_str(__err)); }
#define TEST(n, ...)       int test_ ## n (struct Test T) { T.name = #n ; { __VA_ARGS__ } return 0; }
#define TLOG(...)          { fprintf(T.output, __VA_ARGS__); fprintf(T.output, "%s", "\n"); }
#define TALLOC             realloc
#define TFREE(p)           { void *_ = realloc(p, 0); (void)_; }

static bool run_test(int (*t)(struct Test T)) {
   if (!test_state.output) test_state.output = stderr;
   return t(test_state) == 0;
}

#ifdef __cplusplus
}
#endif

#endif

#else

#define TEST(n, c)

#endif
