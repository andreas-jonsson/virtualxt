#ifndef _VISUALSTUDIO_H_
#define _VISUALSTUDIO_H_

#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void Initialize(char* p0);

extern void Begin(unsigned char p0, struct Registers* p1);

extern void End(struct Registers* p0);

extern void Discard();

extern void ReadByte(unsigned int p0, unsigned char p1);

extern void WriteByte(unsigned int p0, unsigned char p1);

extern void Shutdown();

#ifdef __cplusplus
}
#endif

#endif
