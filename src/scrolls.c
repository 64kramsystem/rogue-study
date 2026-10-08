/*
 * Read a scroll and let it happen
 *
 * scrolls.c	1.4 (AI Design)	12/14/84
 */

#include "rogue.h"
#include "screen.h"

char *laugh = "you hear maniacal laughter%s.";
char *in_dist = " in the distance";
/*
 * read_scroll:
 *	Read a scroll from the pack and do the appropriate thing
 */
void
read_scroll(void)
{
	register Entity *item;
	register int y, x;
	register byte character;
	register Entity *affected_item;
	register int index;
	register bool consume_scroll = FALSE;

	item = select_inventory_item("read", SCROLL);
	if (item == NULL)
		return;
	if (item->item_category != SCROLL){
		show_message("there is nothing on it to read");
		return;
	}
	message_by_verbosity0("the scroll vanishes","as you read the scroll, it vanishes");
	/*
	 * Calculate the effect it has on the poor guy.
	 */
	if (item == equipped_weapon)
		equipped_weapon = NULL;
	switch (item->item_subtype){
	case SCROLL_CONFUSE_MONSTER:
		/*
		 * Scroll of monster confusion.  Give him that power.
		 */
		player.actor_flags |= ACTOR_CAN_CONFUSE;
		show_message("your hands begin to glow red");
		break;
	case SCROLL_ENCHANT_ARMOR:
		if (equipped_armor != NULL) {
			equipped_armor->item_modifier--;
			equipped_armor->item_flags &= ~ITEM_CURSED;
			message_by_verbosity0("your armor glows faintly",
				"your armor glows faintly for a moment");
		}
		break;
	case SCROLL_HOLD_MONSTERS:
		/*
		 * Hold monster scroll.  Stop all monsters within two spaces
		 * from chasing after the hero.
		 */

		for (x = player_position.x - 3; x <= player_position.x + 3; x++)
			if (x >= 0 && x < COLS)
				for (y = player_position.y - 3; y <= player_position.y + 3; y++)
					if ((y > 0 && y < dungeon_bottom_row) && ((affected_item=monster_at(y, x)) != NULL)) {
						affected_item->actor_flags &= ~ACTOR_CHASING;
						affected_item->actor_flags |= ACTOR_HELD;
					}
		break;
	case SCROLL_SLEEP:
		/*
		 * Scroll which makes you fall asleep
		 */
		scroll_identified[SCROLL_SLEEP] = TRUE;
		incapacitated_turns += random_below(SLEEPTIME) + 4;
		player.actor_flags &= ~ACTOR_CHASING;
		show_message("you fall asleep");
		break;
	case SCROLL_CREATE_MONSTER: {
		Position monster_position;

		if (find_monster_spawn_position(player_position.y, player_position.x, &monster_position) && (affected_item=allocate_entity()) != NULL)
			new_monster(affected_item, random_monster_species(FALSE), &monster_position);
		else
			message_by_verbosity0("you hear a faint cry of anguish",
				"you hear a faint cry of anguish in the distance");
	} break;
	case SCROLL_IDENTIFY:
		/*
		 * Identify, let the rogue figure something out
		 */
		scroll_identified[SCROLL_IDENTIFY] = TRUE;
		show_message("this scroll is an identify scroll");
		if (! strcmp(menu_option,"on") || !strcmp(menu_option,"sel"))
			show_more_prompt(" More ");
		identify_item();
		break;
	case SCROLL_MAPPING:
		/*
		 * Scroll of magic mapping.
		 */
		scroll_identified[SCROLL_MAPPING] = TRUE;
		show_message("oh, now this scroll has a map on it");
		/*
		 * Take all the things we want to keep hidden out of the window
		 */
		for (y = 1; y < dungeon_bottom_row; y++)
			for (x = 0; x < COLS; x++) {
				index = map_index(y, x);
				switch (character = terrain_map[index])
				{
				case VWALL:
				case HWALL:
				case ULWALL:
				case URWALL:
				case LLWALL:
				case LRWALL:
					if (!(cell_flags[index] & CELL_REVEALED)) {
						character = terrain_map[index] = DOOR;
						cell_flags[index] &= ~CELL_REVEALED;
					}
					/* fallthrough */
				case DOOR:
				case PASSAGE:
				case STAIRS:
					if ((affected_item = monster_at(y, x)) != NULL)
						if (affected_item->actor_previous_tile == ' ')
							affected_item->actor_previous_tile = character;
					break;
				default:
					character = ' ';
				}
				if (character == DOOR) {
					move(y,x);
					if (inch() != DOOR)
						standout();
				}
				if (character != ' ')
					mvaddch(y, x, character);
				standend();
			}
		break;
	case SCROLL_FOOD_DETECTION:
		/*
					 * Scroll of food detection
					 */
		character = FALSE;
		for (affected_item = level_items; affected_item != NULL; affected_item = next(affected_item)) {
			if (affected_item->item_category == FOOD) {
				character = TRUE;
				standout();
				mvwaddch(hw, affected_item->item_position.y, affected_item->item_position.x, FOOD);
				standend();
			} else /* as a bonus this will detect amulets as well */
			if (affected_item->item_category == AMULET) {
				character = TRUE;
				standout();
				mvwaddch(hw, affected_item->item_position.y, affected_item->item_position.x, AMULET);
				standend();
			}
		}
		if (character) {
			scroll_identified[SCROLL_FOOD_DETECTION] = TRUE;
			show_message("your nose tingles as you sense food");
		} else
			message_by_verbosity0("you hear a growling noise close by","you hear a growling noise very close to you");
		break;
	case SCROLL_TELEPORT:
		/*
		 * Scroll of teleportation:
		 * Make him dissapear and reappear
		 */
		{
		register struct room *cur_room;

		cur_room = player_room;
		teleport();
		if (cur_room != player_room)
			scroll_identified[SCROLL_TELEPORT] = TRUE;
		}
		break;
	case SCROLL_ENCHANT_WEAPON:
		if (equipped_weapon == NULL || equipped_weapon->item_category != WEAPON)
		show_message("you feel a strange sense of loss");
		else
		{
		equipped_weapon->item_flags &= ~ITEM_CURSED;
		if (random_below(2) == 0)
			equipped_weapon->item_hit_bonus++;
		else
			equipped_weapon->item_damage_bonus++;
		message_by_verbosity1("your %s glows blue","your %s glows blue for a moment", weapon_names[equipped_weapon->item_subtype]);
		}
		break;
	case SCROLL_SCARE_MONSTER:
		/*
		 * Reading it is a mistake and produces laughter at the
		 * poor rogue's boo boo.
		 */
			show_message(laugh, terse || expert ? "" : in_dist);
		    break;
	case SCROLL_REMOVE_CURSE:
		if (equipped_armor != NULL)
			equipped_armor->item_flags &= ~ITEM_CURSED;
		if (equipped_weapon != NULL)
			equipped_weapon->item_flags &= ~ITEM_CURSED;
		if (equipped_rings[LEFT] != NULL)
			equipped_rings[LEFT]->item_flags &= ~ITEM_CURSED;
		if (equipped_rings[RIGHT] != NULL)
			equipped_rings[RIGHT]->item_flags &= ~ITEM_CURSED;
		message_by_verbosity0("somebody is watching over you","you feel as if somebody is watching over you");
		break;
	case SCROLL_AGGRAVATE_MONSTERS:
		/*
		 * This scroll aggravates all the monsters on the current
		 * level and sets them running towards the hero
		 */
		aggravate_monsters();
		message_by_verbosity("you hear a humming noise",
					"you hear a high pitched humming noise");
		break;
	case SCROLL_BLANK:
		show_message("this scroll seems to be blank");
		break;
	case SCROLL_VORPALIZE:
		/*
		 * Extra Vorpal Enchant Weapon
		 *     Give weapon +1,+1
		 *     Is extremely vorpal against one certain type of monster
		 *     Against this type (item_slays_species) the weapon gets:
		 *		+4,+4
		 *		The ability to zap one such monster into oblivion
		 *
		 *     Some of these are cursed and if the rogue misses her saving
		 *     throw she will be forced to attack monsters of this type
		 *     whenever she sees one (not yet implemented)
		 *
		 * If he doesn't have a weapon I get to chortle again!
		 */
		if (equipped_weapon == NULL || equipped_weapon->item_category != WEAPON)
			show_message(laugh, terse || expert ? "" : in_dist);
		else {
			/*
			 * You aren't allowed to doubly vorpalize a weapon.
			 */
			if (equipped_weapon->item_slays_species != 0) {
				show_message("your %s vanishes in a puff of smoke",
				weapon_names[equipped_weapon->item_subtype]);
				detach(player_inventory, equipped_weapon);
				release_entity(equipped_weapon);
				equipped_weapon = NULL;
			} else {
				equipped_weapon->item_slays_species = random_vorpal_enemy();
				equipped_weapon->item_hit_bonus++;
				equipped_weapon->item_damage_bonus++;
				equipped_weapon->item_charges = 1;
				show_message(vorpal_flash_message, weapon_names[equipped_weapon->item_subtype],
					terse || expert ? "" : vorpal_flash_intensity);

				/*
				 * Sometimes this is a mixed blessing ...
					if (random_below(20) == 0) {
						equipped_weapon->item_flags |= ITEM_CURSED;
						if (!player_saving_throw(VS_MAGIC)) {
							equipped_weapon->item_flags |= ITEM_LEGACY_VORPAL_FLAG|ITEM_SLAYER_REVEALED;
							scroll_identified[SCROLL_VORPALIZE] = TRUE;
							show_message("you feel a sudden desire to kill %ss.",
							monsters[equipped_weapon->item_slays_species-'A'].name);
						}
					}
				 */
			}
		}
		break;
	default:
		show_message("what a puzzling scroll!");
		return;
	}
	update_player_view(TRUE);	/* put the result of the scroll on the screen */
	update_status_line();
	/*
	 * Get rid of the thing
	 */
	inventory_count--;
	if (item->item_quantity > 1)
	item->item_quantity--;
	else
	{
	detach(player_inventory, item);
	consume_scroll = TRUE;
	}
	prompt_item_label(scroll_identified[item->item_subtype], &scroll_labels[item->item_subtype]);

	if (consume_scroll)
	release_entity(item);
}
