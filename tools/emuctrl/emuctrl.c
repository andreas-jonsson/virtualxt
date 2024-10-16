#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define VERSION "1.1.0"
#define COMMAND_PORT 0xB4
#define DATA_PORT 0xB5
#define PPI_PORT 0x61

#if !defined(__BCC__) || !defined(__MSDOS__)
	#error Use BCC to produce MSDOS executable!
#endif

#ifdef __FIST_ARG_IN_AX__
	#error Unexpected calling convension!
#endif

/* Should be synced with frontend.h */
enum {
	FRONTEND_CTRL_RESET,
	FRONTEND_CTRL_SHUTDOWN,
	FRONTEND_CTRL_HAS_DATA,
	FRONTEND_CTRL_READ_DATA,
	FRONTEND_CTRL_WRITE_DATA,
	FRONTEND_CTRL_POPEN,
	FRONTEND_CTRL_PCLOSE,
	FRONTEND_CTRL_FCLOSE,
	FRONTEND_CTRL_PUSH,
	FRONTEND_CTRL_PULL,
	FRONTEND_CTRL_DEBUG
};

static int read_status(void) {
#asm
	xor ax, ax
	in al, COMMAND_PORT
#endasm
}

static void write_command_(unsigned char c) {
#asm
	push bx
	mov bx, sp
	mov al, [bx+4]
	out COMMAND_PORT, al
	pop bx
#endasm
}

static int write_command(unsigned char c) {
	write_command_(c);
	return read_status();
}

static void reset(void) {
	int status = write_command(FRONTEND_CTRL_RESET);
	assert(!status);
}

static void write_data(unsigned char c) {
#asm
	push bx
	mov bx, sp
	mov al, [bx+4]
	out DATA_PORT, al
	pop bx
#endasm
}

static unsigned char read_data(void) {
#asm
	xor ax, ax
	in al, DATA_PORT
#endasm
}

static void turbo_on(void) {
#asm
	in al, PPI_PORT
	and al, #0xFB
	out PPI_PORT, al
#endasm
}

static void turbo_off(void) {
#asm
	in al, PPI_PORT
	or al, #0x4
	out PPI_PORT, al
#endasm
}

static void print_help(void) {
	printf(
		"Usage: emuctrl [cmd] <args...>\n\n"
		"  version                 Display software information\n"
		"  shutdown                Terminates the emulator\n"
		"  popen     <args...>     Execute command on host and pipe input to guest\n"
		"  turbo     <on/off>      Set CPU turbo mode\n"
		"  push      [src] [dest]  Upload file from guest to host\n"
		"  pull      [src] [dest]  Download file from host to guest\n"
		"\n"
	);
	exit(-1);
}

char buffer[256] = {0};

int main(int argc, char *argv[]) {
	int i;
	char *ptr;
	FILE *fp;
	
	if (write_command(FRONTEND_CTRL_RESET) != 0) {
		printf("Make sure the VirtualXT 'ctrl' module is loaded.\n");
		return -1;
	}
	
	if (argc < 2)
		print_help();

	if (!strcmp(argv[1], "shutdown")) {
		reset();
		write_command(FRONTEND_CTRL_SHUTDOWN);
	} else if (!strcmp(argv[1], "turbo")) {
		if ((argc > 2) && !strcmp(argv[2], "off"))
			turbo_off();
		else
			turbo_on();
	} else if (!strcmp(argv[1], "popen")) {
		if (argc < 3) {
			printf("Host Command: ");
			gets(buffer);
		} else for (i = 2; i < argc; i++) {
			if (i > 2)
				strcat(buffer, " ");
			strcat(buffer, argv[i]);
		}
		
		reset();
		for (ptr = buffer; *ptr; ptr++)
			write_data(*ptr);
		write_data(0);
		
		if (write_command(FRONTEND_CTRL_POPEN)) {
			printf("Could not open process: %s\n", argv[2]);
			return -1;
		}
		
		while (write_command(FRONTEND_CTRL_HAS_DATA))
			putc(read_data(), stdout);
		write_command(FRONTEND_CTRL_PCLOSE);
	} else if (!strcmp(argv[1], "push")) {
		if (argc < 4)
			print_help();
		
		reset();
		for (ptr = argv[3]; *ptr; ptr++)
			write_data(*ptr);
		write_data(0);
		
		if (write_command(FRONTEND_CTRL_PUSH)) {
			printf("Could not copy: %s -> %s\n", argv[2], argv[3]);
			return -1;
		}
		
		if (!(fp = fopen(argv[2], "rb"))) {
			printf("Could not open: %s\n", argv[2]);
			return -1;
		}
		
		while ((i = fgetc(fp)) != EOF)
			write_data(i);
		
		write_command(FRONTEND_CTRL_FCLOSE);
		fclose(fp);
	} else if (!strcmp(argv[1], "pull")) {
		if (argc < 4)
			print_help();
		
		reset();
		for (ptr = argv[2]; *ptr; ptr++)
			write_data(*ptr);
		write_data(0);
		
		if (write_command(FRONTEND_CTRL_PULL)) {
			printf("Could not copy: %s -> %s\n", argv[2], argv[3]);
			return -1;
		}
		
		if (!(fp = fopen(argv[3], "wb"))) {
			printf("Could not open: %s\n", argv[3]);
			return -1;
		}
		
		while (write_command(FRONTEND_CTRL_HAS_DATA))
			fputc(read_data(), fp);
		
		write_command(FRONTEND_CTRL_FCLOSE);
		fclose(fp);
	} else if (!strcmp(argv[1], "version")) {
		printf("VirtualXT Control Software\n");
		printf("Version: %s\n", VERSION);
	} else {
		print_help();
	}
	return 0;
}
