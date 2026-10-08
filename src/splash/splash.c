#include <assert.h>  // assert
#include <stdarg.h>  // variable arguments: va_*, ...
#include <stdio.h>   // *printf, stderr, stdout
#include <stdlib.h>  // EXIT_FAILURE, getenv
#include <string.h>  // strcmp
#include <stdbool.h> // bool, true, false

#include "sdl_splash.h"


static const char * const PIC_PATH = "../rogue.pic";
static const char * PROGNAME = NULL;  // == argv[0], set by main()


static void usage(FILE* stream)
{
	fprintf(stream, "Usage: %s [-h|--help] [-d|--debug] [PIC_PATH]\n", PROGNAME);
}


// explain_output_error_and_die() using stdlib only
_Noreturn static void fatal(const char *format, ...)
{
	char message_text[1000];

	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message_text, sizeof(message_text), format, arguments);
	va_end(arguments);

	fprintf(stderr, "%s: %s\n", PROGNAME, message_text);

	usage(stderr);
	exit(EXIT_FAILURE);
}


int main(int argc, char* argv[])
{
	char* arg;
	const char* path = NULL;
	bool debug = false;

	PROGNAME = argv[0];

	while (--argc) {
		arg = *(++argv);
		if (!strcmp("-h", arg) || !strcmp("--help", arg)) {
			usage(stdout);
			printf("Display a 320x200 PIC image in BSAVE format using SDL\n");
			printf("Uses CGA colors, palette `1i` (Black / Cyan / Magenta / White)\n");
			printf("Default image path, overridden by ROGUE_PIC env var: %s\n", PIC_PATH);
			return 0;
		}
		if (!strcmp("-d", arg) || !strcmp("--debug", arg)) {
			debug = true;
			continue;
		}
		if (!strcmp("--", arg)) {
			argc--;
			break;
		}
		if (arg[0] == '-')
			fatal("invalid option: %s", arg);

		if (path)
			fatal("too many arguments: %s", arg);
		path = arg;
	}
	if (!path) {
		if (argc) {
			path = *(++argv);
			argc--;
		}
		else
			if (!(path = getenv("ROGUE_PIC")))
				path = PIC_PATH;
	}
	if (argc)
		fatal("too many arguments: %s", *(++argv));

	assert(!argc && path);

	if (debug)
		fprintf(stderr, "%s\n", path);

	if (!show_sdl_splash(path))
		exit(EXIT_FAILURE);
}
