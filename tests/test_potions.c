#include "../src/rogue.h"
#include "../src/screen.h"
#include <assert.h>

static Entity potion;
static char potion_label[MAXNAME + 1];
static bool maximum_roll;
static int confusion_expirations;

Entity *__wrap_select_inventory_item(char *purpose, int type)
{
	assert(strcmp(purpose, "quaff") == 0 && type == POTION);
	return &potion;
}

void __wrap_update_status_line(void) {}

void __wrap_show_message(const char *format, ...)
{
	if (strcmp(format, "you feel less confused now") == 0)
		confusion_expirations++;
}

int __wrap_random_below(int range)
{
	assert(range > 0);
	return maximum_roll ? range - 1 : 0;
}

static void prepare_potion(void)
{
	memset(&potion, 0, sizeof(potion));
	potion.item_category = POTION;
	potion.item_subtype = POTION_CONFUSION;
	potion.item_quantity = 2;
	player_inventory = &potion;
	inventory_count = 2;
	player.actor_flags = 0;
	potion_labels[POTION_CONFUSION] = potion_label;
	confusion_expirations = 0;
}

static void wait_for_confusion_to_end(void)
{
	for (int turn = 0; turn < 40 && has_actor_flag(player, ACTOR_CONFUSED); turn++)
		run_delayed_actions();
	assert(!has_actor_flag(player, ACTOR_CONFUSED));
	assert(confusion_expirations == 1);
	for (int turn = 0; turn < 40; turn++)
		run_delayed_actions();
	assert(confusion_expirations == 1);
}

int main(void)
{
	prepare_potion();
	drink_potion();
	assert(has_actor_flag(player, ACTOR_CONFUSED));
	assert(potion.item_quantity == 1 && inventory_count == 1);
	wait_for_confusion_to_end();

	for (int roll = 0; roll <= 1; roll++) {
		prepare_potion();
		maximum_roll = roll;
		player.actor_flags = ACTOR_CONFUSED;
		schedule_delayed_action(end_confusion, 2);
		drink_potion();
		assert(potion.item_quantity == 1 && inventory_count == 1);
		run_delayed_actions();
		run_delayed_actions();
		if (!has_actor_flag(player, ACTOR_CONFUSED)) {
			fputs("A second confusion potion did not extend the existing effect\n", stderr);
			return EXIT_FAILURE;
		}
		assert(confusion_expirations == 0);
		wait_for_confusion_to_end();
	}
	puts("potion duration tests passed");
	return 0;
}
