/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * things.c	1.4 (AI Design)	12/14/84
 */

#include "rogue.h"
#include "curses.h"

static void	format_item_by_verbosity(char *output, char *terse_format, char *verbose_format, ...);
static void	show_category_discoveries(byte type);
static void	shuffle_discovery_order(short *order, int type_count);
static int	choose_item_subtype(struct item_definition *magic, int item_count);
static char	*undiscovered_message(byte type);

/*
 * describe_item:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
char *
describe_item(Entity *item, bool dropped)
{
	register int subtype = item->item_subtype;
	register char *text_cursor;

	text_cursor = description_buffer;
	switch (item->item_category)
	{
	when SCROLL:
		if (item->item_quantity == 1) {
			strcpy(text_cursor, "A scroll ");
			text_cursor = &description_buffer[9];
		} else {
			sprintf(text_cursor, "%d scrolls ", item->item_quantity);
			text_cursor = &description_buffer[strlen(description_buffer)];
		}
		if (scroll_identified[subtype])
			sprintf(text_cursor, "of %s", scroll_definitions[subtype].name);
		else if (*scroll_labels[subtype])
			sprintf(text_cursor, "called %s", scroll_labels[subtype]);
		else
			format_item_by_verbosity(text_cursor, "titled '%.17s'","titled '%s'", &scroll_titles[subtype]);
	when POTION:
		if (item->item_quantity == 1)
		{
			strcpy(text_cursor, "A potion ");
			text_cursor = &description_buffer[9];
		}
		else
		{
			sprintf(text_cursor, "%d potions ", item->item_quantity);
			text_cursor = &text_cursor[strlen(description_buffer)];
		}
		if (potion_identified[subtype]) {
			format_item_by_verbosity(text_cursor, "of %s", "of %s(%s)",
				potion_definitions[subtype].name, potion_colors[subtype]);
		}
		else if (*potion_labels[subtype]) {
			format_item_by_verbosity(text_cursor, "called %s","called %s(%s)", potion_labels[subtype],
				potion_colors[subtype]);
		}
		else if (item->item_quantity == 1)
			sprintf(description_buffer, "A%s %s potion", article_suffix(potion_colors[subtype]),
				potion_colors[subtype]);
		else
			sprintf(description_buffer, "%d %s potions", item->item_quantity, potion_colors[subtype]);
	when FOOD:
		if (subtype == 1)
			if (item->item_quantity == 1)
				sprintf(text_cursor, "A%s %s", article_suffix(favorite_fruit), favorite_fruit);
			else
				sprintf(text_cursor, "%d %ss", item->item_quantity, favorite_fruit);
		else
			if (item->item_quantity == 1)
				strcpy(text_cursor, "Some food");
			else
				sprintf(text_cursor, "%d rations of food", item->item_quantity);
	when WEAPON:
		if (item->item_quantity > 1)
			sprintf(text_cursor, "%d ", item->item_quantity);
		else
			sprintf(text_cursor, "A%s ", article_suffix(weapon_names[subtype]));
		text_cursor = &description_buffer[strlen(description_buffer)];
		if (item->item_flags & ITEM_IDENTIFIED)
			sprintf(text_cursor, "%s %s", format_item_bonus(item->item_hit_bonus, item->item_damage_bonus, WEAPON),
				weapon_names[subtype]);
		else
			sprintf(text_cursor, "%s", weapon_names[subtype]);
		if (item->item_quantity > 1)
			strcat(text_cursor, "s");
		if (item->item_slays_species && (item->item_flags & ITEM_SLAYER_REVEALED))
		{
			strcat(text_cursor, " of ");
			strcat(text_cursor, monster_definitions[item->item_slays_species-'A'].name);
			strcat(text_cursor, " slaying");
		}
	when ARMOR:
		if (item->item_flags & ITEM_IDENTIFIED)
			format_item_by_verbosity(text_cursor, "%s %s","%s %s [armor class %d]",
				format_item_bonus(armor_classes[subtype] - item->item_modifier, 0, ARMOR),
				armor_names[subtype], -(item->item_modifier-11));
		else
			sprintf(text_cursor, "%s", armor_names[subtype]);
	when AMULET:
		strcpy(text_cursor, "The Amulet of Yendor");
	when STICK:
		sprintf(text_cursor, "A%s %s ", article_suffix(wand_kinds[subtype]),
		wand_kinds[subtype]);
		text_cursor = &description_buffer[strlen(description_buffer)];
		if (wand_identified[subtype])
			format_item_by_verbosity(text_cursor, "of %s%s", "of %s%s(%s)",
				wand_definitions[subtype].name,
				format_wand_charges(item), wand_materials[subtype]);
		else if (*wand_labels[subtype])
			format_item_by_verbosity(text_cursor, "called %s", "called %s(%s)", wand_labels[subtype],
				wand_materials[subtype]);
		else
			sprintf(text_cursor = &description_buffer[2], "%s %s", wand_materials[subtype], wand_kinds[subtype]);
	when RING:
		if (ring_identified[subtype])
			format_item_by_verbosity(text_cursor, "A%s ring of %s", "A%s ring of %s(%s)", format_ring_bonus(item),
				ring_definitions[subtype].name, ring_gemstones[subtype]);
		else if (*ring_labels[subtype])
			format_item_by_verbosity(text_cursor, "A ring called %s", "A ring called %s(%s)",
				ring_labels[subtype], ring_gemstones[subtype]);
		else
			sprintf(text_cursor, "A%s %s ring", article_suffix(ring_gemstones[subtype]),
				ring_gemstones[subtype]);
#ifdef DEBUG
	when GOLD:
		sprintf(text_cursor, "Gold at %d,%d", item->item_position.y, item->item_position.x);
	otherwise:
		debug("Picked up someting bizzare %s", describe_key(item->item_category));
		sprintf(text_cursor, "Something bizarre %c(%d)", item->item_category, item->item_category);
#endif
		break;
	}
	if (item == equipped_armor)
		strcat(text_cursor, " (being worn)");
	if (item == equipped_weapon)
		strcat(text_cursor, " (weapon in hand)");
	if (item == equipped_rings[LEFT])
		strcat(text_cursor, " (on left hand)");
	else if (item == equipped_rings[RIGHT])
		strcat(text_cursor, " (on right hand)");
	if (dropped && is_monster_symbol(description_buffer[0]))
		description_buffer[0] = tolower(description_buffer[0]);
	else if (!dropped && is_lower(*description_buffer))
		*description_buffer = toupper(*description_buffer);
	return description_buffer;
}

//@ changed original signature to use varargs
static
void
format_item_by_verbosity(char *output, char *terse_format, char *verbose_format, ...)
{
	va_list arguments;
	va_start(arguments, verbose_format);
	vsnprintf(output, MAXSTR, (terse || expert) ? terse_format : verbose_format, arguments);
	va_end(arguments);
}

/*
 * drop_item:
 *	Put something down
 */
void
drop_item(void)
{
	register byte character;
	register Entity *dropped_item, *item;

	character = terrain_at(player_position.y, player_position.x);
	if (character != FLOOR && character != PASSAGE)
	{
		show_message("there is something there already");
		return;
	}
	if ((item = select_inventory_item("drop", 0)) == NULL)
		return;
	if (!can_drop(item))
		return;
	/*
	 * Take it out of the pack
	 */
	if (item->item_quantity >= 2 && item->item_category != WEAPON)
	{
		if ((dropped_item = allocate_entity()) == NULL)
		{
			show_message("%sit appears to be stuck in your pack!",
				verbose_text("can't drop it, "));
			return;
		}
		item->item_quantity--;
		copy_value(*dropped_item,*item);
		dropped_item->item_quantity = 1;
		item = dropped_item;
		if (item->item_stack_group != 0)
			inventory_count++;
	}
	else
		detach(player_inventory, item);
	inventory_count--;
	/*
	 * Link it into the level object list
	 */
	attach(level_items, item);
	terrain_at(player_position.y, player_position.x) = item->item_category;
	copy_value(item->item_position,player_position);
	if (item->item_category == AMULET)
		carrying_amulet = FALSE;
	show_message("dropped %s", describe_item(item, TRUE));
}

/*
 * can_drop:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
bool
can_drop(Entity *item)
{
	if (item == NULL)
		return TRUE;
	if (item != equipped_armor && item != equipped_weapon
		&& item != equipped_rings[LEFT] && item != equipped_rings[RIGHT])
		return TRUE;
	if (item->item_flags & ITEM_CURSED) {
		show_message("you can't.  It appears to be cursed");
		return FALSE;
	}
	if (item == equipped_weapon)
		equipped_weapon = NULL;
	else if (item == equipped_armor) {
		advance_turn();
		equipped_armor = NULL;
	} else {
		register int hand;

		if (item != equipped_rings[hand = LEFT])
			if (item != equipped_rings[hand = RIGHT]) {
#ifdef DEBUG
				debug("Candrop called with funny thing");
#endif
				return TRUE;
			}
		equipped_rings[hand] = NULL;
		switch (item->item_subtype) {
		case RING_ADD_STRENGTH:
			change_player_strength(-item->item_modifier);
			break;
		case RING_SEE_INVISIBLE:
			end_monster_detection();
			cancel_delayed_action(end_monster_detection);
			break;
		}
	}
	return TRUE;
}

/*
 * generate_item:
 *	Return a new thing
 */
Entity *
generate_item(void)
{
	register Entity *item;
	register int j, k;

	if ((item = allocate_entity()) == NULL)
		return NULL;
	item->item_hit_bonus = item->item_damage_bonus = 0;
	item->item_melee_damage = item->item_thrown_damage = "0d0";
	item->item_modifier = 11;
	item->item_quantity = 1;
	item->item_stack_group = 0;
	item->item_flags = 0;
	item->item_slays_species = 0;
	/*
	 * Decide what kind of object it will be
	 * If we haven't had food for a while, let it be food.
	 */
	switch (levels_without_food > 3 ? 2 : choose_item_subtype(item_category_probabilities, NUMTHINGS))
	{
	when 0:
		item->item_category = POTION;
		item->item_subtype = choose_item_subtype(potion_definitions, MAXPOTIONS);
	when 1:
		item->item_category = SCROLL;
		item->item_subtype = choose_item_subtype(scroll_definitions, MAXSCROLLS);
	when 2:
		levels_without_food = 0;
		item->item_category = FOOD;
		if (random_below(10) != 0)
			item->item_subtype = 0;
		else
			item->item_subtype = 1;
	when 3:
		item->item_category = WEAPON;
		item->item_subtype = random_below(MAXWEAPONS);
		init_weapon(item, item->item_subtype);
		if ((k = random_below(100)) < 10)
		{
			item->item_flags |= ITEM_CURSED;
			item->item_hit_bonus -= random_below(3) + 1;
		}
		else if (k < 15)
			item->item_hit_bonus += random_below(3) + 1;
	when 4:
		item->item_category = ARMOR;
		for (j = 0, k = random_below(100); j < MAXARMORS; j++)
			if (k < armor_probabilities[j])
				break;
#ifdef DEBUG
		if (j == MAXARMORS)
		{
		debug("Picked a bad armor %d", k);
		j = 0;
		}
#endif
		item->item_subtype = j;
		item->item_modifier = armor_classes[j];
		if ((k = random_below(100)) < 20)
		{
			item->item_flags |= ITEM_CURSED;
			item->item_modifier += random_below(3) + 1;
		}
		else if (k < 28)
			item->item_modifier -= random_below(3) + 1;
	when 5:
		item->item_category = RING;
		item->item_subtype = choose_item_subtype(ring_definitions, MAXRINGS);
		switch (item->item_subtype)
		{
		when RING_ADD_STRENGTH:
		case RING_PROTECTION:
		case RING_DEXTERITY:
		case RING_DAMAGE:
			if ((item->item_modifier = random_below(3)) == 0)
			{
				item->item_modifier = -1;
				item->item_flags |= ITEM_CURSED;
			}
		when RING_AGGRAVATION:
		case RING_TELEPORTATION:
			item->item_flags |= ITEM_CURSED;
			break;
		}
	when 6:
		item->item_category = STICK;
		item->item_subtype = choose_item_subtype(wand_definitions, MAXSTICKS);
		initialize_wand(item);
#ifdef DEBUG
	otherwise:
		debug("Picked a bad kind of object");
		wait_for_key(' ');
#endif
		break;
	}
	return item;
}

/*
 * choose_item_subtype:
 *	Pick an item out of a list of nitems possible magic items
 */
static
shint  //@ actually an offset, the element index in the array
choose_item_subtype(struct item_definition *magic, int item_count)
{
	register struct item_definition *end;
	register int i;
	register struct item_definition *start;

	start = magic;
	for (end = &magic[item_count], i = random_below(100); magic < end; magic++)
		if (i < magic->probability)
			break;
	if (magic == end)
	{
#ifdef DEBUG
		if (wizard)
		{
			show_message("bad pick_one: %d from %d items", i, item_count);
			for (magic = start; magic < end; magic++)
				show_message("%s: %d%%", magic->name, magic->probability);
		}
#endif
		magic = start;
	}
	return magic - start;
}

/*
 * show_discoveries:
 *	list what the player has discovered in this game of a certain type
 */
static int inventory_line_count = 0;

static bool inventory_new_page = FALSE;

static char *inventory_line_format, *inventory_line_argument;

void
show_discoveries(void)
{
	show_category_discoveries(POTION);
	add_line(empty_string, " ", "");
	show_category_discoveries(SCROLL);
	add_line(empty_string, " ", "");
	show_category_discoveries(RING);
	add_line(empty_string, " ", "");
	show_category_discoveries(STICK);
	end_line(empty_string);
}

/*
 * show_category_discoveries:
 *	Print what we've discovered of type 'type'
 */

#define MAX(a,b,c,d) (a>b?(a>c?(a>d?a:d):(c>d?c:d)):(b>c?(b>d?b:d):(c>d?c:d)))

static
void
show_category_discoveries(byte type)
{
	register bool *identified_types = NULL;
	register char **labels = NULL;
	register int i, type_count = 0, num_found;
	static Entity item;
	static short order[MAX(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];

	switch (type)
	{
	case SCROLL:
		type_count = MAXSCROLLS;
		identified_types = scroll_identified;
		labels = scroll_labels;
		break;
	case POTION:
		type_count = MAXPOTIONS;
		identified_types = potion_identified;
		labels = potion_labels;
		break;
	case RING:
		type_count = MAXRINGS;
		identified_types = ring_identified;
		labels = ring_labels;
		break;
	case STICK:
		type_count = MAXSTICKS;
		identified_types = wand_identified;
		labels = wand_labels;
		break;
	}
	shuffle_discovery_order(order, type_count);
	item.item_quantity = 1;
	item.item_flags = 0;
	num_found = 0;
	for (i = 0; i < type_count; i++)
		if (identified_types[order[i]] || *labels[order[i]])
		{
			item.item_category = type;
			item.item_subtype = order[i];
			add_line(empty_string, "%s", describe_item(&item, FALSE));
			num_found++;
		}
	if (num_found == 0)
		add_line(empty_string, undiscovered_message(type), "");
}

/*
 * shuffle_discovery_order:
 *	Set up order for list
 */
static
void
shuffle_discovery_order(short *order, int type_count)
{
	register int i, random_index, swap_value;

	for (i = 0; i< type_count; i++)
		order[i] = i;

	for (i = type_count; i > 0; i--)
	{
		random_index = random_below(i);
		swap_value = order[i - 1];
		order[i - 1] = order[random_index];
		order[random_index] = swap_value;
	}
}

/*
 * add_line:
 *	Add a line to the list of discoveries
 *
 * VARARGS1
 */
byte
add_line(char *use, char *format, char *arg)
{
	int x, y;
	register byte retchar = ' ';
	if (inventory_line_count == 0)
	{
		save_screen();
		clear();
	}
	if (inventory_line_count >= LINES - 1 || format == NULL)
	{
		move(LINES-1, 0);
		if (*use)
			printw("-Select item to %s. Esc to cancel-", use);
		else
			addstr("-Press space to continue-");
		do
			retchar = read_game_key();
		while (retchar != ESCAPE && retchar != ' ' && (!is_lower(retchar)));
		clear();
		inventory_new_page = TRUE;
		inventory_line_count = 0;
	}
	if (format != NULL && !(inventory_line_count == 0 && *format == '\0'))
	{
		move(inventory_line_count, 0);
		printw(format, arg);
		getxy(&x,&y);
		/*
		 * if the line wrapped but nothing was printed on this
		 * line you might as well use it for the next item
		 */
		if (y!=0)
			inventory_line_count = x + 1;
		inventory_line_format = format;
		inventory_line_argument = arg;
	}
	return(retchar);
}

/*
 * end_line:
 *	End the list of lines
 */
byte
end_line(char *use)
{
	register int retchar;

	retchar = add_line(use, NULL, "");
	restore_screen();
	inventory_line_count = 0;
	inventory_new_page = FALSE;
	return(retchar);
}

/*
 * undiscovered_message:
 *	Set up prbuf so that message for "nothing found" is there
 */
static
char *
undiscovered_message(byte type)
{
	register char *output_cursor, *category_name;

	sprintf(description_buffer, "Haven't discovered anything");
	if (terse)
		sprintf(description_buffer,"Nothing");
	output_cursor = &description_buffer[strlen(description_buffer)];
	switch (type)
	{
		when POTION: category_name = "potion";
		when SCROLL: category_name = "scroll";
		when RING: category_name = "ring";
		when STICK: category_name = "stick";
		//@ not in original, avoid possibly uninitialized use of tystr
		otherwise: category_name = "item";
	}
	sprintf(output_cursor, " about any %ss", category_name);
	return description_buffer;
}
