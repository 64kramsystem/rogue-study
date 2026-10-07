/*
 * Routines dealing specifically with rings
 *
 * rings.c		1.4 (AI Design)		12/13/84
 */

#include "rogue.h"
#include "curses.h"

static int	choose_ring_hand(void);

/*
 * ring_on:
 *	Put a ring on a hand
 */
void
put_on_ring()
{
	register Entity *item;
	register int ring = -1;

	if ((item = select_inventory_item("put on", RING)) == NULL)
		goto no_ring;
	/*
	 * Make certain that it is somethings that we want to wear
	 */
	if (item->item_category != RING) {
		show_message("you can't put that on your finger");
		goto no_ring;
	}

	/*
	 * find out which hand to put it on
	 */
	if (is_equipped(item))
		goto no_ring;

	if (equipped_rings[LEFT] == NULL)
		ring = LEFT;
	if (equipped_rings[RIGHT] == NULL)
		ring = RIGHT;
	if (equipped_rings[LEFT] == NULL && equipped_rings[RIGHT] == NULL)
		if ((ring = choose_ring_hand()) < 0)
			goto no_ring;
	if (ring < 0) {
		show_message("you already have a ring on each hand");
		goto no_ring;
	}
	equipped_rings[ring] = item;

	/*
	 * Calculate the effect it has on the poor guy.
	 */
	switch (item->item_subtype) {
	case RING_ADD_STRENGTH:
		change_player_strength(item->item_modifier);
		break;
	case RING_SEE_INVISIBLE:
		reveal_invisible_monsters();
		break;
	case RING_AGGRAVATION:
		aggravate_monsters();
		break;
	}

	show_message("%swearing %s (%c)", verbose_text("you are now "),
		describe_item(item, TRUE), inventory_key(item));
	return ;

no_ring:
	turn_consumed = FALSE;
	return;
}

/*
 * ring_off:
 *	Take off a ring
 */
void
remove_ring(void)
{
	register int ring;
	register Entity *item;
	register char item_key;

	if (equipped_rings[LEFT] == NULL && equipped_rings[RIGHT] == NULL) {
		show_message("you aren't wearing any rings");
		turn_consumed = FALSE;
		return;
	} else if (equipped_rings[LEFT] == NULL)
		ring = RIGHT;
	else if (equipped_rings[RIGHT] == NULL)
		ring = LEFT;
	else
		if ((ring = choose_ring_hand()) < 0)
			return;
	message_column = 0;
	item = equipped_rings[ring];
	if (item == NULL) {
		show_message("not wearing such a ring");
		turn_consumed = FALSE;
		return;
	}
	item_key = inventory_key(item);
	if (can_drop(item))
		show_message("was wearing %s(%c)", describe_item(item, TRUE), item_key);
}

/*
 * gethand:
 *	Which hand is the hero interested in?
 */
static
int
choose_ring_hand(void)
{
	register int column;

	for (;;) {
		show_message("left hand or right hand? ");
		if ((column = read_game_key()) == ESCAPE)  {
			turn_consumed = FALSE;
			return -1;
		}
		message_column = 0;
		if (column == 'l' || column == 'L')
			return LEFT;
		else if (column == 'r' || column == 'R')
			return RIGHT;
		show_message("please type L or R");
	}
	return -1;
}

/*
 * ring_eat:
 *	How much food does this ring use up?
 */
int
ring_food_cost(int hand)
{
	if (equipped_rings[hand] == NULL)
		return 0;
	switch (equipped_rings[hand]->item_subtype) {
	case RING_REGENERATION:
		return 2;
	case RING_SUSTAIN_STRENGTH:
	case RING_MAINTAIN_ARMOR:
	case RING_PROTECTION:
	case RING_ADD_STRENGTH:
	case RING_STEALTH:
		return 1;
	case RING_SEARCHING:
		return(random_below(5)==0);
	case RING_DEXTERITY:
	case RING_DAMAGE:
		return (random_below(3) == 0);
	case RING_SLOW_DIGESTION:
		return -random_below(2);
	case RING_SEE_INVISIBLE:
		return (random_below(5) == 0);
	default:
		return 0;
	}
}

/*
 * ring_num:
 *	Print ring bonuses
 */
char *
format_ring_bonus(Entity *item)
{
	if (!(item->item_flags & ITEM_IDENTIFIED))
		return "";
	switch (item->item_subtype) {
	when RING_PROTECTION:
	case RING_ADD_STRENGTH:
	case RING_DAMAGE:
	case RING_DEXTERITY:
		ring_bonus_buffer[0] = ' ';
		strcpy(&ring_bonus_buffer[1], format_item_bonus(item->item_modifier, 0, RING));
	otherwise:
		return "";
	}
	return ring_bonus_buffer;
}
