#include "../src/rip.c"
#include <assert.h>

int COLS = 80, LINES = 25, screen_updates_suspended, dos_screen_mode;
char score_filename[64] = "rogue.scr", player_name[] = "Tester";
static char print_buffer[1024];
char *description_buffer = print_buffer;
static char output[8192];
static const char *input;

void screen_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vsnprintf(output + strlen(output), sizeof(output) - strlen(output), format, args);
	va_end(args);
}

void screen_write_text(char *s) { screen_printf("%s", s); }
void screen_write_text_at(int row, int column, char *s) { (void)row; (void)column; screen_write_text(s); }
int screen_move(int row, int column) { (void)row; (void)column; return 0; }
void screen_clear(void) { output[0] = 0; }
void set_display_attribute(int attr) { (void)attr; }
void print_highlighted_text(char *s) { screen_write_text(s); }
void wait_for_enter(const char *s) { (void)s; }
byte read_game_key(void) { assert(input && *input); return *input++; }

int main(void)
{
	struct score_entry restored[MAX_HIGH_SCORES];

	/* A missing parent directory reliably makes creation fail, even as root. */
	strcpy(score_filename, "missing/rogue.scr");
	input = "ca";
	update_high_scores(100, 1, 0);
	assert(*input == 0);

	strcpy(score_filename, "rogue.scr");
	input = "c";
	player_stats.experience_level = 2;
	deepest_level = 1;
	update_high_scores(100, 1, 0);
	assert(strstr(output, "Tester"));
	score_file = fopen(score_filename, "r");
	assert(score_file);
	read_scores(restored);
	fclose(score_file);
	assert(restored[0].gold == 100);
	assert(restored[0].experience_level == 2);
	puts("score tests passed");
	return 0;
}
