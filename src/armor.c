/*
 * This file contains misc functions for dealing with armor
 * @(#)armor.c		1.2 (AI Design)		2/12/84
 *
 */

#include "rogue.h"
#include "screen.h"

/*
 * wear_armor:
 *	The player wants to wear something, so let him/her put it on.
 */
void
wear_armor()
{
	register Entity *item;
	register char *text_cursor;

	if (equipped_armor != NULL) {
		show_message("you are already wearing some%s.",
			verbose_text(".  You'll have to take it off first"));
		turn_consumed = FALSE;
		return;
	}
	if ((item = select_inventory_item("wear",ARMOR)) == NULL)
		return;
	if (item->item_category != ARMOR) {
		show_message("you can't wear that");
		return;
	}
	advance_turn();
	item->item_flags |= ITEM_IDENTIFIED ;
	text_cursor = describe_item(item, TRUE);
	equipped_armor = item;
	show_message("you are now wearing %s", text_cursor);
}

/*
 * remove_armor:
 *	Get the armor off of the player's back
 */
void
remove_armor()
{
	register Entity *item;

	if ((item = equipped_armor) == NULL) {
		turn_consumed = FALSE;
		show_message("you aren't wearing any armor");
		return;
	}
	if (!can_drop(equipped_armor))
		return;
	equipped_armor = NULL;
	show_message("you used to be wearing %c) %s", inventory_key(item), describe_item(item, TRUE));
}

/*
 * advance_turn:
 *	Do nothing but let other things happen
 */
void
advance_turn()
{
	run_recurring_actions();
	run_delayed_actions();
}
