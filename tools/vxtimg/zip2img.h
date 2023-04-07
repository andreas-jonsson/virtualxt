// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#ifndef _ZIP2IMG_H_
#define _ZIP2IMG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <miniz.h>
#include <fat16.h>

#include "freedos_minimal_40M.h"

#define MZ_PRINT_ERROR(a) { fprintf(stderr, "miniz error: %s\n", mz_zip_get_error_string(mz_zip_get_last_error(a))); }

enum img_template {
	ZIP2IMG_TMPL_FREEDOS_MINIMAL_40M
};

const int img_templates_size[] = {
	freedos_minimal_40M_size
};

const unsigned char *img_templates_data[] = {
	freedos_minimal_40M_data
};

FAT16 fat;
BLOCKDEV dev;
FILE *output;

static void block_seek(const uint32_t pos) {
	fseek(output, pos, SEEK_SET);
}

static void block_rseek(const int16_t pos) {
	fseek(output, pos, SEEK_CUR);
}

static void block_flush(void) {
	fflush(output);
}

static void block_load(void *dest, const uint16_t len) {
	fread(dest, len, 1, output);
}

static void block_store(const void* source, const uint16_t len) {
	fwrite(source, len, 1, output);
}

static void block_write(const uint8_t b) {
	fwrite(&b, 1, 1, output);
}

static uint8_t block_read(void) {
	uint8_t b;
	fread(&b, 1, 1, output);
	return b;
}

static bool block_open(const char *file) {
	dev = (BLOCKDEV){
		.read = &block_read,
		.write = &block_write,
		.load = &block_load,
		.store = &block_store,
		.seek = &block_seek,
		.rseek = &block_rseek,
		.flush = &block_flush
	};
	output = fopen(file, "rb+");
	return output != NULL;
}

static void block_close() {
	if (!output)
		return;
	fflush(output);
	fclose(output);
	output = NULL;
}

static bool create_dir(FFILE *file, const char *name) {
	if (!ff_mkdir(file, name)) {
		if (!ff_find(file, name)) {
			fprintf(stderr, "Could not find sub-directory: %s\n", name);
			return false;
		} else if (!ff_opendir(file)) {
			fprintf(stderr, "Could not open sub-directory: %s\n", name);
			return false;
		}
	}
	return true;
}

static char *strupper(char *str) {
	for (int i = 0; i < (int)strlen(str); i++)
		str[i] = toupper(str[i]);
	return str;
}

static bool write_img_file(const char *path, void *data, int size) {
	FFILE file;
	ff_root(&fat, &file);

	char name[13] = {0};
	const char *sub = path;
	char *pch = strchr(path, '/');

	while (pch != NULL) {
		int ln = pch - sub;
		if (ln > 12) {
			fprintf(stderr, "Name of sub-directory is to long: %s\n", path);
			return false;
		}

		strncpy(name, sub, ln);
		name[ln] = 0;
		if (!create_dir(&file, strupper(name)))
			return false;

		sub = pch + 1;
		pch = strchr(pch + 1, '/');
	}

	if (strlen(sub) > 12) {
		fprintf(stderr, "Name of file is to long: %s\n", sub);
		return false;
	}

	strcpy(name, sub);
	if (!ff_newfile(&file, strupper(name))) {
		fprintf(stderr, "Could not ff_newfile\n");
		return false;
	}

	if (!ff_write(&file, data, size)) {
		fprintf(stderr, "Could not ff_write\n");
		return false;
	}

	ff_flush_file(&file);
	return true;
}

static bool do_zip2img(mz_zip_archive *in) {
	int num = mz_zip_reader_get_num_files(in);
    for (int i = 0; i < num; i++) {
		mz_zip_archive_file_stat stat;
		if (!mz_zip_reader_file_stat(in, i, &stat)) {
			MZ_PRINT_ERROR(in);
			return false;
		}

		if (mz_zip_reader_is_file_a_directory(in, i))
			continue;

		size_t sz;
		void *data = mz_zip_reader_extract_file_to_heap(in, stat.m_filename, &sz, 0);
		if (!data) {
			MZ_PRINT_ERROR(in);
			return false;
		}

		bool res = write_img_file(stat.m_filename, data, (int)sz);
		mz_free(data);
		if (!res)
			return false;
	}
	return true;
}

static bool zip2img(const char *in, const char *out) {
	mz_zip_archive ar;
	memset(&ar, 0, sizeof(ar));
	mz_bool status = mz_zip_reader_init_file(&ar, in, 0);
	if (!status) {
		MZ_PRINT_ERROR(&ar);
		return false;
	}

	if (!block_open(out)) {
		fprintf(stderr, "Could not open: %s\n", out);
		mz_zip_reader_end(&ar);
		return false;
	}

	if (!ff_init(&dev, &fat)) {
		fprintf(stderr, "Could not initialize FAT16!\n");
		mz_zip_reader_end(&ar);
		block_close();
		return false;
	}

	bool res = do_zip2img(&ar);
    mz_zip_reader_end(&ar);
	block_close();
	return res;
}

static bool zip2img_tmpl(const char *in, const char *out, enum img_template tmpl) {
	mz_zip_archive ar;
	memset(&ar, 0, sizeof(ar));
	mz_bool status = mz_zip_reader_init_mem(&ar, img_templates_data[tmpl], img_templates_size[tmpl], 0);
	if (!status) {
		MZ_PRINT_ERROR(&ar);
		return false;
	}

	bool res = mz_zip_reader_extract_to_file(&ar, 0, out, 0);
	if (!res) {
		MZ_PRINT_ERROR(&ar);
		mz_zip_reader_end(&ar);
		return false;
	}
	mz_zip_reader_end(&ar);
	return in ? zip2img(in, out) : true;
}

static bool zip2img_create_autoexec(const char *img, const char *script) {
	if (!block_open(img)) {
		fprintf(stderr, "Could not open: %s\n", img);
		return false;
	}

	if (!ff_init(&dev, &fat)) {
		fprintf(stderr, "Could not initialize FAT16!\n");
		block_close();
		return false;
	}

	FFILE file;
	ff_root(&fat, &file);

	if (!ff_newfile(&file, "AUTOEXEC.BAT")) {
		fprintf(stderr, "Could not ff_newfile\n");
		block_close();
		return false;
	}

	if (!ff_write(&file, script, (uint32_t)strlen(script))) {
		fprintf(stderr, "Could not ff_write\n");
		block_close();
		return false;
	}

	ff_flush_file(&file);
	block_close();
	return true;
}

#endif
