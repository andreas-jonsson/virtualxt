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

//go:generate go run ../tools/version/version.go -file current.go

package version

import (
	"fmt"
	"reflect"
)

type Version struct {
	Major, Minor, Patch byte
	Build               string
}

func New(major, minor, patch byte) Version {
	return Version{major, minor, patch, ""}
}

func NewFromSlice(v []byte) Version {
	return Version{v[0], v[1], v[2], ""}
}

func (v Version) Slice() []byte {
	return []byte{v.Major, v.Minor, v.Patch}
}

func (v Version) String() string {
	return fmt.Sprintf("%d.%d.%d", v.Major, v.Minor, v.Patch)
}

func (v Version) FullString() string {
	if v.Build == "" {
		return v.String()
	}
	return fmt.Sprintf("%s-%s", v.String(), v.Build)
}

func (v Version) Equal(ver Version) bool {
	return reflect.DeepEqual(v, ver)
}

func (v Version) Compatible(ver Version) bool {
	return v.Major == ver.Major && v.Minor == ver.Minor
}
