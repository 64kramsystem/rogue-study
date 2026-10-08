/*
 * Functions to implement the various sticks one might find
 * while wandering around the dungeon.
 *
 * @(#)sticks.c		1.2 (AI Design)		2/12/84
 */

#include "rogue.h"
#include "screen.h"

/*
 * initialize_wand:
 *	Set up a new stick
 */
void
initialize_wand(register Entity *wand)
{
	if (strcmp(wand_kinds[wand->item_subtype], "staff") == 0)
		wand->item_melee_damage = "2d3";
	else
		wand->item_melee_damage = "1d1";
	wand->item_thrown_damage = "1d1";

	wand->item_charges = 3 + random_below(5);
	switch (wand->item_subtype)
	{
	case WAND_STRIKING:
		wand->item_hit_bonus = 100;
		wand->item_damage_bonus = 3;
		wand->item_melee_damage = "1d8";
		break;
	case WAND_LIGHT:
		wand->item_charges = 10 + random_below(10);
		break;
	}
}

/*
 * zap_wand:
 *	Perform a zap with a wand
 */
void
zap_wand(void)
{
	Entity *item;
	Entity *monster;
	register int y, x;
	register char *name;
	int effect_index;

	if ((item = select_inventory_item("zap with", STICK)) == NULL)
		return;
	effect_index = item->item_subtype;
	if (item->item_category != STICK)
	{
		if (item->item_slays_species && item->item_charges)
			effect_index = MAXSTICKS;
		else
		{
			show_message("you can't zap with that!");
			turn_consumed = FALSE;
			return;
		}
	}
	if (item->item_charges == 0)
	{
		show_message("nothing happens");
		return;
	}
	switch (effect_index)
	{
	case WAND_LIGHT:
		/*
		 * Reddy Kilowat wand.  Light up the room
		 */
		if (has_actor_flag(player,ACTOR_BLIND))
			show_message("you feel a warm glow around you");
		else
		{
			wand_identified[WAND_LIGHT] = TRUE;
			if (player_room->flags & ROOM_ABSENT)
				show_message("the corridor glows and then fades");
			else
				show_message("the room is lit by a shimmering blue light");
		}
		if (!(player_room->flags & ROOM_ABSENT))
		{
			player_room->flags &= ~ROOM_DARK;
			/*
			 * Light the room and put the player back up
			 */
			enter_room(&player_position);
		}
		break;
	case WAND_DRAIN_LIFE:
		/*
		 * Take away 1/2 of hero's hit points, then take it away
		 * evenly from the monsters in the room (or next to hero
		 * if he is in a passage)
		 */
		if (player_stats.hit_points < 2)
		{
			show_message("you are too weak to use it");
			return;
		}
		else
			drain_monsters();
		break;
	case WAND_POLYMORPH:
	case WAND_TELEPORT_AWAY:
	case WAND_TELEPORT_TO:
	case WAND_CANCELLATION:
	case MAXSTICKS:			/* Special case for vorpal weapon */
	{
		register byte monster_definition, oldch;
		register int rm;
		Position new_yx;

		y = player_position.y;
		x = player_position.x;
		while (is_walkable_symbol(visible_entity_at(y, x)))
		{
			y += action_direction.y;
			x += action_direction.x;
		}
		if ((monster = monster_at(y, x)) != NULL)
		{
			register byte omonst;

			omonst = monster_definition = monster->actor_species;
			if (monster_definition == 'F')
				player.actor_flags &= ~ACTOR_HELD;
			if (effect_index == MAXSTICKS)
			{
				if (monster_definition == item->item_slays_species)
				{
					show_message("the %s vanishes in a puff of smoke",
						monster_definitions[monster_definition-'A'].name);
					kill_monster(monster, FALSE);
				}
				else
					show_message("you hear a maniacal chuckle in the distance.");
			}
			else if (effect_index == WAND_POLYMORPH)
			{
				register Entity *pp;

				pp = monster->actor_inventory;
				detach(level_monsters, monster);
				if (player_can_see_monster(monster))
					mvaddch(y, x, terrain_at(y, x));
				oldch = monster->actor_previous_tile;
				action_direction.y = y;
				action_direction.x = x;
				new_monster(monster, monster_definition = random_below(26) + 'A', &action_direction);
				if (player_can_see_monster(monster))
					mvaddch(y, x, monster_definition);
				monster->actor_previous_tile = oldch;
				monster->actor_inventory = pp;
				wand_identified[WAND_POLYMORPH] |= (monster_definition != omonst);
			}
			else if (effect_index == WAND_CANCELLATION)
			{
				monster->actor_flags |= ACTOR_CANCELLED;
				monster->actor_flags &= ~(ACTOR_INVISIBLE|ACTOR_CAN_CONFUSE);
				monster->actor_disguise = monster->actor_species;
			}
			else
			{
				if (player_can_see_monster(monster))
					mvaddch(y, x, monster->actor_previous_tile);
				if (effect_index == WAND_TELEPORT_AWAY)
				{
					monster->actor_previous_tile = '@';
					do
					{
						rm = random_room_index();
						new_yx = monster->actor_position;
						random_room_position(&rooms[rm], &new_yx);
					}  while (!(is_floor_tile(visible_entity_at(new_yx.y, new_yx.x))));
					monster->actor_position = new_yx;
					if (player_can_see_monster(monster))
						mvaddch(monster->actor_position.y, monster->actor_position.x, monster->actor_disguise);
					else if (has_actor_flag(player, ACTOR_DETECTS_MONSTERS))
					{
						standout();
						mvaddch(monster->actor_position.y, monster->actor_position.x, monster->actor_disguise);
						standend();
					}
				}
				else /* it MUST BE at WAND_TELEPORT_TO */
				{
					monster->actor_position.y = player_position.y + action_direction.y;
					monster->actor_position.x = player_position.x + action_direction.x;
				}
				if (monster->actor_species == 'F')
					player.actor_flags &= ~ACTOR_HELD;
				if (monster->actor_position.y != y || monster->actor_position.x != x)
					monster->actor_previous_tile = mvinch(monster->actor_position.y, monster->actor_position.x);
			}
			monster->actor_destination = &player_position;
			monster->actor_flags |= ACTOR_CHASING;
		}
	} break;
	case WAND_MAGIC_MISSILE: {
		Entity bolt;

		wand_identified[WAND_MAGIC_MISSILE] = TRUE;
		bolt.item_category = '*';
		bolt.item_thrown_damage = "1d8";
		bolt.item_hit_bonus = 1000;
		bolt.item_damage_bonus = 1;
		bolt.item_flags = ITEM_THROWABLE;
		if (equipped_weapon != NULL)
			bolt.item_launcher = equipped_weapon->item_subtype;
		animate_projectile(&bolt, action_direction.y, action_direction.x);
		if ((monster = monster_at(bolt.item_position.y, bolt.item_position.x)) != NULL && !actor_saving_throw(VS_MAGIC, monster))
			hit_monster(position_yx(bolt.item_position), &bolt);
		else
		show_message("the missle vanishes with a puff of smoke");
	} break;
	case WAND_STRIKING:
		action_direction.y += player_position.y;
		action_direction.x += player_position.x;
		if ((monster = monster_at(action_direction.y, action_direction.x)) != NULL)
		{
			if (random_below(20) == 0)
			{
				item->item_melee_damage = "3d8";
				item->item_damage_bonus = 9;
			}
			else
			{
				item->item_melee_damage = "2d8";
				item->item_damage_bonus = 4;
			}
			player_attack(&action_direction, monster->actor_species, item, FALSE);
		}
		break;
	case WAND_HASTE_MONSTER:
	case WAND_SLOW_MONSTER:
		y = player_position.y;
		x = player_position.x;
		while (is_walkable_symbol(visible_entity_at(y, x)))
		{
			y += action_direction.y;
			x += action_direction.x;
		}
		if ((monster = monster_at(y, x)) != NULL)
		{
			if (effect_index == WAND_HASTE_MONSTER)
			{
				if (has_actor_flag(*monster, ACTOR_SLOWED))
					monster->actor_flags &= ~ACTOR_SLOWED;
				else
					monster->actor_flags |= ACTOR_HASTED;
			}
			else
			{
				if (has_actor_flag(*monster, ACTOR_HASTED))
					monster->actor_flags &= ~ACTOR_HASTED;
				else
					monster->actor_flags |= ACTOR_SLOWED;
				monster->actor_move_this_turn = TRUE;
			}
			action_direction.y = y;
			action_direction.x = x;
			start_monster_chase(&action_direction);
		}
		break;
	case WAND_LIGHTNING:
	case WAND_FIRE:
	case WAND_COLD:
		if (effect_index == WAND_LIGHTNING)
			name = "bolt";
		else if (effect_index == WAND_FIRE)
			name = "flame";
		else
			name = "ice";
		fire_bolt(&player_position, &action_direction, name);
		wand_identified[effect_index] = TRUE;
#ifdef DEBUG
		break;
	default:
		show_message("what a bizarre schtick!");
#endif
		break;
	}
	if (--item->item_charges < 0)
		item->item_charges = 0;
}

/*
 * drain_monsters:
 *	Do drain hit points from player schtick
 */
void
drain_monsters(void)
{
	Entity *monster;
	register int target_count;
	register struct room *player_region;
	register Entity **target_cursor;
	register bool in_passage;
	Entity *targets[40];

	/*
	 * First cnt how many things we need to spread the hit points among
	 */
	target_count = 0;
	if (terrain_at(player_position.y, player_position.x) == DOOR)
		player_region = &passages[cell_flags_at(player_position.y, player_position.x) & PASSAGE_NUMBER_MASK];
	else
		player_region = NULL;
	in_passage = (player_room->flags & ROOM_ABSENT);
	target_cursor = targets;
	for (monster = level_monsters; monster != NULL; monster = next(monster))
		if (monster->actor_room == player_room || monster->actor_room == player_region ||
			(in_passage && terrain_at(monster->actor_position.y, monster->actor_position.x) == DOOR &&
			&passages[cell_flags_at(monster->actor_position.y, monster->actor_position.x) & PASSAGE_NUMBER_MASK] == player_room))
			*target_cursor++ = monster;
	if ((target_count = target_cursor - targets) == 0)
	{
		show_message("you have a tingling feeling");
		return;
	}
	*target_cursor = NULL;
	player_stats.hit_points /= 2;
	target_count = player_stats.hit_points / target_count + 1;
	/*
	 * Now zot all of the monsters
	 */
	for (target_cursor = targets; *target_cursor; target_cursor++)
	{
		monster = *target_cursor;
		if ((monster->actor_stats.hit_points -= target_count) <= 0)
			kill_monster(monster, player_can_see_monster(monster));
		else
			start_monster_chase(&monster->actor_position);
	}
}

/*
 * fire_bolt:
 *	Fire a bolt in a given direction from a specific starting place
 */
void
fire_bolt(Position *start, Position *direction, char *name)
{
	register byte bolt_symbol = 0, character;
	register Entity *monster;
	register bool hit_hero, used, changed;
	register int i, j;
	Position position;
	struct {
		Position s_pos;
		byte s_under;
	} trail[BOLT_LENGTH*2];
	Entity bolt;
	bool is_frost;

	is_frost = (strcmp(name, "frost") == 0);
	bolt.item_category = WEAPON;
	bolt.item_subtype = FLAME;
	bolt.item_melee_damage = bolt.item_thrown_damage = "6d6";
	bolt.item_hit_bonus = 30;
	bolt.item_damage_bonus = 0;
	weapon_names[FLAME] = name;
	switch (direction->y + direction->x) {
	case 0:
		bolt_symbol = '/';
		break;
	case 1:
	case -1:
		bolt_symbol = (direction->y == 0 ? '-' : '|');
		break;
	case 2:
	case -2:
		bolt_symbol = '\\';
		break;
	}
	position = *start;
	hit_hero = (start != &player_position);
	used = FALSE;
	changed = FALSE;
	for (i = 0; i < BOLT_LENGTH && !used; i++) {
		position.y += direction->y;
		position.x += direction->x;
		character = visible_entity_at(position.y, position.x);
		trail[i].s_pos = position;
		if ((trail[i].s_under = mvinch(position.y, position.x)) == bolt_symbol)
			trail[i].s_under = 0;
		switch (character) {
		case DOOR:
		case HWALL:
		case VWALL:
		case ULWALL:
		case URWALL:
		case LLWALL:
		case LRWALL:
		case ' ':
			if (!changed)
				hit_hero = !hit_hero;
			changed = FALSE;
			direction->y = -direction->y;
			direction->x = -direction->x;
			i--;
			show_message("the %s bounces", name);
			break;
		default:
			if (!hit_hero && (monster = monster_at(position.y, position.x)) != NULL) {
				hit_hero = TRUE;
				changed = !changed;
				if (monster->actor_previous_tile != '@')
					monster->actor_previous_tile = terrain_at(position.y, position.x);
				if (!actor_saving_throw(VS_MAGIC, monster) || is_frost) {
					bolt.item_position = position;
					used = TRUE;
					if (monster->actor_species == 'D' && strcmp(name, "flame") == 0)
						show_message("the flame bounces off the dragon");
					else {
						hit_monster(position_yx(position), &bolt);
						if (mvinch(position_yx(position)) != bolt_symbol)
							trail[i].s_under = mvinch(position_yx(position));
					}
				} else if (character != 'X' || monster->actor_disguise == 'X') {
					if (start == &player_position)
						start_monster_chase(&position);
					show_message("the %s whizzes past the %s",
						name, monster_definitions[character-'A'].name);
				}
			} else if (hit_hero && positions_equal(position, player_position)) {
				hit_hero = FALSE;
				changed = !changed;
				if (!player_saving_throw(VS_MAGIC)) {
					if (is_frost) {
						show_message("You are frozen by a blast of frost%s.",
							verbose_text(" from the Ice Monster"));
						if (incapacitated_turns < 20)
							incapacitated_turns += randomize_duration(7);
					} else if ((player_stats.hit_points -= roll_dice(6, 6)) <= 0) {
						if (start == &player_position)
							show_death_screen('b');
						else
							show_death_screen(monster_at(start->y, start->x)->actor_species);
					}
					used = TRUE;
					if (!is_frost)
						show_message("you are hit by the %s", name);
				} else
					show_message("the %s whizzes by you", name);
			}
			if (is_frost)
				blue();
			else
				red();
			tick_pause();
			mvaddch(position.y, position.x, bolt_symbol);
			standend();
			break;
		}
	}
	for (j = 0; j < i; j++) {
		tick_pause();
		if (trail[j].s_under)
			mvaddch(trail[j].s_pos.y, trail[j].s_pos.x, trail[j].s_under);
	}
}

/*
 * format_wand_charges:
 *	Return an appropriate string for a wand charge
 */
char *
format_wand_charges(register Entity *item)
{
	static char buffer[20];

	if (!(item->item_flags & ITEM_IDENTIFIED))
		buffer[0] = '\0';
	else
		sprintf(buffer, " [%d charges]", item->item_charges);
	return buffer;
}
