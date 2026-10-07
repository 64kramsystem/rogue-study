#include "rogue.h"
#include "curses.h"

/*
 * Routines to deal with the pack
 *
 * pack.c	1.4 (A.I. Design)	12/14/84
 */

static
Entity *
inventory_item_for_key(byte inventory_key, byte *final_key)
{
	register Entity *item;
	register byte previous_key;

	for (item = player_inventory, previous_key = 'a'; item != NULL; item = next(item), previous_key++)
		if (inventory_key == previous_key)
			return item;
	*final_key = previous_key;
	return NULL;
}

/*
 * add_pack:
 *	Pick up an object and add it to the pack.  If the argument is
 *	non-null use it as the linked_list pointer instead of gettting
 *	it off the ground.
 */
void
add_to_inventory(Entity *item, bool silent)
{
	register Entity *other_item, *previous_item = NULL;
	register bool exact, from_floor;
	register byte floor_symbol;

	if (item == NULL)
	{
		from_floor = TRUE;
		if ((item = item_at(player_position.y, player_position.x)) == NULL)
			return;
	}
	else
		from_floor = FALSE;
	/*
	 * Link it into the pack.  Search the pack for a object of similar type
	 * if there isn't one, stuff it at the beginning, if there is, look for one
	 * that is exactly the same and just increment the count if there is.
	 * Food is always put at the beginning for ease of access, but it
	 * is not ordered so that you can't tell good food from bad.  First check
	 * to see if there is something in the same group and if there is then
	 * increment the count.
	 */

	/*@
	 *  bug in original Rogue: it didn't check proom != NULL, as is the case
	 *  when add_pack() is called from init_player(), which happens before
	 *  any room even exist. proom is set in enter_room(), which is first
	 *  called in new_level()
	 */
	floor_symbol = (player_room != NULL && (player_room->flags & ROOM_ABSENT)) ? PASSAGE : FLOOR;
	if (item->item_stack_group)
	{
		for (other_item = player_inventory; other_item != NULL; other_item = next(other_item))
		{
			if (other_item->item_stack_group == item->item_stack_group)
			{
			/*
			 * Put it in the pack and notify the user
			 */
				other_item->item_quantity += item->item_quantity;
				if (from_floor)
				{
					detach(level_items, item);
					mvaddch(player_position.y, player_position.x, floor_symbol);
					terrain_at(player_position.y, player_position.x) = floor_symbol;
				}
				release_entity(item);
				item = other_item;
				goto picked_up;
			}
		}
	}
	/*
	 * Check if there is room
	 */
	if (inventory_count >= MAXPACK-1)
	{
		show_message("you can't carry anything else");
		return;
	}
	/*
	 * Check for and deal with scare monster scrolls
	 */
	if (item->item_category == SCROLL && item->item_subtype == SCROLL_SCARE_MONSTER)
	{
		if (item->item_flags & ACTOR_FOUND)
		{
			detach(level_items, item);
			mvaddch(player_position.y, player_position.x, floor_symbol);
			terrain_at(player_position.y, player_position.x) = floor_symbol;
			show_message("the scroll turns to dust%s.", verbose_text(" as you pick it up"));
			return;
		}
		else
			item->item_flags |= ACTOR_FOUND;
	}

	inventory_count++;
	if (from_floor)
	{
		detach(level_items, item);
		mvaddch(player_position.y, player_position.x, floor_symbol);
		terrain_at(player_position.y, player_position.x) = floor_symbol;
	}
	/*
	 * Search for an object of the same type
	 */
	exact = FALSE;
	for (other_item = player_inventory; other_item != NULL; other_item = next(other_item))
		if (item->item_category == other_item->item_category)
			break;
	if (other_item == NULL)
	{
		/*
		 * Put it at the end of the pack since it is a new type
		 */
		for (other_item = player_inventory; other_item != NULL; other_item = next(other_item))
		{
			if (other_item->item_category != FOOD)
				break;
			previous_item = other_item;
		}
	}
	else
	{
		/*
		 * Search for an object which is exactly the same
		 */
		while (other_item->item_category == item->item_category)
		{
			if (other_item->item_subtype == item->item_subtype)
			{
				exact = TRUE;
				break;
			}
			previous_item = other_item;
			if ((other_item = next(other_item)) == NULL)
				break;
		}
	}
	if (other_item == NULL)
	{
		/*
		 * Didn't find an exact match, just stick it here
		 */
		if (player_inventory == NULL)
			player_inventory = item;
		else
		{
			previous_item->next_entity = item;
			item->previous_entity = previous_item;
			item->next_entity = NULL;
		}
	}
	else
	{
		/*
		 * If we found an exact match.  If it is a potion, food, or a
		 * scroll, increase the count, otherwise put it with its clones.
		 */
		if (exact && is_stackable_category(item->item_category))
		{
			other_item->item_quantity++;
			release_entity(item);
			item = other_item;
			goto picked_up;
		}
		if ((item->previous_entity = prev(other_item)) != NULL)
		{
			item->previous_entity->next_entity = item;
		}
		else
		{
			player_inventory = item;
		}
		item->next_entity = other_item;
		other_item->previous_entity = item;
	}
picked_up:
	/*
	 * If this was the object of something's desire, that monster will
	 * get mad and run at the hero
	 */
	for (other_item = level_monsters; other_item != NULL; other_item = next(other_item))
	{
		/*
		 *  compiler bug: jll : 2-7-83
		 *		It is stupid because it thinks the obj... is not an lvalue
		 *		this may be true since there is no structure assignments,
		 *		but still it should let you have the address??!!
		 *
		if (&obj->_o._o_pos == op->t_dest)
		 *
		 *  the following should do the same
		 */
		/*@
		 * Another bug in Rogue: missed NULL check for t_dest. Monsters could
		 * be not chasing (sleeping, another room, Ice Monster, etc), so a
		 * destination could possibly have never been assigned.
		 */
		if (other_item->actor_destination != NULL &&
		   (other_item->actor_destination->x == item->item_position.x) && (other_item->actor_destination->y == item->item_position.y))
			other_item->actor_destination = &player_position;
	}

	if (item->item_category == AMULET)
	{
		carrying_amulet = TRUE;
		saw_amulet = TRUE;
	}
	/*
	 * Notify the user
	 */
	if (!silent)
		show_message("%s%s (%c)",verbose_text("you now have "),
			describe_item(item, TRUE), inventory_key(item));
}

/*
 * inventory:
 *	List what is in the pack
 */
byte
show_inventory(Entity *items, int type, char *line_prefix)
{
	register byte character;
	register int displayed_count;
	char line_format[MAXSTR];

	displayed_count = 0;
	for (character = 'a'; items != NULL; character++, items = next(items))
	{
		/*
		 * Don't print this one if:
		 *	the type doesn't match the type we were passed AND
		 *	it isn't a callable type AND
		 *	it isn't a zappable weapon
		 */
		if (type && type != items->item_category && !(type == CALLABLE &&
		  (items->item_category == SCROLL || items->item_category == POTION ||
		  items->item_category == RING || items->item_category == STICK)) &&
		  !(type == WEAPON && items->item_category == POTION) &&
		  !(type == STICK && items->item_slays_species && items->item_charges))
			continue;
		displayed_count++;
		sprintf(line_format, "%c) %%s", character);
		add_line(line_prefix, line_format, describe_item(items, FALSE));
	}
	if (displayed_count == 0)
	{
		show_message(type == 0 ? "you are empty handed" :
					"you don't have anything appropriate");
		return 0;
	}
	return(end_line(line_prefix));
}

/*
 * pick_up:
 *	Add something to characters pack.
 */
void
pick_up_item(byte character)
{
	register Entity *item;

	switch (character)
	{
	case GOLD:
		if ((item = item_at(player_position.y, player_position.x)) == NULL)
		return;
		collect_gold(item->item_gold_amount);
		detach(level_items, item);
		release_entity(item);
		player_room->gold_amount = 0;
		break;
	default:
	case ARMOR:
	case POTION:
	case FOOD:
	case WEAPON:
	case SCROLL:
	case AMULET:
	case RING:
	case STICK:
		add_to_inventory(NULL, FALSE);
		break;
	}
}

/*
 * get_item:
 *	Pick something out of a pack for a purpose
 */
Entity *
select_inventory_item(char *purpose, int type)
{
	register Entity *item;
	register byte character;
	byte command_key;
	static byte last_inventory_key;
	static Entity *previous_item = NULL;
	byte selection_state;	/* get item sub state */
	int show_help_once = FALSE;

	if (((!strncmp(menu_option,"sel",3) && strcmp(purpose,"eat")
	  && strcmp(purpose,"drop"))) || !strcmp(menu_option,"on"))
		show_help_once = TRUE;

	selection_state = repeating_command;
	if (player_inventory == NULL)
		show_message("you aren't carrying anything");
	else {
		character = last_inventory_key;
		for (;;) {
			/*
			 * if we are doing something AGAIN, and the pack hasn't
			 * changed then don't ask just give him the same thing
			 * he got on the last command.
			 */
			if (selection_state && previous_item == inventory_item_for_key(character, &command_key))
				goto skip;
			if (show_help_once) {
				character = '*';
				goto skip;
			}
			if (!terse && !expert)
				append_message("which object do you want to ");
			show_message("%s? (* for list): ",purpose);
			/*
			 * ignore any alt characters that may be typed
			 */
			character = read_game_key();
			skip:
			message_column = 0;
			selection_state = FALSE;
			show_help_once = FALSE;
			if (character == '*') {
				if ((character = show_inventory(player_inventory, type, purpose)) == 0) {
					turn_consumed = FALSE;
					return NULL;
				}
				if (character == ' ')
					continue;
				last_inventory_key = character;
			}
			/*
			 * Give the poor player a chance to abort the command
			 */
			if (character == ESCAPE) {
				turn_consumed = FALSE;
				show_message("");
				return NULL;
			}
			if ((item = inventory_item_for_key(character, &command_key)) == NULL) {
				message_by_verbosity1("range is 'a' to '%c'","please specify a letter between 'a' and '%c'", command_key-1);
				continue;
			} else {
				/*
				 * If you find an object reset flag because
				 * you really don't know if the object he is getting
				 * is going to change the pack.  If he detaches the
				 * thing from the pack later this flag will get set.
				 */
				if (strcmp(purpose, "identify")) {
					last_inventory_key = character;
					previous_item = item;
				}
				return item;
		   }
		}
	}
	return NULL;
}

/*
 * pack_char:
 *	Return which character would address a pack object
 */
byte
inventory_key(Entity *target_item)
{
	register Entity *item;
	register byte item_key;

	item_key = 'a';
	for (item = player_inventory; item != NULL; item = next(item))
		if (item == target_item)
			return item_key;
		else
			item_key++;
	return '?';
}

/*
 * money:
 *	Add or subtract gold from the pack
 */
void
collect_gold(int value)
{
	register byte floor;

	floor = (player_room->flags & ROOM_ABSENT) ? PASSAGE : FLOOR;
	player_gold += value;
	mvaddch(player_position.y, player_position.x, floor);
	terrain_at(player_position.y, player_position.x) = floor;
	if (value > 0)
	{
		show_message("you found %d gold pieces", value);
	}
}
