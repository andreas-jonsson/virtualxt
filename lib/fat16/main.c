#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "fat16.h"



// ------------- test ----------------

BLOCKDEV test;
FILE* testf;

void test_seek(const uint32_t pos)
{
	fseek(testf, pos, SEEK_SET);
}

void test_rseek(const int16_t pos)
{
	fseek(testf, pos, SEEK_CUR);
}

void test_load(void* dest, const uint16_t len)
{
	fread(dest, len, 1, testf);
//	for (int a = 0; a < len; a++) {
//		fread(dest+a, 1, 1, testf);
//	}
}

void test_store(const void* source, const uint16_t len)
{
	fwrite(source, len, 1, testf);

//	for (int a = 0; a < len; a++) {
//		fwrite(source+a, 1, 1, testf);
//	}
}

void test_write(const uint8_t b)
{
	fwrite(&b, 1, 1, testf);
}

uint8_t test_read()
{
	uint8_t a;
	fread(&a, 1, 1, testf);
	return a;
}

void test_open()
{
	test.read = &test_read;
	test.write = &test_write;

	test.load = &test_load;
	test.store = &test_store;

	test.seek = &test_seek;
	test.rseek = &test_rseek;

	testf = fopen("imgs/hamlet.img", "rb+");
}

void test_close()
{
	fflush(testf);
	fclose(testf);
}


// --- testing ---

int main()
{
	test_open();

	// Initialize the FS
	FAT16 fat;
	ff_init(&test, &fat);

	FFILE file;
	ff_root(&fat, &file);

	char str[12];

	printf("Disk label: %s\n", ff_disk_label(&fat, str));

	do
	{
		if (!ff_is_regular(&file))
			continue;

		printf("File name: %s, %c, %d B, @ 0x%x\n",
			   ff_dispname(&file, str),
			   file.type, file.size, file.clu_start);

	}
	while (ff_next(&file));

//	fat16_root(&fat, &file);

//	if (fat16_mkfile(&file, "slepice8.ini"))
//	{
//		printf("created.\n");
//		char* moo = "Contrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of 'de Finibus Bonorum et Malorum' (The Extremes of Good and Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of Lorem Ipsum, 'Lorem ipsum dolor sit amet..', comes from a line in section 1.10.32.";

//		fat16_write(&file, moo, strlen(moo));
//	}

//	if (fat16_find(&file, "slepice2.ini"))
//	{
//		printf("Found.\n");

//		char m[1000000];
//		fat16_read(&file, m, file.size);
//		m[file.size] = 0;
//		printf("%s\n", m);

////		char* moo = "\n\nContrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of 'de Finibus Bonorum et Malorum' (The Extremes of Good and Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of Lorem Ipsum, 'Lorem ipsum dolor sit amet..', comes from a line in section 1.10.32. Contrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of 'de Finibus Bonorum et Malorum' (The Extremes of Good and Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of Lorem Ipsum, 'Lorem ipsum dolor sit amet..', comes from a line in section 1.10.32.Contrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of 'de Finibus Bonorum et Malorum' (The Extremes of Good and Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of Lorem Ipsum, 'Lorem ipsum dolor sit amet..', comes from a line in section 1.10.32.Contrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of 'de Finibus Bonorum et Malorum' (The Extremes of Good and Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of Lorem Ipsum, 'Lorem ipsum dolor sit amet..', comes from a line in section 1.10.32.Contrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of 'de Finibus Bonorum et Malorum' (The Extremes of Good and Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of Lorem Ipsum, 'Lorem ipsum dolor sit amet..', comes from a line in section 1.10.32.";

////		fat16_seek(&file, file.size);
////		fat16_write(&file, moo, strlen(moo));
//	}

//	if (fat16_find(&file, "banana2.exe"))
//	{
//		printf("Found.\n");
//		printf("Deleted? %d\n", fat16_delete(&file));
//	}

	test_close();

	return 0;
}
