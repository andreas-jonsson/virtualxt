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

package mda

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
