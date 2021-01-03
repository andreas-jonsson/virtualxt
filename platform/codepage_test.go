/*
Copyright (c) 2019-2021 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

package platform

import (
	"testing"
)

func TestCodePage(t *testing.T) {
	t.Run("ASCII", func(t *testing.T) {
		for i := 32; i < 127; i++ {
			if codePage437[i] != rune(i) {
				t.Errorf("codePage437[%d] != rune(%d)", i, i)
			}
		}
	})

	t.Run("Duplicates", func(t *testing.T) {
		for i := 0; i < 256; i++ {
			for j := 0; j < 256; j++ {
				if codePage437[i] == codePage437[j] && i != j {
					t.Errorf("codePage437[%d] == codePage437[%d]", i, j)
				}
			}
		}
	})
}
