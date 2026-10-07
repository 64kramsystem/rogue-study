#include "../src/rip.c"
#include <assert.h>

int COLS = 80, LINES = 25, is_saved, scr_type;
char s_score[64] = "rogue.scr", whoami[] = "Tester";
static char print_buffer[1024];
char *prbuf = print_buffer;
static char output[8192];
static const char *input;

void cur_printw(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(output + strlen(output), sizeof(output) - strlen(output), fmt, args);
	va_end(args);
}

void cur_addstr(char *s) { cur_printw("%s", s); }
void cur_mvaddstr(int r, int c, char *s) { (void)r; (void)c; cur_addstr(s); }
int cur_move(int r, int c) { (void)r; (void)c; return 0; }
void cur_clear(void) { output[0] = 0; }
void set_attr(int attr) { (void)attr; }
void str_attr(char *s) { cur_addstr(s); }
void wait_msg(const char *s) { (void)s; }
byte readchar(void) { assert(input && *input); return *input++; }

int main(void)
{
	struct sc_ent restored[TOPSCORES];

	/* A missing parent directory reliably makes creation fail, even as root. */
	strcpy(s_score, "missing/rogue.scr");
	input = "ca";
	score(100, 1, 0);
	assert(*input == 0);

	strcpy(s_score, "rogue.scr");
	input = "c";
	pstats.s_lvl = 2;
	max_level = 1;
	score(100, 1, 0);
	assert(strstr(output, "Tester"));
	file = fopen(s_score, "r");
	assert(file);
	get_scores(restored);
	fclose(file);
	assert(restored[0].sc_gold == 100);
	assert(restored[0].sc_rank == 2);
	puts("score tests passed");
	return 0;
}
