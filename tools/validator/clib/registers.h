/*
Copyright (C) 2019-2020 Andreas T Jonsson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _REGISTERS_H_
#define _REGISTERS_H_

#include <stdbool.h>

#define DefulatQueueSize  0x400      // 1KB
#define DefaultBufferSize 0x40000000 // 1GB

struct Registers {
  unsigned short AX;
  unsigned short CX;
  unsigned short DX;
  unsigned short BX;
  unsigned short SP;
  unsigned short BP;
  unsigned short SI;
  unsigned short DI;
  unsigned short ES;
  unsigned short CS;
  unsigned short SS;
  unsigned short DS;
  unsigned short IP;

  bool CF;
  bool PF;
  bool AF;
  bool ZF;
  bool SF;
  bool TF;
  bool IF;
  bool DF;
  bool OF;
};

#endif
