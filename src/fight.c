/*
 * All the fighting gets done here
 *
 * @(#)fight.c		1.43 (AI Design)		1/19/85
 */

#include "rogue.h"
#include "screen.h"

/*
 * player_attack:
 *	The player attacks the monster.
 */
bool
player_attack(Position *monster_position, char monster_symbol, Entity *weapon, bool thrown)
{
	register Entity *monster;
	register char *monster_name;

	/*
	 * Find the monster we want to fight
	 */
	if ((monster = monster_at(monster_position->y, monster_position->x)) == 0)
		return FALSE;
	/*
	 * Since we are fighting, things are not quiet so no healing takes
	 * place.  Cancel any command counts so player can recover.
	 */
	command_repeat_count = healing_turns = 0;
	start_monster_chase(monster_position);
	/*
	 * Let him know it was really a mimic (if it was one).
	 */
	if (monster->actor_species == 'X' && monster->actor_disguise != 'X' && !has_actor_flag(player, ACTOR_BLIND)) {
		monster_symbol = monster->actor_disguise = 'X';
		if (thrown)
			return FALSE;
		show_message("wait! That's a Xeroc!");
	}
	monster_name = monster_definitions[monster_symbol-'A'].name;
	if (has_actor_flag(player, ACTOR_BLIND))
		monster_name = pronoun_it;
	if (resolve_attack_damage(&player, monster, weapon, thrown)||(weapon && weapon->item_category == POTION)) {
		bool did_huh = FALSE;

		if (thrown)
			report_projectile_hit(weapon, monster_name, "hits", "hit");
		else
			report_hit(NULL, monster_name);
		//@ original missed NULL check for weap
		if (weapon && weapon->item_category == POTION) {
			apply_thrown_potion(weapon, monster);
			if (!thrown) {
				if (weapon->item_quantity > 1)
					weapon->item_quantity--;
				else {
					detach(player_inventory, weapon);
					release_entity(weapon);
				}
				equipped_weapon = NULL;
			}
		}
		if (has_actor_flag(player, ACTOR_CAN_CONFUSE)) {
			did_huh = TRUE;
			monster->actor_flags |= ACTOR_CONFUSED;
			player.actor_flags &= ~ACTOR_CAN_CONFUSE;
			show_message("your hands stop glowing red");
		}
		if (monster->actor_stats.hit_points <= 0)
			kill_monster(monster, TRUE);
		else if (did_huh && !has_actor_flag(player, ACTOR_BLIND))
			show_message("the %s appears confused", monster_name);
		return TRUE;
	}
	if (thrown)
		report_projectile_hit(weapon, monster_name, "misses", "missed");
	else
		report_miss(NULL, monster_name);
	if (monster->actor_species == 'S' && random_below(100) > 25)
		slime_split(monster);
	return FALSE;
}

/*
 * monster_attack:
 *	The monster attacks the player
 */
void
monster_attack(Entity *monster)
{
	register char *monster_name;

	/*
	 * Since this is an attack, stop running and any healing that was
	 * going on at the time.
	 */
	running = FALSE;
	command_repeat_count = healing_turns = 0;
	if (monster->actor_species == 'X' && !has_actor_flag(player, ACTOR_BLIND))
		monster->actor_disguise = 'X';
	monster_name = monster_definitions[monster->actor_species-'A'].name;
	if (has_actor_flag(player, ACTOR_BLIND))
		monster_name = pronoun_it;
	if (resolve_attack_damage(monster, &player, NULL, FALSE)) {
		report_hit(monster_name, NULL);
		if (player_stats.hit_points <= 0)
			show_death_screen(monster->actor_species);	/* Bye bye life ... */
		if (!has_actor_flag(*monster, ACTOR_CANCELLED))
			switch (monster->actor_species)
		{
		when 'A':
			/*
			 * If a rust monster hits, you lose armor, unless
			 * that armor is leather or there is a magic ring
			 */
			if (equipped_armor != NULL && equipped_armor->item_modifier < 9
			  && equipped_armor->item_subtype != LEATHER)
			{
				if (wearing_ring(RING_MAINTAIN_ARMOR))
					show_message("the rust vanishes instantly");
				else
				{
					show_message("your armor weakens, oh my!");
					equipped_armor->item_modifier++;
				}
			}
		when 'I':
			/*
			 * When an Ice Monster hits you, you get unfrozen faster
			 */
			if (incapacitated_turns > 1)
				incapacitated_turns--;
			break;
		when 'R':
			/*
			 * Rattlesnakes have poisonous bites
			 */
			if (!player_saving_throw(VS_POISON))
			{
				if (!wearing_ring(RING_SUSTAIN_STRENGTH))
				{
					change_player_strength(-1);
					show_message("you feel a bite in your leg%s",
						verbose_text(" and now feel weaker"));
				}
				else
					show_message("a bite momentarily weakens you");
			}
		when 'W':
		case 'V':
			/*
			 * Wraiths might drain energy levels, and Vampires
			 * can steal player_max_hit_points
			 */
			if (random_below(100) < (monster->actor_species == 'W' ? 15 : 30))
			{
			register int fewer;

			if (monster->actor_species == 'W')
			{
				if (player_stats.experience == 0)
				show_death_screen('W');		/* All levels gone */
				if (--player_stats.experience_level == 0)
				{
				player_stats.experience = 0;
				player_stats.experience_level = 1;
				}
				else
				player_stats.experience = experience_thresholds[player_stats.experience_level-1]+1;
				fewer = roll_dice(1, 10);
			}
			else
				fewer = roll_dice(1, 5);
			player_stats.hit_points -= fewer;
			player_max_hit_points -= fewer;
			if (player_stats.hit_points < 1)
				player_stats.hit_points = 1;
			if (player_max_hit_points < 1)
				show_death_screen(monster->actor_species);
			show_message("you suddenly feel weaker");
			}
		when 'F':
			/*
			 * Violet fungi stops the poor guy from moving
			 */
			player.actor_flags |= ACTOR_HELD;
			sprintf(monster->actor_stats.damage_dice,"%dd1",++flytrap_damage);
		when 'L':
		{
			/*
			 * Leperachaun steals some gold
			 */
			register long lastpurse;

			lastpurse = player_gold;
			player_gold -= GOLDCALC;
			if (!player_saving_throw(VS_MAGIC))
			player_gold -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
			if (player_gold < 0)
			player_gold = 0;
			remove_monster(&monster->actor_position, monster, FALSE);
			if (player_gold != lastpurse)
			show_message("your purse feels lighter");
		}
		when 'N':
		{
			register Entity *item, *steal;
			register int nobj;
			char *she_stole = "she stole %s!";

			/*
			 * Nymph's steal a magic item, look through the pack
			 * and pick out one we like.
			 */
			steal = NULL;
			for (nobj = 0, item = player_inventory; item != NULL; item = next(item))
			if (item != equipped_armor && item != equipped_weapon
				&& item != equipped_rings[LEFT] && item != equipped_rings[RIGHT]
				&& is_magic(item) && random_below(++nobj) == 0)
				steal = item;
			if (steal != NULL)
			{
				remove_monster(&monster->actor_position, monster, FALSE);
				inventory_count--;
				if (steal->item_quantity > 1 && steal->item_stack_group == 0)
				{
					register int oc;

					oc = steal->item_quantity--;
					steal->item_quantity = 1;
					show_message(she_stole, describe_item(steal, TRUE));
					steal->item_quantity = oc;
				}
				else
				{
					detach(player_inventory, steal);
					release_entity(steal);
					show_message(she_stole, describe_item(steal, TRUE));
				}
			}
		}
		otherwise:
			break;
		}
	}
	else if (monster->actor_species != 'I')
	{
	if (monster->actor_species == 'F')
	{
		player_stats.hit_points -= flytrap_damage;
		if (player_stats.hit_points <= 0)
		show_death_screen(monster->actor_species);	/* Bye bye life ... */
	}
	report_miss(monster_name, NULL);
	}
	clear_macro_input();
	command_repeat_count = 0;
	update_status_line();
}

/*
 * attack_hits:
 *	Returns true if the swing hits
 */
bool
attack_hits(int attacker_level, int defender_armor, int hit_bonus)
{
	register int attack_roll = random_below(20);
	register int required_roll = (20 - attacker_level) - defender_armor;

	return (attack_roll + hit_bonus >= required_roll);
}

/*
 * check_experience_level:
 *	Check to see if the guy has gone up a level.
 */
void
check_experience_level(void)
{
	register int i, hit_points_gained, previous_level;

	for (i = 0; experience_thresholds[i] != 0; i++)
	if (experience_thresholds[i] > player_stats.experience)
		break;
	i++;
	previous_level = player_stats.experience_level;
	player_stats.experience_level = i;
	if (i > previous_level)
	{
		hit_points_gained = roll_dice(i - previous_level, 10);
		player_max_hit_points += hit_points_gained;
		if ((player_stats.hit_points += hit_points_gained) > player_max_hit_points)
			player_stats.hit_points = player_max_hit_points;
		show_message("and achieve the rank of \"%s\"", rank_names[i-1]);
	}
}

/*
 * resolve_attack_damage:
 *	Roll several attacks
 */
bool
resolve_attack_damage(Entity *attacker, Entity *defender, Entity *weapon, bool thrown)
{
	register struct combat_stats *attacker_stats, *defender_stats;
	char *damage_cursor;
	int dice_count, dice_sides, defender_armor;
	register bool did_hit = FALSE;
	register int hit_bonus;
	register int damage_bonus;
	register int damage;
	attacker_stats = &attacker->actor_stats;
	defender_stats = &defender->actor_stats;
	if (weapon == NULL)
	{
		damage_cursor = attacker_stats->damage_dice;
		damage_bonus = 0;
		hit_bonus = 0;
	}
	else
	{
		hit_bonus = weapon->item_hit_bonus;
		damage_bonus = weapon->item_damage_bonus;
		/*
		 * Check for vorpally enchanted weapon
		 */
		if (defender->actor_species == weapon->item_slays_species)
		{
			hit_bonus += 4;
			damage_bonus += 4;
		}
		if (weapon == equipped_weapon)
		{
			if (hand_has_ring(LEFT, RING_DAMAGE))
				damage_bonus += equipped_rings[LEFT]->item_modifier;
			else if (hand_has_ring(LEFT, RING_DEXTERITY))
				hit_bonus += equipped_rings[LEFT]->item_modifier;
			if (hand_has_ring(RIGHT, RING_DAMAGE))
				damage_bonus += equipped_rings[RIGHT]->item_modifier;
			else if (hand_has_ring(RIGHT, RING_DEXTERITY))
				hit_bonus += equipped_rings[RIGHT]->item_modifier;
		}
		damage_cursor = weapon->item_melee_damage;
		if (thrown && (weapon->item_flags&ITEM_THROWABLE) && equipped_weapon != NULL &&
			  equipped_weapon->item_subtype == weapon->item_launcher)
		{
			damage_cursor = weapon->item_thrown_damage;
			hit_bonus += equipped_weapon->item_hit_bonus;
			damage_bonus += equipped_weapon->item_damage_bonus;
		}
		/*
		 * Drain a staff of striking
		 */
		if (weapon->item_category == STICK && weapon->item_subtype == WAND_STRIKING
			&& --weapon->item_charges < 0)
		{
			damage_cursor = weapon->item_melee_damage = "0d0";
			weapon->item_hit_bonus = weapon->item_damage_bonus = 0;
			weapon->item_charges = 0;
		}
	}

	//@ New NULL check to prevent segfault on atoi()
	if (damage_cursor == NULL)
	{
		return FALSE;
	}

	/*
	 * If the creature being attacked is not running (alseep or held)
	 * then the attacker gets a plus four bonus to hit.
	 */
	if (!has_actor_flag(*defender, ACTOR_CHASING))
		hit_bonus += 4;
	defender_armor = defender_stats->armor_class;
	if (defender_stats == &player_stats)
	{
		if (equipped_armor != NULL)
			defender_armor = equipped_armor->item_modifier;
		if (hand_has_ring(LEFT, RING_PROTECTION))
			defender_armor -= equipped_rings[LEFT]->item_modifier;
		if (hand_has_ring(RIGHT, RING_PROTECTION))
			defender_armor -= equipped_rings[RIGHT]->item_modifier;
	}
	for (;;)
	{
		dice_count = atoi(damage_cursor);
		if ((damage_cursor = strchr(damage_cursor, 'd')) == NULL)
			break;
		dice_sides = atoi(++damage_cursor);
		if (attack_hits(attacker_stats->experience_level, defender_armor, hit_bonus + strength_hit_bonus(attacker_stats->strength)))
		{
			register int rolled_damage;

			rolled_damage = roll_dice(dice_count, dice_sides);
			damage = damage_bonus + rolled_damage + strength_damage_bonus(attacker_stats->strength);
			/*
			 * special goodies for the commercial version of rogue
			 */
				if (defender == &player && deepest_level == 1)
				 /*
				  * make it easier on level one
				  */
						damage = (damage+1) / 2;
				/*
				 * copy protection goodies
				 */
				if (defender == &player)
					damage *= incoming_damage_multiplier;
			defender_stats->hit_points -= max(0, damage);
			did_hit = TRUE;
		}
		if ((damage_cursor = strchr(damage_cursor, '/')) == NULL)
			break;
		damage_cursor++;
	}
	return did_hit;
}

//@ No need to declare in rogue.h
/*
 * format_combat_name:
 *	The print name of a combatant
 */
char *
format_combat_name(char *combatant_name, bool capitalize)
{
	*combat_name_buffer = '\0';
	if (combatant_name == 0)
		strcpy(combat_name_buffer, pronoun_you);
	else if (has_actor_flag(player, ACTOR_BLIND))
		strcpy(combat_name_buffer, pronoun_it);
	else
	{
		strcpy(combat_name_buffer, "the ");
		strcat(combat_name_buffer, combatant_name);
	}
	if (capitalize)
		*combat_name_buffer = toupper(*combat_name_buffer);
	return combat_name_buffer;
}

/*
 * report_hit:
 *	Print a message to indicate a succesful hit
 */
void
report_hit(char *attacker_name, char *defender_name)
{
	register char *message_format = "";

	append_message(format_combat_name(attacker_name, TRUE));
	switch ((terse || expert) ? 1 : random_below(4))
	{
		when 0: message_format = " scored an excellent hit on ";
		when 1: message_format = " hit ";
		when 2: message_format = (attacker_name == 0 ? " have injured " : " has injured ");
		when 3: message_format = (attacker_name == 0 ? " swing and hit " : " swings and hits ");
		break;
	}
	show_message("%s%s",message_format,format_combat_name(defender_name, FALSE));
}

/*
 * report_miss:
 *	Print a message to indicate a poor swing
 */
void
report_miss(char *attacker_name, char *defender_name)
{
	register char *message_format = "";


	append_message(format_combat_name(attacker_name, TRUE));
	switch ((terse || expert) ? 1 : random_below(4))
	{
		when 0: message_format = (attacker_name == 0 ? " swing and miss" : " swings and misses");
		when 1: message_format = (attacker_name == 0 ? " miss" : " misses");
		when 2: message_format = (attacker_name == 0 ? " barely miss" : " barely misses");
		when 3: message_format = (attacker_name == 0 ? " don't hit" : " doesn't hit");
		break;
	}
	show_message("%s %s",message_format,format_combat_name(defender_name, FALSE));
}

/*
 * actor_saving_throw:
 *	See if a creature save against something
 */
bool
actor_saving_throw(int which, Entity *entity)
{
	register int need;

	need = 14 + which - entity->actor_stats.experience_level / 2;
	return (roll_dice(1, 20) >= need);
}

/*
 * player_saving_throw:
 *	See if he saves against various nasty things
 */
bool
player_saving_throw(int which)
{
	if (which == VS_MAGIC) {
		if (hand_has_ring(LEFT, RING_PROTECTION))
			which -= equipped_rings[LEFT]->item_modifier;
		if (hand_has_ring(RIGHT, RING_PROTECTION))
			which -= equipped_rings[RIGHT]->item_modifier;
	}
	return actor_saving_throw(which, &player);
}

/*
 * strength_hit_bonus:
 *	Compute bonus/penalties for strength on the "to hit" roll
 */
int
strength_hit_bonus(Strength strength)
{
	register int bonus = 4;

	if (strength < 8)
		return strength - 7;
	if (strength < 31)
		bonus--;
	if (strength < 21)
		bonus--;
	if (strength < 19)
		bonus--;
	if (strength < 17)
		bonus--;
	return bonus;
}

/*
 * strength_damage_bonus:
 *	Compute additional damage done for exceptionally high or low strength
 */
int
strength_damage_bonus(Strength strength)
{
	int bonus = 6;

	if (strength < 8)
		return strength - 7;
	if (strength < 31)
		bonus--;
	if (strength < 22)
		bonus--;
	if (strength < 20)
		bonus--;
	if (strength < 18)
		bonus--;
	if (strength < 17)
		bonus--;
	if (strength < 16)
		bonus--;
	return bonus;
}

/*
 * gain_experience_level:
 *	The guy just magically went up a level.
 */
void
gain_experience_level(void)
{
	player_stats.experience = experience_thresholds[player_stats.experience_level-1] + 1L;
	check_experience_level();
}

/*
 * report_projectile_hit:
 *	A missile hit or missed a monster
 */
void
report_projectile_hit(Entity *weapon, char *monster_name, char *present_verb, char *past_verb)
{
	if (weapon->item_category == WEAPON)
		append_message("the %s %s ", weapon_names[weapon->item_subtype], present_verb);
	else
		append_message("you %s ", past_verb);
	if (has_actor_flag(player, ACTOR_BLIND))
		show_message(pronoun_it);
	else
		show_message("the %s", monster_name);
}

//@ renamed from remove() to avoid conflict with <stdio.h>
/*
 * remove_monster:
 *	Remove a monster from the screen
 */
void
remove_monster(Position *position, Entity *monster, bool killed)
{
	register Entity *item, *next_item;

	if (monster == NULL)
		return;

	for (item = monster->actor_inventory; item != NULL; item = next_item)
	{
		next_item = next(item);
		copy_value(item->item_position,monster->actor_position);
		detach(monster->actor_inventory, item);
		if (killed)
			drop_projectile(item, FALSE);
		else
			release_entity(item);
	}
	if (terrain_map[map_index(position->y,position->x)] == PASSAGE)
		standout();
	if (monster->actor_previous_tile == FLOOR && !player_can_see_position(position->y, position->x))
		mvaddch(position->y, position->x, ' ');
	else if (monster->actor_previous_tile != '@')
		mvaddch(position->y, position->x, monster->actor_previous_tile);
	standend();
	detach(level_monsters, monster);
	release_entity(monster);
}

/*
 * is_magic:
 *	Returns true if an object radiates magic
 */
bool
is_magic(Entity *item)
{
	switch (item->item_category)
	{
	case ARMOR:
		return item->item_modifier != armor_classes[item->item_subtype];
	case WEAPON:
		return item->item_hit_bonus != 0 || item->item_damage_bonus != 0;
	case POTION:
	case SCROLL:
	case STICK:
	case RING:
	case AMULET:
		return TRUE;
	}
	return FALSE;
}

/*
 * kill_monster:
 *	Called to put a monster to death
 */
void
kill_monster(Entity *monster, bool print_message)
{
	player_stats.experience += monster->actor_stats.experience;
	/*
	 * If the monster was a violet fungi, un-hold him
	 */
	switch (monster->actor_species)
	{
	when 'F':
		player.actor_flags &= ~ACTOR_HELD;
		reset_flytrap_damage();
	when 'L':;
		register Entity *gold;

		if ((gold = allocate_entity()) == NULL)
			return;
		gold->item_category = GOLD;
		gold->item_gold_amount = GOLDCALC;
		if (player_saving_throw(VS_MAGIC))
			gold->item_gold_amount += GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
		attach(monster->actor_inventory, gold);
		break;
	}
	/*
	 * Get rid of the monster.
	 */
	remove_monster(&monster->actor_position, monster, TRUE);
	if (print_message)
	{
	append_message("you have defeated ");
	if (has_actor_flag(player, ACTOR_BLIND))
		show_message(pronoun_it);
	else
		show_message("the %s", monster_definitions[monster->actor_species-'A'].name);
	}
	/*
	 * Do adjustments if he went up a level
	 */
	check_experience_level();
}
