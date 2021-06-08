#ifndef _VISUALSTUDIO_H_
#define _VISUALSTUDIO_H_

#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void Initialize(char* p0);

extern void Begin(unsigned char p0);

extern void End();

extern void Discard();

extern void ReadReg8(unsigned char reg, unsigned char data);

extern void WriteReg8(unsigned char reg, unsigned char data);

extern void ReadReg16(unsigned char reg, unsigned short data);

extern void WriteReg16(unsigned char reg, unsigned short data);

extern void ReadByte(unsigned int p0, unsigned char p1);

extern void WriteByte(unsigned int p0, unsigned char p1);

extern void Shutdown();

#ifdef __cplusplus
}
#endif

#endif
