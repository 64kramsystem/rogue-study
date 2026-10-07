#include "../src/rip.c"
#include <assert.h>
#include <limits.h>

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

static void check_record(struct score_entry entry, bool valid, size_t bytes)
{
	struct score_entry scores[MAX_HIGH_SCORES];
	score_file = tmpfile();
	assert(score_file);
	assert(fwrite(&entry, 1, bytes, score_file) == bytes);
	rewind(score_file);
	read_scores(scores);
	fclose(score_file);
	assert(scores[0].gold == (valid ? entry.gold : 0));
	for (int i = 1; i < MAX_HIGH_SCORES; i++)
		assert(scores[i].gold == 0);
	display_scores(0, scores);
}

int main(void)
{
	struct score_entry entry = {"Tester", 2, 100, 'A', 1};
	struct score_entry scores[MAX_HIGH_SCORES] = {0}, restored[MAX_HIGH_SCORES];
	check_record(entry, TRUE, sizeof(entry));
	for (size_t n = 0; n < sizeof(entry); n++)
		check_record(entry, FALSE, n);
	int bad_ranks[] = {INT_MIN, -1, 0, (int)rank_name_count + 1, INT_MAX};
	for (size_t i = 0; i < sizeof(bad_ranks) / sizeof(*bad_ranks); i++) {
		entry.experience_level = bad_ranks[i];
		check_record(entry, FALSE, sizeof(entry));
	}
	entry.experience_level = (int)rank_name_count;
	check_record(entry, TRUE, sizeof(entry));
	entry.dungeon_level = 0;
	check_record(entry, FALSE, sizeof(entry));
	entry.dungeon_level = 1;
	entry.gold = -1;
	check_record(entry, FALSE, sizeof(entry));
	entry.gold = 100;
	memset(entry.player_name, 'X', sizeof(entry.player_name));
	check_record(entry, FALSE, sizeof(entry));
	strcpy(entry.player_name, "Tester");

	for (int gold = 10; gold <= 110; gold += 10) {
		entry.gold = gold;
		assert(insert_score(&entry, scores) == 1);
	}
	for (int i = 0; i < MAX_HIGH_SCORES; i++)
		assert(scores[i].gold == 110 - i * 10);
	entry.gold = 1;
	assert(insert_score(&entry, scores) == 0);
	entry.gold = 100;
	assert(insert_score(&entry, scores) == 3);
	score_file = tmpfile();
	assert(score_file && write_scores(scores));
	rewind(score_file);
	read_scores(restored);
	fclose(score_file);
	assert(memcmp(scores, restored, sizeof(scores)) == 0);

	/* A missing parent directory reliably makes creation fail, even as root. */
	strcpy(score_filename, "missing/rogue.scr");
	input = "ca";
	update_high_scores(100, 1, 0);
	assert(strstr(output, "Could not create scorefile"));
	assert(*input == 0);

	strcpy(score_filename, "rogue.scr");
	input = "c";
	player_stats.experience_level = 2;
	deepest_level = 1;
	update_high_scores(100, 1, 0);
	assert(strstr(output, "Tester"));
	score_file = fopen(score_filename, "rb");
	assert(score_file);
	read_scores(restored);
	fclose(score_file);
	assert(restored[0].gold == 100);
	assert(restored[0].experience_level == 2);
#ifdef __linux__
	strcpy(score_filename, "full.scr");
	assert(symlink("/dev/full", score_filename) == 0);
	update_high_scores(100, 1, 0);
	assert(strstr(output, "Could not write scorefile"));
#endif
	puts("score tests passed");
	return 0;
}
