// +build ignore

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

package main

import (
	"flag"
	"fmt"
	"net/http"
	"strings"
)

func main() {
	root := flag.String("root", "", "Server root")
	port := flag.Int("port", 8080, "Server port")
	flag.Parse()

	fs := http.FileServer(http.Dir(*root))
	fmt.Println("Starting server!")

	if err := http.ListenAndServe(fmt.Sprintf(":%d", *port), http.HandlerFunc(func(resp http.ResponseWriter, req *http.Request) {
		resp.Header().Add("Cache-Control", "no-cache")
		if strings.HasSuffix(req.URL.Path, ".wasm") {
			resp.Header().Set("content-type", "application/wasm")
		}
		fs.ServeHTTP(resp, req)
	})); err != nil {
		panic(err)
	}
}
