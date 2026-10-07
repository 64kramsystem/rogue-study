#include "../src/rip.c"
#include <assert.h>
#include <limits.h>

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

static void check_record(struct sc_ent entry, bool valid, size_t bytes)
{
	struct sc_ent scores[TOPSCORES];
	file = tmpfile();
	assert(file);
	assert(fwrite(&entry, 1, bytes, file) == bytes);
	rewind(file);
	get_scores(scores);
	fclose(file);
	assert(scores[0].sc_gold == (valid ? entry.sc_gold : 0));
	for (int i = 1; i < TOPSCORES; i++)
		assert(scores[i].sc_gold == 0);
	pr_scores(0, scores);
}

int main(void)
{
	struct sc_ent entry = {"Tester", 2, 100, 'A', 1};
	struct sc_ent scores[TOPSCORES] = {0}, restored[TOPSCORES];
	check_record(entry, TRUE, sizeof(entry));
	for (size_t n = 0; n < sizeof(entry); n++)
		check_record(entry, FALSE, n);
	int bad_ranks[] = {INT_MIN, -1, 0, (int)he_man_count + 1, INT_MAX};
	for (size_t i = 0; i < sizeof(bad_ranks) / sizeof(*bad_ranks); i++) {
		entry.sc_rank = bad_ranks[i];
		check_record(entry, FALSE, sizeof(entry));
	}
	entry.sc_rank = (int)he_man_count;
	check_record(entry, TRUE, sizeof(entry));
	entry.sc_level = 0;
	check_record(entry, FALSE, sizeof(entry));
	entry.sc_level = 1;
	entry.sc_gold = -1;
	check_record(entry, FALSE, sizeof(entry));
	entry.sc_gold = 100;
	memset(entry.sc_name, 'X', sizeof(entry.sc_name));
	check_record(entry, FALSE, sizeof(entry));
	strcpy(entry.sc_name, "Tester");

	for (int gold = 10; gold <= 110; gold += 10) {
		entry.sc_gold = gold;
		assert(add_scores(&entry, scores) == 1);
	}
	for (int i = 0; i < TOPSCORES; i++)
		assert(scores[i].sc_gold == 110 - i * 10);
	entry.sc_gold = 1;
	assert(add_scores(&entry, scores) == 0);
	entry.sc_gold = 100;
	assert(add_scores(&entry, scores) == 3);
	file = tmpfile();
	assert(file && put_scores(scores));
	rewind(file);
	get_scores(restored);
	fclose(file);
	assert(memcmp(scores, restored, sizeof(scores)) == 0);

	/* A missing parent directory reliably makes creation fail, even as root. */
	strcpy(s_score, "missing/rogue.scr");
	input = "ca";
	score(100, 1, 0);
	assert(strstr(output, "Could not create scorefile"));
	assert(*input == 0);

	strcpy(s_score, "rogue.scr");
	input = "c";
	pstats.s_lvl = 2;
	max_level = 1;
	score(100, 1, 0);
	assert(strstr(output, "Tester"));
	file = fopen(s_score, "rb");
	assert(file);
	get_scores(restored);
	fclose(file);
	assert(restored[0].sc_gold == 100);
	assert(restored[0].sc_rank == 2);
#ifdef __linux__
	strcpy(s_score, "full.scr");
	assert(symlink("/dev/full", s_score) == 0);
	score(100, 1, 0);
	assert(strstr(output, "Could not write scorefile"));
#endif
	puts("score tests passed");
	return 0;
}
