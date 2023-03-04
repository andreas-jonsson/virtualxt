// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#ifndef _RIFS_H_
#define _RIFS_H_

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>

    #ifndef F_OK
        #define F_OK 0
    #endif
    #ifndef strcasecmp
        #define strcasecmp _stricmp
    #endif
    #ifndef mkdir
        #define mkdir _mkdir
    #endif
    #ifndef rmdir
        #define rmdir _rmdir
    #endif
    #ifndef access
        #define access _access
    #endif
    #ifndef dirname
        #define dirname _dirname
    #endif
#else
    #include <alloca.h>
    #include <strings.h>
    #include <libgen.h>
    #include <unistd.h>
#endif

#endif
