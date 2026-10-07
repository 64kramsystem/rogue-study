#include "../src/options.c"
#include <assert.h>
#include <setjmp.h>

static jmp_buf error;
static bool expect_error;

void protection_tick(void) {}

void fatal(const char *format, ...)
{
	(void)format;
	assert(expect_error);
	fclose(options_file);
	longjmp(error, 1);
}

static void parse(const char *text)
{
	FILE *input = fopen("rogue.opt", "wb");
	assert(input);
	fputs(text, input);
	fclose(input);
	assert(load_options_file("rogue.opt"));
}

int main(void)
{
	assert(!load_options_file("missing.opt"));
	parse("# comment\r\nNAME = Alice\r\nfruit - slime   mold\r\nMENU = OFF\nSCREEN = BW FAST");
	assert(strcmp(player_name, "Alice") == 0);
	assert(strcmp(favorite_fruit, "slime mold") == 0);
	assert(strcmp(menu_option, "off") == 0);
	assert(strcmp(screen_option, "bw fast") == 0);

	parse("name=\nfruit=pear\nmacro=   \nmenu=on\n");
	assert(*player_name == 0 && *keyboard_macro == 0);
	assert(strcmp(favorite_fruit, "pear") == 0);
	assert(strcmp(menu_option, "on") == 0);
	parse("macro=");
	assert(*keyboard_macro == 0);
	parse("# comment without newline");
	parse("name=Bob\x1a");
	assert(strcmp(player_name, "Bob") == 0);

	/* Exercise the former end-of-buffer writes, including the exact boundaries. */
	for (size_t length = 1; length <= 4096; length++) {
		FILE *input = fopen("rogue.opt", "w");
		assert(input);
		for (size_t i = 0; i < length; i++)
			fputc('x', input);
		fputs("=ignored\nmacro=", input);
		for (size_t i = 0; i < length; i++)
			fputc('h', input);
		fputs("\nname=Alice\n", input);
		fclose(input);
		assert(load_options_file("rogue.opt"));
		assert(strlen(keyboard_macro) == (length < 40 ? length : 40));
		assert(strcmp(player_name, "Alice") == 0);
	}

	expect_error = TRUE;
	if (setjmp(error) == 0) {
		parse("name\nfruit=pear\n");
		assert(!"missing separator should fail");
	}
	if (setjmp(error) == 0) {
		parse("name");
		assert(!"incomplete label should fail");
	}
	puts("configuration tests passed");
	return 0;
}
