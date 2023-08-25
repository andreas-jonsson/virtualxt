files {
    "rifs.c"
}
filter "toolset:clang or gcc"
	buildoptions "-Wno-format-truncation"
