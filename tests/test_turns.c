#include "../src/rogue.h"
#include "../src/screen.h"
#include <assert.h>

static int expected_actions, status_updates, command_reads;
static int haste_roll, teleport_roll;
static bool expire_haste;
static char events[64];
static size_t event_count;
static Entity test_rings[2];

static void record_event(char event)
{
	assert(event_count + 1 < sizeof(events));
	events[event_count++] = event;
	events[event_count] = '\0';
}

void __wrap_update_status_line(void)
{
	/* Fail promptly on the old infinite loop, without a test timeout. */
	if (++status_updates > expected_actions) {
		fputs("process_turn exceeded its action budget\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void __wrap_update_player_view(bool wakeup) { (void)wakeup; }
byte __wrap_read_game_key(void) { command_reads++; return '.'; }
void __wrap_regenerate_health(void) { record_event('C'); }
void __wrap_screen_refresh(void) { record_event('R'); }
void __wrap_run_recurring_actions(void) { record_event('D'); }
void __wrap_search(void) { record_event('S'); }
int __wrap_teleport(void) { record_event('T'); return 0; }

void __wrap_run_delayed_actions(void)
{
	record_event('F');
	if (expire_haste)
		player.actor_flags &= ~ACTOR_HASTED;
}

void __wrap_show_message(const char *format, ...)
{
	assert(strcmp(format, "you can move again") == 0);
	record_event('M');
}

int __wrap_random_below(int range)
{
	if (range == 2)
		return haste_roll;
	assert(range == 50);
	return teleport_roll;
}

static void reset_turn(int actions)
{
	expected_actions = actions;
	status_updates = command_reads = 0;
	haste_roll = teleport_roll = 0;
	expire_haste = FALSE;
	event_count = 0;
	events[0] = '\0';
	player.actor_flags = 0;
	incapacitated_turns = 0;
	equipped_rings[LEFT] = equipped_rings[RIGHT] = NULL;
}

static void equip_ring(int hand, int subtype)
{
	test_rings[hand].item_category = RING;
	test_rings[hand].item_subtype = subtype;
	equipped_rings[hand] = &test_rings[hand];
}

static void check_turn(const char *expected_events, int expected_commands)
{
	process_turn();
	assert(status_updates == expected_actions);
	assert(command_reads == expected_commands);
	assert(strcmp(events, expected_events) == 0);
}

int main(void)
{
	/* Event order: command, fuse callbacks, daemon callbacks, ring effects. */
	reset_turn(1);
	check_turn("CFD", 1);

	reset_turn(2);
	player.actor_flags = ACTOR_HASTED;
	check_turn("CFDCFD", 2);

	reset_turn(3);
	player.actor_flags = ACTOR_HASTED;
	haste_roll = 1;
	check_turn("CFDCFDCFD", 3);

	for (int hand = LEFT; hand <= RIGHT; hand++) {
		reset_turn(1);
		equip_ring(hand, RING_SEARCHING);
		check_turn("CFDS", 1);
	}
	reset_turn(1);
	equip_ring(LEFT, RING_SEARCHING);
	equip_ring(RIGHT, RING_SEARCHING);
	check_turn("CFDSS", 1);

	reset_turn(2);
	player.actor_flags = ACTOR_HASTED;
	equip_ring(LEFT, RING_SEARCHING);
	equip_ring(RIGHT, RING_TELEPORTATION);
	teleport_roll = 17;
	check_turn("CFDSTCFDST", 2);

	reset_turn(1);
	equip_ring(LEFT, RING_TELEPORTATION);
	equip_ring(RIGHT, RING_SEARCHING);
	teleport_roll = 17;
	check_turn("CFDTS", 1);

	reset_turn(1);
	equip_ring(LEFT, RING_TELEPORTATION);
	check_turn("CFD", 1);

	/* Incapacitation skips input but still advances callbacks and ring effects. */
	reset_turn(1);
	incapacitated_turns = 2;
	equip_ring(RIGHT, RING_SEARCHING);
	check_turn("RFDS", 0);
	assert(incapacitated_turns == 1);

	reset_turn(2);
	player.actor_flags = ACTOR_HASTED;
	incapacitated_turns = 1;
	check_turn("MRFDCFD", 1);
	assert(incapacitated_turns == 0);

	/* Haste expiring during callbacks changes the budget of the following call. */
	reset_turn(2);
	player.actor_flags = ACTOR_HASTED;
	expire_haste = TRUE;
	check_turn("CFDCFD", 2);
	assert(!has_actor_flag(player, ACTOR_HASTED));
	expected_actions++;
	check_turn("CFDCFDCFD", 3);

	puts("turn processing tests passed");
	return 0;
}
