#include "../src/rogue.h"
#include "../src/screen.h"
#include <assert.h>

static char test_message_buffer[BUFSIZE];
static char test_description_buffer[MAXSTR];
static char test_combat_buffer[MAXSTR];
static char displayed[BUFSIZE];
static const char *input;

void __wrap_update_player_view(bool wakeup) { (void)wakeup; }
void __wrap_regenerate_health(void) {}
int __wrap_screen_move(int row, int column) { (void)row; (void)column; return 0; }
void __wrap_screen_clear_to_eol(void) {}
void __wrap_generate_level(void) {}
bool __wrap_player_saving_throw(int kind) { assert(kind == VS_LUCK); return TRUE; }

byte __wrap_read_game_key(void)
{
	assert(input && *input);
	return *input++;
}

void __wrap_screen_printf(const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(displayed, sizeof(displayed), format, arguments);
	va_end(arguments);
}

Entity *__wrap_select_inventory_item(char *purpose, int type)
{
	assert(strcmp(purpose, "identify") == 0 && type == 0);
	return player_inventory;
}

static void reset_display(void)
{
	message_column = 0;
	displayed[0] = '\0';
}

static void expect_display(const char *text)
{
	if (strcmp(displayed, text) != 0) {
		fprintf(stderr, "Message changed: expected [%s], got [%s]\n", text, displayed);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	static const char *texts[] = {
		"Percent %%", "Percent %s", "Percent %n", "Percent %d", "Percent %"
	};
	Entity food = {0};
	char expected[MAXSTR];

	COLS = 80;
	LINES = 25;
	message_buffer = test_message_buffer;
	description_buffer = test_description_buffer;
	combat_name_buffer = test_combat_buffer;
	remember_message = TRUE;

	for (size_t i = 0; i < sizeof(texts) / sizeof(*texts); i++) {
		reset_display();
		strcpy(previous_message, texts[i]);
		input = "\022.";  /* Repeat message, then consume a turn to leave the dispatcher. */
		execute_command();
		expect_display(texts[i]);
		assert(*input == '\0');
	}
	reset_display();
	previous_message[0] = '\0';
	input = "\022.";
	execute_command();
	expect_display("");
	assert(message_column == 0);

	food.item_category = FOOD;
	food.item_subtype = 1;
	food.item_quantity = 1;
	player_inventory = &food;
	for (size_t i = 0; i < sizeof(texts) / sizeof(*texts); i++) {
		reset_display();
		strcpy(favorite_fruit, texts[i]);
		snprintf(expected, sizeof(expected), "A %s", texts[i]);
		identify_item();
		expect_display(expected);
	}

	expert = TRUE;
	reset_display();
	report_hit("Percent %%", "target");
	expect_display("The Percent %% hit the target");
	reset_display();
	report_miss("Percent %%", "target");
	expect_display("The Percent %% misses the target");

	for (size_t i = 0; i < sizeof(texts) / sizeof(*texts); i++) {
		reset_display();
		fall_to_next_level((char *)texts[i]);
		expect_display(texts[i]);
	}
	reset_display();
	fall_to_next_level("");
	assert(message_column == 0);

	reset_display();
	show_message("Gold: %d", 42);
	expect_display("Gold: 42");
	show_message("");
	assert(message_column == 0);
	assert(strcmp(previous_message, "Gold: 42") == 0);
	puts("message text tests passed");
	return 0;
}
