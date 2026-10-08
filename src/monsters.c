/*
 * File with various monster functions in it
 *
 * monsters.c	1.4 (A.I. Design)	12/14/84
 */

#include "rogue.h"
#include "screen.h"

static int	monster_experience_bonus(Entity *monster);

/*
 * List of monsters in rough order of vorpalness
 */

/*@
 * Note:  vorpal_monster_choices was not present in the original v1.48 code.  It is used to
 * select a target for the Vorpalize Weapon scroll.
 * Previously, level_monster_choices was used, which contains spaces.  When a space
 * character was selected, identifying the player's weapon then indexed the
 * monsters array out-of-bounds, causing a segfault.
 *
 * From disassembling earlier Rogue PC versions, we can deduce that level_monster_choices
 * originally had no spaces when the Vorpalize scroll was introduced, so
 * re-introducing this string with no spaces is believed to reproduce the
 * intended behavior.
 */

static char *vorpal_monster_choices = "KEBHISORZLCAQNYTWFPUGMXVJD";
static char *level_monster_choices =  "K BHISOR LCA NYTWFP GMXVJD";
static char *wandering_monster_choices = "KEBHISORZ CAQ YTW PUGM VJ ";

/*
 * random_monster_species:
 *	Pick a monster to show up.  The lower the level,
 *	the meaner the monster.
 */
char
random_monster_species(bool wander)
{
	register int species_index;
	register char *species_choices;

	species_choices = wander ? wandering_monster_choices : level_monster_choices;
	do {
		int depth_adjustment = random_below(5) + random_below(6);

		species_index = dungeon_level + (depth_adjustment - 5);
		if (species_index < 1)
			species_index = random_below(5) + 1;
		if (species_index > 26)
			species_index = random_below(5) + 22;
	} while (species_choices[--species_index] == ' ');
	return species_choices[species_index];
}

/*
 * new_monster:
 *	Pick a new monster and add it to the list
 */
void
new_monster(Entity *monster, byte type, Position *spawn_position)
{
	register struct monster_definition *definition;
	register int depth_bonus;

	if ((depth_bonus = dungeon_level - AMULETLEVEL) < 0)
		depth_bonus = 0;
	attach(level_monsters, monster);
	monster->actor_species = type;
	monster->actor_disguise = type;
	copy_value(monster->actor_position,*spawn_position);
	monster->actor_previous_tile = '@';
	monster->actor_room = room_at(spawn_position);
	definition = &monster_definitions[monster->actor_species-'A'];
	monster->actor_stats.experience_level = definition->stats.experience_level + depth_bonus;
	monster->actor_stats.max_hit_points = monster->actor_stats.hit_points = roll_dice(monster->actor_stats.experience_level, 8);
	monster->actor_stats.armor_class = definition->stats.armor_class - depth_bonus;
	monster->actor_stats.damage_dice = definition->stats.damage_dice;
	monster->actor_stats.strength = definition->stats.strength;
	monster->actor_stats.experience = definition->stats.experience + depth_bonus * 10 + monster_experience_bonus(monster);
	monster->actor_flags = definition->flags;
	monster->actor_move_this_turn = TRUE;
	monster->actor_inventory = NULL;
	if (wearing_ring(RING_AGGRAVATION))
		start_monster_chase(spawn_position);
	if (type == 'F')
		monster->actor_stats.damage_dice = flytrap_damage_dice;
	if (type == 'X')
	{
		switch (random_below(dungeon_level > 25 ? 9 : 8))
		{
		case 0:
			monster->actor_disguise = GOLD;
			break;
		case 1:
			monster->actor_disguise = POTION;
			break;
		case 2:
			monster->actor_disguise = SCROLL;
			break;
		case 3:
			monster->actor_disguise = STAIRS;
			break;
		case 4:
			monster->actor_disguise = WEAPON;
			break;
		case 5:
			monster->actor_disguise = ARMOR;
			break;
		case 6:
			monster->actor_disguise = RING;
			break;
		case 7:
			monster->actor_disguise = STICK;
			break;
		case 8:
			monster->actor_disguise = AMULET;
			break;
		}
	}
}

/*
 *  reset_flytrap_damage(): restor initial damage string for flytraps
 */
void
reset_flytrap_damage(void)
{
	register struct monster_definition *definition = &monster_definitions['F'-'A'];

	flytrap_damage = 0;
	strcpy(flytrap_damage_dice, definition->stats.damage_dice);
}

/*
 * expadd:
 *	Experience to add for this monster's level/hit points
 */
static
int
monster_experience_bonus(Entity *monster)
{
	register int experience_bonus;

	if (monster->actor_stats.experience_level == 1)
		experience_bonus = monster->actor_stats.max_hit_points / 8;
	else
		experience_bonus = monster->actor_stats.max_hit_points / 6;
	if (monster->actor_stats.experience_level > 9)
		experience_bonus *= 20;
	else if (monster->actor_stats.experience_level > 6)
		experience_bonus *= 4;
	return experience_bonus;
}

/*
 * spawn_wandering_monster:
 *	Create a new wandering monster and aim it at the player
 */
void
spawn_wandering_monster(void)
{
	int i;
	register struct room *room;
	register Entity *monster;
	Position spawn_position;

	/*
	 * can we allocate a new monster
	 */
	if ((monster = allocate_entity()) == NULL)
		return;
	do {
		i = random_room_index();
		if ((room = &rooms[i]) == player_room)
			continue;
		random_room_position(room, &spawn_position);
	} while (!(room != player_room && is_walkable_symbol(visible_entity_at(spawn_position.y, spawn_position.x))));
	new_monster(monster, random_monster_species(TRUE), &spawn_position);
#ifdef TEST
	if (pending_trapdoor_fall && me())
		show_message("wanderer bailout");
#endif //TEST
#ifdef WIZARD
	if (wizard)
		show_message("started a wandering %s", monster_definitions[monster->actor_species-'A'].name);
#endif
	start_monster_chase(&monster->actor_position);
}

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster
 */
Entity *
wake_monster(int y, int x)
{
	register Entity *monster;
	register struct room *room;
	register byte character;
	register int gold_distance;

	if ((monster = monster_at(y, x)) == NULL)
		return monster;
	character = monster->actor_species;
	/*
	 * Every time he sees mean monster, it might start chasing him
	 */
	if (!has_actor_flag(*monster, ACTOR_CHASING) && random_below(3) != 0 && has_actor_flag(*monster, ACTOR_AGGRESSIVE) && !has_actor_flag(*monster, ACTOR_HELD)
		&& !wearing_ring(RING_STEALTH))
	{
		monster->actor_destination = &player_position;
		monster->actor_flags |= ACTOR_CHASING;
	}
	if (character == 'M' && !has_actor_flag(player, ACTOR_BLIND) && !has_actor_flag(*monster, ENTITY_ENCOUNTERED)
		&& !has_actor_flag(*monster, ACTOR_CANCELLED) && has_actor_flag(*monster, ACTOR_CHASING))
	{
		room = player_room;
		gold_distance = distance_squared(y, x, player_position.y, player_position.x);
		if ((room != NULL && !(room->flags & ROOM_DARK)) || gold_distance < LAMPDIST) {
			monster->actor_flags |= ENTITY_ENCOUNTERED;
			if (!player_saving_throw(VS_MAGIC)) {
				if (has_actor_flag(player, ACTOR_CONFUSED))
					extend_delayed_action(end_confusion, random_below(20) + HUHDURATION);
				else
					schedule_delayed_action(end_confusion, random_below(20) + HUHDURATION);
				player.actor_flags |= ACTOR_CONFUSED;
				show_message("the medusa's gaze has confused you");
			}
		}
	}
	/*
	 * Let greedy ones guard gold
	 */
	if (has_actor_flag(*monster, ACTOR_GREEDY) && !has_actor_flag(*monster, ACTOR_CHASING)) {
		monster->actor_flags = monster->actor_flags | ACTOR_CHASING;
		if (player_room->gold_amount)
			monster->actor_destination = &player_room->gold_position;
		else
			monster->actor_destination = &player_position;
	}
	return monster;
}

/*
 * give_monster_item:
 *	Give a pack to a monster if it deserves one
 */
void
give_monster_item(Entity *monster)
{
	/*
	 * check if we can allocate a new item
	 */
	if (allocated_entity_count < MAXITEMS && random_below(100) < monster_definitions[monster->actor_species-'A'].carry_probability)
		attach(monster->actor_inventory, generate_item());
}

/*
 * random_vorpal_enemy:
 *	Choose a sort of monster for the enemy of a vorpally enchanted weapon
 *
 *	@ Fixed:  level_monster_choices renamed to vorpal_monster_choices, to prevent this function from
 *	  returning space characters.  See comment for vorpal_monster_choices above.
 */
char
random_vorpal_enemy(void)
{
	register char *text_cursor = vorpal_monster_choices + strlen(vorpal_monster_choices);

	while (--text_cursor >= vorpal_monster_choices && random_below(10))
		;
	if (text_cursor < vorpal_monster_choices)
		return 'M';
	return *text_cursor;
}


/*
 * monster_at(x,y)
 *    returns pointer to monster at coordinate
 *	  if no monster there return NULL
 */

Entity *
monster_at(int y, int x)
{
	register Entity *monster;

	for (monster = level_monsters ; monster != NULL ; monster = next(monster))
		if (monster->actor_position.x == x  && monster->actor_position.y == y)
			return(monster);
	return(NULL);
}
