/*
 * Contains functions for dealing with things that happen in the
 * future.
 *
 * (#)daemon.c	5.2 (Berkeley) 6/18/82
 */

#include "rogue.h"
#include "screen.h"

#define EMPTY_ACTION	0
#define ACTIVE_ACTION	1
#define RECURRING_ACTION -1
#define MAX_SCHEDULED_ACTIONS 20

/*@
 * struct delayed_action, as well as functions using it as return type such
 * as find_free_action_slot() and find_action_slot() are now marked static as the struct is not
 * declared in rogue.h, and they are only used in this file
 */

/*@
 * `int d_arg` member was removed as all fuses and daemons have no arguments,
 * and for the only one that did, set_monster_detection(), the argument type is bool. It's
 * also now wrapped and no longer directly used as fuse, as its return type is
 * not void, making the argument member of this struct unneeded.
 *
 * A solution to handle generic functions of multiple return and argument types
 * would be a somewhat complex approach using unions to simulate overload, a
 * sophistication not needed for Rogue.
 */
static
struct delayed_action {
	void (*callback)(void);
	int turns_remaining;
} scheduled_actions[MAX_SCHEDULED_ACTIONS];

/*
 * find_free_action_slot:
 *	Find an empty slot in the daemon/fuse list
 */
static
struct delayed_action *
find_free_action_slot(void)
{
	register struct delayed_action *action;

	for (action = scheduled_actions; action < &scheduled_actions[MAX_SCHEDULED_ACTIONS]; action++)
		if (action->callback == EMPTY_ACTION)
			return action;
#ifdef DEBUG
	debug("Ran out of fuse slots");
#endif
	return NULL;
}

/*
 * find_action_slot:
 *	Find a particular slot in the table
 */
static
struct delayed_action *
find_action_slot(void (*func)(void))
{
	register struct delayed_action *action;

	for (action = scheduled_actions; action < &scheduled_actions[MAX_SCHEDULED_ACTIONS]; action++)
	if (func == action->callback)
		return action;
	return NULL;
}

/*
 * daemon:
 *	Start a daemon, takes a function.
 */
void
schedule_recurring_action(void (*func)(void))
{
	register struct delayed_action *action;

	action = find_free_action_slot();
	action->callback = func;
	action->turns_remaining = RECURRING_ACTION;
}

/*
 * run_recurring_actions:
 *	Run all the daemons, passing the argument to the function.
 */
void
run_recurring_actions(void)
{
	register struct delayed_action *action;

	/*
	 * Loop through the devil list
	 */
	for (action = scheduled_actions; action < &scheduled_actions[MAX_SCHEDULED_ACTIONS]; action++)
	{
		/*
		 * Executing each one, giving it the proper arguments
		 * @ Sorry, no more "arguments". And it was a single one.
		 */
		if (action->turns_remaining == RECURRING_ACTION && action->callback != EMPTY_ACTION)
		{
			(*action->callback)();
		}
	}
}

/*
 * schedule_delayed_action:
 *	Start a fuse to go off in a certain number of turns
 */
void
schedule_delayed_action(void (*func)(void), int time)
{
	register struct delayed_action *action;

	action = find_free_action_slot();
	action->callback = func;
	action->turns_remaining = time;
}

/*
 * extend_delayed_action:
 *	Increase the time until a fuse goes off
 */
void
extend_delayed_action(void (*func)(void), int xtime)
{
	register struct delayed_action *action;

	if ((action = find_action_slot(func)) == NULL)
		return;
	action->turns_remaining += xtime;
}

/*
 * cancel_delayed_action:
 *	Put out a fuse
 */
void
cancel_delayed_action(void (*func)(void))
{
	register struct delayed_action *action;

	if ((action = find_action_slot(func)) == NULL)
		return;
	action->callback = EMPTY_ACTION;
}

/*
 * run_delayed_actions:
 *	Decrement counters and start needed fuses
 */
void
run_delayed_actions(void)
{
	register struct delayed_action *action;

	/*
	 * Step though the list
	 */
	for (action = scheduled_actions; action < &scheduled_actions[MAX_SCHEDULED_ACTIONS]; action++) {
	/*
	 * Decrementing counters and starting things we want.  We also need
	 * to remove the fuse from the list once it has gone off.
	 */
		if (action->callback != EMPTY_ACTION && action->turns_remaining > 0 && --action->turns_remaining == 0)
		{
			(*action->callback)();
			action->callback = EMPTY_ACTION;
		}
	}
}
