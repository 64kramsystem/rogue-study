
/*
 * Special wizard commands (some of which are also non-wizard commands
 * under strange circumstances)
 *
 * wizard.c	1.4 (AI Design)	12/14/84
 */

#include "rogue.h"
#include "screen.h"

#ifdef WIZARD
static int	get_num(int *place);
#endif


/*
 * identify_item:
 *	What a certin object is
 */
void
identify_item(void)
{
	register Entity *item;

	if (player_inventory == NULL) {
		show_message("You don't have anything in your pack to identify");
		return;
	}

	for (;;) {
		if ((item = select_inventory_item("identify", 0)) == NULL) {
			show_message("You must identify something");
			show_message(" ");
			message_column = 0;
		} else
			break;
	}

	switch (item->item_category) {
	when SCROLL:
		scroll_identified[item->item_subtype] = TRUE;
		*scroll_labels[item->item_subtype] = '\0';
	when POTION:
		potion_identified[item->item_subtype] = TRUE;
		*potion_labels[item->item_subtype] = '\0';
	when STICK:
		wand_identified[item->item_subtype] = TRUE;
		item->item_flags |= ITEM_IDENTIFIED;
		*wand_labels[item->item_subtype] = '\0';
	when WEAPON:
	case ARMOR:
		item->item_flags |= ITEM_IDENTIFIED;
	when RING:
		ring_identified[item->item_subtype] = TRUE;
		item->item_flags |= ITEM_IDENTIFIED;
		*ring_labels[item->item_subtype] = '\0';
		break;
	}
	/*
	 * If it is vorpally enchanted, then reveal what type of monster it is
	 * vorpally enchanted against
	 */
	if (item->item_slays_species)
		item->item_flags |= ITEM_SLAYER_REVEALED;
	show_message("%s", describe_item(item, FALSE));
}

#ifdef WIZARD
/*
 * create_obj:
 *	Wizard command for getting anything he wants
 */
void
create_obj(void)
{
	Entity *obj;
	byte ch, bless;

	if ((obj = allocate_entity()) == NULL)
	{
		show_message("can't create anything now");
		return;
	}
	show_message("type of item: ");
	switch (read_game_key()) {
		when '!': obj->item_category = POTION;
		when '?': obj->item_category = SCROLL;
		when '/': obj->item_category = STICK;
		when '=': obj->item_category = RING;
		when ')': obj->item_category = WEAPON;
		when ']': obj->item_category = ARMOR;
		when ',': obj->item_category = AMULET;
		otherwise:
			obj->item_category = FOOD;
	}
	message_column = 0;
	show_message("which %c do you want? (0-f)", obj->item_category);
	obj->item_subtype = (is_digit((ch = read_game_key())) ? ch - '0' : ch - 'a' + 10);
	obj->item_stack_group = 0;
	obj->item_quantity = 1;
	obj->item_melee_damage = obj->item_thrown_damage = "0d0";
	message_column = 0;
	if (obj->item_category == WEAPON || obj->item_category == ARMOR)
	{
		show_message("blessing? (+,-,n)");
		bless = read_game_key();
		message_column = 0;
		if (bless == '-')
			obj->item_flags |= ITEM_CURSED;
		if (obj->item_category == WEAPON)
		{
			init_weapon(obj, obj->item_subtype);
			if (bless == '-')
				obj->item_hit_bonus -= random_below(3)+1;
			if (bless == '+')
				obj->item_hit_bonus += random_below(3)+1;
		}
		else
		{
			obj->item_modifier = armor_classes[obj->item_subtype];
			if (bless == '-')
				obj->item_modifier += random_below(3)+1;
			if (bless == '+')
				obj->item_modifier -= random_below(3)+1;
		}
	}
	else if (obj->item_category == RING)
		switch (obj->item_subtype)
		{
		case RING_PROTECTION:
		case RING_ADD_STRENGTH:
		case RING_DEXTERITY:
		case RING_DAMAGE:
			show_message("blessing? (+,-,n)");
			bless = read_game_key();
			message_column = 0;
			if (bless == '-')
				obj->item_flags |= ITEM_CURSED;
			obj->item_modifier = (bless == '-' ? -1 : random_below(2) + 1);
		when RING_AGGRAVATION:
		case RING_TELEPORTATION:
			obj->item_flags |= ITEM_CURSED;
			/* fallthrough */
		}
	else if (obj->item_category == STICK)
		initialize_wand(obj);
	else if (obj->item_category == GOLD)
	{
		show_message("how much?");
		get_num(&obj->item_gold_amount, stdscr);
	}
	add_to_inventory(obj, FALSE);
}
#endif

/*
			 * telport:
			 *	Bamf the hero someplace else
			 */
int
teleport(void)
{
	register int rm;
	Position column;

	mvaddch(player_position.y, player_position.x, terrain_at(player_position.y, player_position.x));
	do
	{
		rm = random_room_index();
		random_room_position(&rooms[rm], &column);
	} while (!(is_walkable_symbol(visible_entity_at(column.y, column.x))));
	if (&rooms[rm] != player_room)
	{
		leave_room(&player_position);
		copy_value(player_position,column);
		enter_room(&player_position);
	}
	else
	{
		copy_value(player_position,column);
		update_player_view(TRUE);
	}
	mvaddch(player_position.y, player_position.x, PLAYER);
	/*
	 * turn off ACTOR_HELD in case teleportation was done while fighting
	 * a Fungi
	 */
	if (has_actor_flag(player, ACTOR_HELD)) {
		player.actor_flags &= ~ACTOR_HELD;
		reset_flytrap_damage();
	}
	immobile_turns = 0;
	command_repeat_count = 0;
	running = FALSE;
	clear_macro_input();
	/*
	 * Teleportation can be a confusing experience
	 * (unless you really are a wizard)
	 */
#ifdef WIZARD
	if (!wizard)
	{
#endif //WIZARD
	if (has_actor_flag(player, ACTOR_CONFUSED))
		extend_delayed_action(end_confusion, random_below(4)+2);
	else
		schedule_delayed_action(end_confusion, random_below(4)+2);
	player.actor_flags |= ACTOR_CONFUSED;
#ifdef WIZARD
	}
#endif //WIZARD
	return rm;
}

#ifdef WIZARD
#ifdef UNIX
/*
 * passwd:
 *	See if user knows password
 *	@ unused
 */
static
bool
passwd(void)
{
	register char *sp, c;
	char buf[MAXSTR], *crypt();

	show_message("wizard's Password:");
	message_column = 0;
	sp = buf;
	while ((c = getchar()) != '\n' && c != '\r' && c != ESCAPE)
		if (c == _tty.sg_kill)
			sp = buf;
		else if (c == _tty.sg_erase && sp > buf)
			sp--;
		else
			*sp++ = c;
	if (sp == buf)
		return FALSE;
	*sp = '\0';
	return (strcmp(PASSWD, crypt(buf, "mT")) == 0);
}
#endif  // UNIX

/*
 * show_map:
 *	Print out the map for the wizard
 *	@unused, which is a shame...
 */
static
void
show_map(void)
{
	register int y, x, real;

	save_screen();
	clear();
	for (y = 1; y < dungeon_bottom_row; y++)
	for (x = 0; x < COLS; x++)
	{
		if (!(real = cell_flags_at(y, x) & CELL_REVEALED))
		standout();
		mvaddch(y, x, terrain_at(y, x));
		if (!real)
		standend();
	}
	show_overlay_message("---More (level map)---");
	restore_screen();
}

static
int
get_num(int *place)
{
	char numbuf[12];

	read_line(numbuf,10);
	*place = atoi(numbuf);
	return(*place);
}
#endif  // WIZARD
