/*
 * Function(s) for dealing with potions
 *
 * potions.c	1.4 (AI Design)		2/12/84
 */

#include "rogue.h"
#include "curses.h"


//@ turn_see() wrapper to use as a fuse
static
void
clear_monster_detection(void)
{
	set_monster_detection(TRUE);
}

/*
 * quaff:
 *	Quaff a potion from the pack
 */
void
drink_potion(void)
{
	register Entity *item, *monster;
	register bool consume_potion = FALSE;

	if ((item = select_inventory_item("quaff", POTION)) == NULL)
		return;
	/*
	 * Make certain that it is somethings that we want to drink
	 */
	if (item->item_category != POTION)
	{
		show_message("yuk! Why would you want to drink that?");
		return;
	}
	if (item == equipped_weapon)
		equipped_weapon = NULL;

	/*
	 * Calculate the effect it has on the poor guy.
	 */
	switch (item->item_subtype)
	{
	when POTION_CONFUSION:
		potion_identified[POTION_CONFUSION] = TRUE;
		if (!has_actor_flag(player, ACTOR_CONFUSED))
			{
			if (has_actor_flag(player, ACTOR_CONFUSED))
				extend_delayed_action(end_confusion, random_below(8)+HUHDURATION);
			else
				schedule_delayed_action(end_confusion, random_below(8)+HUHDURATION);
			player.actor_flags |= ACTOR_CONFUSED;
			show_message("wait, what's going on? Huh? What? Who?");
		}
	when POTION_POISON:
		{
		char *sick = "you feel %s sick.";

		potion_identified[POTION_POISON] = TRUE;
		if (!wearing_ring(RING_SUSTAIN_STRENGTH))
		{
			change_player_strength(-(random_below(3)+1));
			show_message(sick, "very");
		}
		else
			show_message(sick, "momentarily");
		}
	when POTION_HEALING:
		potion_identified[POTION_HEALING] = TRUE;
		if ((player_stats.hit_points += roll_dice(player_stats.experience_level, 4)) > player_max_hit_points)
			player_stats.hit_points = ++player_max_hit_points;
		end_blindness();
		show_message("you begin to feel better");
	when POTION_GAIN_STRENGTH:
		potion_identified[POTION_GAIN_STRENGTH] = TRUE;
		change_player_strength(1);
		show_message("you feel stronger. What bulging muscles!");
	when POTION_MONSTER_DETECTION:
#ifndef DEMO
		schedule_delayed_action(clear_monster_detection, HUHDURATION);
		if (level_monsters == NULL)
			show_message("you have a strange feeling%s.",
				verbose_text(" for a moment"));
		else
		{
			if (set_monster_detection(FALSE))
			{
				potion_identified[POTION_MONSTER_DETECTION] = TRUE;
			}
			show_message("");
		}
#else
		show_message("you can't move");
		show_message(" and are forced to watch this advertisement");
		show_message("rogue: The ULTIMATE Adventure Game");
		show_message("the most popular game on UNIX ever!");
		show_message("now runs on YOUR IBM PC");
		show_message("UNIX is a trademark of Bell Labs");
		potion_identified[POTION_MONSTER_DETECTION] = TRUE;
#endif
	  when POTION_MAGIC_DETECTION:
		/*
		 * Potion of magic detection.  Find everything interesting on
		 * the level and show him where they are.  Also give hints as
		 * to whether he would want to use the object.
		 */
		if (level_items != NULL)
		{
			register Entity *entity;
			register bool show;

			show = FALSE;
			for (entity = level_items; entity != NULL; entity = next(entity))
			{
				if (is_magic(entity))
				{
					show = TRUE;
					mvwaddch(hw, entity->item_position.y, entity->item_position.x, display_item_symbol(entity));
					potion_identified[POTION_MAGIC_DETECTION] = TRUE;
				}
			}
			for (monster = level_monsters; monster != NULL; monster = next(monster))
			{
				for (entity = monster->actor_inventory; entity != NULL; entity = next(entity))
				{
					if (is_magic(entity))
					{
						show = TRUE;
						mvwaddch(hw, monster->actor_position.y, monster->actor_position.x, MAGIC);
						potion_identified[POTION_MAGIC_DETECTION] = TRUE;
					}
				}
			}
			if (show)
			{
				show_message("You sense the presence of magic.");
				break;
			}
		}
		show_message("you have a strange feeling for a moment%s.",
				verbose_text(", then it passes"));
	when POTION_PARALYSIS:
		potion_identified[POTION_PARALYSIS] = TRUE;
		incapacitated_turns = HOLDTIME;
		player.actor_flags &= ~ACTOR_CHASING;
		show_message("you can't move");
	when POTION_SEE_INVISIBLE:
		if (!has_actor_flag(player, ACTOR_SEES_INVISIBLE)) {
			schedule_delayed_action(end_monster_detection, SEEDURATION);
			update_player_view(FALSE);
			reveal_invisible_monsters();
		}
		end_blindness();
		show_message("this potion tastes like %s juice", favorite_fruit);
	when POTION_GAIN_LEVEL:
		potion_identified[POTION_GAIN_LEVEL] = TRUE;
		show_message("you suddenly feel much more skillful");
		gain_experience_level();
	when POTION_EXTRA_HEALING:
		potion_identified[POTION_EXTRA_HEALING] = TRUE;
		if ((player_stats.hit_points += roll_dice(player_stats.experience_level, 8)) > player_max_hit_points)
		{
			if (player_stats.hit_points > player_max_hit_points + player_stats.experience_level + 1)
				++player_max_hit_points;
			player_stats.hit_points = ++player_max_hit_points;
		}
		end_blindness();
		show_message("you begin to feel much better");
	when POTION_HASTE:
		potion_identified[POTION_HASTE] = TRUE;
		if (add_haste(TRUE))
			show_message("you feel yourself moving much faster");
	when POTION_RESTORE_STRENGTH:
		if (hand_has_ring(LEFT, RING_ADD_STRENGTH))
			adjust_strength(&player_stats.strength, -equipped_rings[LEFT]->item_modifier);
		if (hand_has_ring(RIGHT, RING_ADD_STRENGTH))
			adjust_strength(&player_stats.strength, -equipped_rings[RIGHT]->item_modifier);
		if (player_stats.strength < maximum_player_stats.strength)
			player_stats.strength = maximum_player_stats.strength;
		if (hand_has_ring(LEFT, RING_ADD_STRENGTH))
			adjust_strength(&player_stats.strength, equipped_rings[LEFT]->item_modifier);
		if (hand_has_ring(RIGHT, RING_ADD_STRENGTH))
			adjust_strength(&player_stats.strength, equipped_rings[RIGHT]->item_modifier);
		show_message("%syou feel warm all over",
			verbose_text("hey, this tastes great.  It makes "));
	when POTION_BLINDNESS:
		potion_identified[POTION_BLINDNESS] = TRUE;
		if (!has_actor_flag(player, ACTOR_BLIND))
		{
			player.actor_flags |= ACTOR_BLIND;
			schedule_delayed_action(end_blindness, SEEDURATION);
			update_player_view(FALSE);
		}
		show_message("a cloak of darkness falls around you");
	when POTION_THIRST_QUENCHING:
		show_message("this potion tastes extremely dull");
	otherwise:
		show_message("what an odd tasting potion!");
		return;
	}
	update_status_line();
	/*
	 * Throw the item away
	 */
	inventory_count--;
	if (item->item_quantity > 1)
		item->item_quantity--;
	else
	{
		detach(player_inventory, item);
		consume_potion = TRUE;
	}

	prompt_item_label(potion_identified[item->item_subtype], &potion_labels[item->item_subtype]);

	if (consume_potion)
		release_entity(item);
}

/*
 * invis_on:
 *	Turn on the ability to see invisible
 */
void
reveal_invisible_monsters(void)
{
	register Entity *actor;

	player.actor_flags |= ACTOR_SEES_INVISIBLE;
	for (actor = level_monsters; actor != NULL; actor = next(actor))
	if (has_actor_flag(*actor, ACTOR_INVISIBLE) && player_can_see_monster(actor))
	{
		mvaddch(actor->actor_position.y, actor->actor_position.x,actor->actor_disguise);
	}
}

/*
 * turn_see:
 *	Put on or off seeing monsters on this level
 */
bool
set_monster_detection(bool turn_off)
{
	register Entity *monster;
	register bool can_see, add_new;
	byte previous_symbol = inch();

	add_new = FALSE;
	for (monster = level_monsters; monster != NULL; monster = next(monster)) {
		move(monster->actor_position.y, monster->actor_position.x);
		can_see = (player_can_see_monster(monster) || (previous_symbol = inch()) == monster->actor_species);
		if (turn_off) {
			if (!player_can_see_monster(monster) && monster->actor_previous_tile != '@')
				addch(monster->actor_previous_tile);
		} else {
			if (!can_see) {
				standout();
				monster->actor_previous_tile = previous_symbol;
			}
			addch(monster->actor_species);
			if (!can_see) {
				standend();
				add_new = TRUE;
			}
		}
	}
	player.actor_flags |= ACTOR_DETECTS_MONSTERS;
	if (turn_off)
		player.actor_flags &= ~ACTOR_DETECTS_MONSTERS;
	return add_new;
}

/*
 * th_effect:
 *	Compute the effect of this potion hitting a monster.
 */
void
apply_thrown_potion(Entity *item, Entity *target)
{
	switch (item->item_subtype)
	{
	when POTION_CONFUSION:
	case POTION_BLINDNESS:
		target->actor_flags |= ACTOR_CONFUSED;
		show_message("the %s appears confused", monster_definitions[target->actor_species-'A'].name);
	when POTION_PARALYSIS:
		target->actor_flags &= ~ACTOR_CHASING;
		target->actor_flags |= ACTOR_HELD;
	when POTION_HEALING:
	case POTION_EXTRA_HEALING:
		if ((target->actor_stats.hit_points += random_below(8)) > target->actor_stats.max_hit_points)
		target->actor_stats.hit_points = ++target->actor_stats.max_hit_points;
	when POTION_GAIN_LEVEL:
		target->actor_stats.hit_points += 8;
		target->actor_stats.max_hit_points += 8;
		target->actor_stats.experience_level++;
	when POTION_HASTE:
		target->actor_flags |= ACTOR_HASTED;
		break;
	}
	show_message("the flask shatters.");
}
