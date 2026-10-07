/*
 * All the daemon and fuse functions are in here
 *
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rogue.h"
#include "curses.h"


/*
 * doctor:
 *	A healing daemon that restores hit points after rest
 */
void
regenerate_health(void)
{
	register int experience_level, previous_hit_points;

	experience_level = player_stats.experience_level;
	previous_hit_points = player_stats.hit_points;
	healing_turns++;
	if (experience_level < 8)
	{
		if (healing_turns + (experience_level << 1) > 20)
			player_stats.hit_points++;
	}
	else
	if (healing_turns >= 3)
		player_stats.hit_points += random_below(experience_level - 7) + 1;
	if (hand_has_ring(LEFT, RING_REGENERATION))
		player_stats.hit_points++;
	if (hand_has_ring(RIGHT, RING_REGENERATION))
		player_stats.hit_points++;
	if (previous_hit_points != player_stats.hit_points)
	{
		if (player_stats.hit_points > player_max_hit_points)
			player_stats.hit_points = player_max_hit_points;
		healing_turns = 0;
	}
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
void
start_wander_checks(void)
{
	schedule_recurring_action(check_wandering_spawn);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */
void
check_wandering_spawn(void)
{
	static int turns_since_check = 0;

	if (++turns_since_check >= 3 + random_below(3))
	{
		if (roll_dice(1, 6) == 4)
		{
			spawn_wandering_monster();
			cancel_delayed_action(check_wandering_spawn);
			schedule_delayed_action(start_wander_checks, WANDERTIME);
		}
	turns_since_check = 0;
	}
}

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */
void
end_confusion(void)
{
	player.actor_flags &= ~ACTOR_CONFUSED;
	show_message("you feel less confused now");
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
void
end_monster_detection(void)
{
	register Entity *actor;

	for (actor = level_monsters; actor != NULL; actor = next(actor))
		if (has_actor_flag(*actor, ACTOR_INVISIBLE) && player_can_see_monster(actor) && actor->actor_previous_tile != '@')
			mvaddch(actor->actor_position.y, actor->actor_position.x,actor->actor_previous_tile);
	player.actor_flags &= ~ACTOR_SEES_INVISIBLE;
}

/*
 * sight:
 *	He gets his sight back
 */
void
end_blindness(void)
{
	if (has_actor_flag(player, ACTOR_BLIND))
	{
		cancel_delayed_action(end_blindness);
		player.actor_flags &= ~ACTOR_BLIND;
		if (!(player_room->flags & ROOM_ABSENT))
			enter_room(&player_position);
		show_message("the veil of darkness lifts");
	}
}

/*
 * nohaste:
 *	End the hasting
 */
void
end_haste(void)
{
	player.actor_flags &= ~ACTOR_HASTED;
	show_message("you feel yourself slowing down");
}

/*
 * stomach:
 *	Digest the hero's food
 */
void
consume_food(void)
{
	register int previous_food, food_consumed;

	if (food_remaining <= 0)
	{
		if (food_remaining-- < -STARVETIME)
			show_death_screen('s');
		/*
		 * the hero is fainting
		 */
		if (incapacitated_turns || random_below(5) != 0)
			return;
		incapacitated_turns += random_below(8) + 4;
		player.actor_flags &= ~ACTOR_CHASING;
		running = FALSE;
		command_repeat_count = 0;
		hunger_state = 3;
		show_message("%syou faint from lack of food",verbose_text("you feel very weak. "));
	}
	else
	{
		previous_food = food_remaining;
		/*
		 * If you are in 40 column mode use food twice as fast
		 * (e.g. 3-(80/40) = 1, 3-(40/40) = 2 : pretty gross huh?)
		 */
		food_consumed = ring_food_cost(LEFT) + ring_food_cost(RIGHT) + 1;
		if (terse)
			food_consumed *= 2;
		food_remaining -= food_consumed;

		if (food_remaining < MORETIME && previous_food >= MORETIME)
		{
			hunger_state = 2;
			show_message("you are starting to feel weak");
		}
		else if (food_remaining < 2 * MORETIME && previous_food >= 2 * MORETIME)
		{
			hunger_state = 1;
			show_message("you are starting to get hungry");
		}
	}
}
