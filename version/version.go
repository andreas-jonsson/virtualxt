/*
Copyright (c) 2019-2020 Andreas T Jonsson

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
