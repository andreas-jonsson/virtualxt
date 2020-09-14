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

package cgatext

var codePage437 = [256]rune{
	0x0000, // NULL
	0x263A, // ☺
	0x263B, // ☻
	0x2665, // ♥
	0x2666, // ♦
	0x2663, // ♣
	0x2660, // ♠
	0x2022, // •
	0x25D8, // ◘
	0x25CB, // ○
	0x25D9, // ◙
	0x2642, // ♂
	0x2640, // ♀
	0x266A, // ♪
	0x266B, // ♫
	0x263C, // ☼
	0x25BA, // ►
	0x25C4, // ◄
	0x2195, // ↕
	0x203C, // ‼
	0x00B6, // ¶
	0x00A7, // §
	0x25AC, // ▬
	0x21A8, // ↨
	0x2191, // ↑
	0x2193, // ↓
	0x2192, // →
	0x2190, // ←
	0x221F, // ∟
	0x2194, // ↔
	0x25B2, // ▲
	0x25BC, // ▼
	' ',
	'!',
	'"',
	'#',
	'$',
	'%',
	'&',
	0x0027, // '
	'(',
	')',
	'*',
	'+',
	',',
	'-',
	'.',
	'/',
	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	':',
	';',
	'<',
	'=',
	'>',
	'?',
	'@',
	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z',
	'[',
	0x005C, // \
	']',
	'^',
	'_',
	'`',
	'a',
	'b',
	'c',
	'd',
	'e',
	'f',
	'g',
	'h',
	'i',
	'j',
	'k',
	'l',
	'm',
	'n',
	'o',
	'p',
	'q',
	'r',
	's',
	't',
	'u',
	'v',
	'w',
	'x',
	'y',
	'z',
	'{',
	'|',
	'}',
	'~',
	0x2302, // ⌂
	0x00C7, // Ç
	0x00FC, // ü
	0x00E9, // é
	0x00E2, // â
	0x00E4, // ä
	0x00E0, // à
	0x00E5, // å
	0x00E7, // ç
	0x00EA, // ê
	0x00EB, // ë
	0x00E8, // è
	0x00EF, // ï
	0x00EE, // î
	0x00EC, // ì
	0x00C4, // Ä
	0x00C5, // Å
	0x00C9, // É
	0x00E6, // æ
	0x00C6, // Æ
	0x00F4, // ô
	0x00F6, // ö
	0x00F2, // ò
	0x00FB, // û
	0x00F9, // ù
	0x00FF, // ÿ
	0x00D6, // Ö
	0x00DC, // Ü
	0x00A2, // ¢
	0x00A3, // £
	0x00A5, // ¥
	0x20A7, // ₧
	0x0192, // ƒ
	0x00E1, // á
	0x00ED, // í
	0x00F3, // ó
	0x00FA, // ú
	0x00F1, // ñ
	0x00D1, // Ñ
	0x00AA, // ª
	0x00BA, // º
	0x00BF, // ¿
	0x2310, // ⌐
	0x00AC, // ¬
	0x00BD, // ½
	0x00BC, // ¼
	0x00A1, // ¡
	0x00AB, // «
	0x00BB, // »
	0x2591, // ░
	0x2592, // ▒
	0x2593, // ▓
	0x2502, // │
	0x2524, // ┤
	0x2561, // ╡
	0x2562, // ╢
	0x2556, // ╖
	0x2555, // ╕
	0x2563, // ╣
	0x2551, // ║
	0x2557, // ╗
	0x255D, // ╝
	0x255C, // ╜
	0x255B, // ╛
	0x2510, // ┐
	0x2514, // └
	0x2534, // ┴
	0x252C, // ┬
	0x251C, // ├
	0x2500, // ─
	0x253C, // ┼
	0x255E, // ╞
	0x255F, // ╟
	0x255A, // ╚
	0x2554, // ╔
	0x2569, // ╩
	0x2566, // ╦
	0x2560, // ╠
	0x2550, // ═
	0x256C, // ╬
	0x2567, // ╧
	0x2568, // ╨
	0x2564, // ╤
	0x2565, // ╥
	0x2559, // ╙
	0x2558, // ╘
	0x2552, // ╒
	0x2553, // ╓
	0x256B, // ╫
	0x256A, // ╪
	0x2518, // ┘
	0x250C, // ┌
	0x2588, // █
	0x2584, // ▄
	0x258C, // ▌
	0x2590, // ▐
	0x2580, // ▀
	0x03B1, // α
	0x00DF, // ß
	0x0393, // Γ
	0x03C0, // π
	0x03A3, // Σ
	0x03C3, // σ
	0x00B5, // µ
	0x03C4, // τ
	0x03A6, // Φ
	0x0398, // Θ
	0x03A9, // Ω
	0x03B4, // δ
	0x221E, // ∞
	0x03C6, // φ
	0x03B5, // ε
	0x2229, // ∩
	0x2261, // ≡
	0x00B1, // ±
	0x2265, // ≥
	0x2264, // ≤
	0x2320, // ⌠
	0x2321, // ⌡
	0x00F7, // ÷
	0x2248, // ≈
	0x00B0, // °
	0x2219, // ∙
	0x00B7, // ·
	0x221A, // √
	0x207F, // ⁿ
	0x00B2, // ²
	0x25A0, // ■
	0x00A0, // NBSP
}
