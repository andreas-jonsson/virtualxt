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
	"log"
	"os"
	"os/exec"
	"path"
	"strings"
	"text/template"
	"time"
)

func main() {
	file := flag.String("file", "-", "Save the generated output to file.")
	pkg := flag.String("package", "version", "Package name of the generated output.")
	ver := flag.String("variable", "FULL_VERSION", "Environment variable containing the version number.")
	flag.Parse()

	cmd := exec.Command("git", "rev-parse", "HEAD")
	res, err := cmd.Output()
	if err != nil {
		log.Print("could not parse Git hash: ", err)
	}

	defaultVersion := "0.0.1.0"
	version := os.Getenv(*ver)
	if version == "" {
		version = defaultVersion
		log.Printf("%s is not set. Defaulting to %s", *ver, version)
	}

	parts := strings.SplitN(version, ".", 4)
	if len(parts) != 4 {
		log.Print("invalid version format: ", version)
		version = defaultVersion
		parts = strings.Split(version, ".")
	}

	const (
		startYear    = 2019
		copyrightFmt = "Copyright (c) %v Andreas T Jonsson"
	)

	copyrightString := fmt.Sprintf(copyrightFmt, startYear)
	if year := time.Now().Year(); year != startYear {
		copyrightString = fmt.Sprintf(copyrightFmt, fmt.Sprintf("%d-%d", startYear, year))
	}

	if parts[3] == "0" {
		parts[3] = ""
	}

	values := map[string]interface{}{
		"hash":  strings.TrimSpace(string(res)),
		"major": parts[0],
		"minor": parts[1],
		"patch": parts[2],
		"build": parts[3],
		"copy":  copyrightString,
		"pkg":   *pkg,
	}

	tmpl := template.New("version")
	tmpl = template.Must(tmpl.Parse(content))
	os.MkdirAll(path.Dir(*file), 0777)

	fp := os.Stdout
	if *file != "-" {
		fp, err = os.Create(*file)
		if err != nil {
			log.Panicln(err)
		}
		defer fp.Close()
	}

	if err := tmpl.Execute(fp, values); err != nil {
		log.Panicln(err)
	}
}

var content = `/*
{{.copy}}

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

package {{.pkg}}

var (
	Current = Version{ {{.major}}, {{.minor}}, {{.patch}}, "{{.build}}" }
	Copyright = "{{.copy}}"
	Hash = "{{.hash}}"
)
`
