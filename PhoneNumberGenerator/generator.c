/*
 * Author: @F1D0
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <windows.h>
#include <tre/tre.h>

#define PREFIX_SIZE 2
#define NUM_SIZE 10

#define RED 0x00CF
#define GREEN 0x00AF
#define BLUE 0x009F
#define PURPLE 0x00DF

/* MAX NL AUTHORIZED BY THE PROGRAM */
static const unsigned long maxNL = 10000;
static unsigned long nl = 6;

static char* prefix = "06";

static FILE* pStream, * nlStream;

static HANDLE hConsole;
static CONSOLE_SCREEN_BUFFER_INFOEX consoleInfo;

void sigHandler(int dummy) {
	SetConsoleTextAttribute(hConsole, PURPLE);
	printf("Program interrupted. Exiting quietly...");
	GetConsoleScreenBufferInfoEx(hConsole, &consoleInfo);
	exit(dummy);
}

void printError(const char* format, ...) {
	va_list marker;
	va_start(marker, format);
	SetConsoleTextAttribute(hConsole, RED);
	vfprintf(stderr, format, marker);
	GetConsoleScreenBufferInfoEx(hConsole, &consoleInfo);
	va_end(marker);
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	signal(SIGINT, sigHandler);

	consoleInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

	/* Store the current console infos */
	GetConsoleScreenBufferInfoEx(hConsole, &consoleInfo);

	if (argc < 2)
		printError("Usage: %s pattern.txt [06] [12]", argv[0]);

	errno_t rcode;

	rcode = fopen_s(&pStream, argv[1], "rb");
	if (rcode != 0)
		printError("Can't access file %s", argv[1]);

	if (argc > 2) {
		if (strlen(argv[2]) != PREFIX_SIZE)
			printError("Prefix has to be composed of %d digits", PREFIX_SIZE);
		else
			prefix = argv[2];
	}


	if (argc > 3) {
		unsigned long value = strtoul(argv[3], NULL, 10);
		if (value > maxNL)
			printError("Number of numlists is too large");
		else
			nl = value;
	}

	/* Get the file's size */
	fseek(pStream, 0, SEEK_END);
	long size = ftell(pStream);

	char* filter = malloc(sizeof(char) * (size_t)size);
	if (filter == NULL)
		printError("Memory exceeded !");

	size_t blocks = fread(filter, (size_t)size, 1, pStream);
	if (blocks != size)
		printError("Could not read all blocks from %s", argv[1]);

	regex_t preg;

	if (tre_regcomp(&preg, filter, REG_EXTENDED) != REG_OK)
		printError("Could not compile regex \"%s\" from %s", filter, argv[1]);


	int y, i, j;

	for (y = 0; y < nl; ++y) {
		char numlist[16];
		snprintf(numlist, sizeof(numlist), "numlist%d.txt", y + 1);
		SetConsoleTextAttribute(hConsole, GREEN);
		printf("Generating %s...", numlist);

		fopen_s(&nlStream, numlist, "w");
		int current = y * 10000 / nl;
		int after = (y + 1) * 10000 / nl;
		if (!y)
			after += 10000 % nl;
		char buffer[5]; // including null terminator
		for (i = current; i < after; ++i) {
			snprintf(buffer, sizeof(buffer), "%04d", i);
			if (tre_regexec(&preg, buffer, 0, NULL, 0)) {
				for (j = 0; j < 10000; ++j) {
					fprintf(nlStream, "%s%s%04d", prefix, buffer, j);
				}
			}
		}
		SetConsoleTextAttribute(hConsole, BLUE);
		printf("%s Done !", numlist);
		fclose(nlStream);
	}
	GetConsoleScreenBufferInfoEx(hConsole, &consoleInfo);
	free(filter);
}