/*
 * Functions for dealing with problems brought about by weapons
 *
 * weapons.c	1.4 (AI Design)	12/22/84
 */

#include "rogue.h"
#include "screen.h"

#define NO_LAUNCHER 100

static struct weapon_definition {
	char *melee_damage;	/* Damage when wielded */
	char *thrown_damage;	/* Damage when thrown */
	char launcher;	/* Launching weapon */
	int flags;	/* Miscellaneous flags */
} weapon_definitions[MAXWEAPONS] = {
	{"2d4",	"1d3",	NO_LAUNCHER,     0},            	/* Mace */
	{"3d4",	"1d2",	NO_LAUNCHER,     0},            	/* Long sword */
	{"1d1",	"1d1",	NO_LAUNCHER,     0},            	/* Bow */
	{"1d1",	"2d3",	BOW,      ITEM_STACKABLE|ITEM_THROWABLE},	/* Arrow */
	{"1d6",	"1d4",	NO_LAUNCHER,     ITEM_THROWABLE},       	/* Dagger */
	{"4d4",	"1d2",	NO_LAUNCHER,     0},            	/* 2h sword */
	{"1d1",	"1d3",	NO_LAUNCHER,     ITEM_STACKABLE|ITEM_THROWABLE},	/* Dart */
	{"1d1",	"1d1",	NO_LAUNCHER,     0},            	/* Crossbow */
	{"1d2",	"2d5",	CROSSBOW, ITEM_STACKABLE|ITEM_THROWABLE},	/* Crossbow bolt */
	{"2d3",	"1d6",	NO_LAUNCHER,     ITEM_THROWABLE}        	/* Spear */
};

static int	find_projectile_landing(Entity *item, Position *landing_position);
static char	*short_name(Entity *item);

/*
 * throw_item:
 *	Fire a missile in a given direction
 */
void
throw_item(int ydelta, int xdelta)
{
	register Entity *item, *projectile;

	/*
	 * Get which thing we are hurling
	 */
	if ((item = select_inventory_item("throw", WEAPON)) == NULL)
		return;
	if (!can_drop(item) || is_equipped(item))
		return;
	/*
	 * Get rid of the thing.  If it is a non-multiple item object, or
	 * if it is the last thing, just drop it.  Otherwise, create a new
	 * item with a count of one.
	 */
	hack:
	if (item->item_quantity < 2) {
		detach(player_inventory, item);
		inventory_count--;
	} else {
		/*
		 * here is a quick hack to check if we can get a new item
		 */
		if ((projectile = allocate_entity()) == NULL) {
			item->item_quantity = 1;
			show_message("something in your pack explodes!!!");
			goto hack;
		}
		item->item_quantity--;
		if (item->item_stack_group == 0)
			inventory_count--;
		copy_value(*projectile,*item);
		projectile->item_quantity = 1;
		item = projectile;
	}
	animate_projectile(item, ydelta, xdelta);
	/*
	 * AHA! Here it has hit something.  If it is a wall or a door,
	 * or if it misses (combat) the monster, put it on the floor
	 */
	if (monster_at(item->item_position.y, item->item_position.x) == NULL
		|| !hit_monster(position_yx(item->item_position), item))
			drop_projectile(item, TRUE);
}

/*
 * animate_projectile:
 *	Do the actual motion on the screen done by an object traveling
 *	across the room
 */
void
animate_projectile(Entity *item, int ydelta, int xdelta)
{
	register byte under = '@';

	/*
	 * Come fly with us ...
	 */
	copy_value(item->item_position,player_position);
	for (;;) {
		register int character;

		/*
		 * Erase the old one
		 */
		if (under != '@' && !positions_equal(item->item_position, player_position) && player_can_see_position(position_yx(item->item_position)))
			mvaddch(item->item_position.y, item->item_position.x, under);
		/*
		 * Get the new position
		 */
		item->item_position.y += ydelta;
		item->item_position.x += xdelta;

		if (is_walkable_symbol(character = visible_entity_at(item->item_position.y, item->item_position.x)) && character != DOOR) {
			/*
			 * It hasn't hit anything yet, so display it
			 * If it alright.
			 */
			if (player_can_see_position(position_yx(item->item_position))) {
				under = terrain_at(item->item_position.y, item->item_position.x);
				mvaddch(item->item_position.y, item->item_position.x, item->item_category);
				tick_pause();
			} else
				under = '@';
			continue;
		}
		break;
	}
}

static
char *
short_name(Entity *item)
{
	switch (item->item_category) {
		case WEAPON: return weapon_names[item->item_subtype];
		case ARMOR: return armor_names[item->item_subtype];
		case FOOD: return "food";
		case POTION:
		case SCROLL:
		case AMULET:
		case STICK:
		case RING:
			return strchr(describe_item(item, TRUE), ' ') + 1;
		default:
			return "bizzare thing";
	}
}

/*
 * drop_projectile:
 *	Drop an item someplace around here.
 */
void
drop_projectile(Entity *item, bool print_message)
{
	static Position landing_position;
	register int index;

	switch (find_projectile_landing(item, &landing_position))
	{
	case 1:
		index = map_index(landing_position.y, landing_position.x);
		terrain_map[index] = item->item_category;
		copy_value(item->item_position,landing_position);
		if (player_can_see_position(landing_position.y, landing_position.x))
		{
			if ((cell_flags_at(item->item_position.y, item->item_position.x) & CELL_PASSAGE) ||
						   (cell_flags_at(item->item_position.y, item->item_position.x) & CELL_MAZE))
				standout();
			mvaddch(landing_position.y, landing_position.x, item->item_category);
			standend();
			if (monster_at(landing_position.y,landing_position.x) != NULL)
				monster_at(landing_position.y,landing_position.x)->actor_previous_tile = item->item_category;
		}
		attach(level_items, item);
		return;
	case 2:
		print_message = 0;
		break;
	}
	if (print_message)
		show_message("the %s vanishes%s.", short_name(item),
								  verbose_text(" as it hits the ground"));
	release_entity(item);
}

/*
 * init_weapon:
 *	Set up the initial goodies for a weapon
 */
void
init_weapon(Entity *weapon, byte type)
{
	register struct weapon_definition *definition;

	definition = &weapon_definitions[type];
	weapon->item_melee_damage = definition->melee_damage;
	weapon->item_thrown_damage = definition->thrown_damage;
	weapon->item_launcher = definition->launcher;
	weapon->item_flags = definition->flags;
	if (weapon->item_flags & ITEM_STACKABLE)
	{
		weapon->item_quantity = random_below(8) + 8;
		weapon->item_stack_group = next_stack_group++;
	}
	else
		weapon->item_quantity = 1;
}

/*
 * hit_monster:
 *	Does the missile hit the monster?
 */
bool
hit_monster(int y, int x, Entity *item)
{
	static Position monster_position;
	register Entity *mo = monster_at(y, x);

	if (mo) {
		monster_position.y = y;
		monster_position.x = x;
		return player_attack(&monster_position, mo->actor_species, item, TRUE);
	}
	return FALSE;
}

/*
 * format_item_bonus:
 *	Figure out the plus number for armor/weapons
 */
char *
format_item_bonus(int n1, int n2, char type)
{
	static char numbuf[10];

	sprintf(numbuf, "%s%d", n1 < 0 ? "" : "+", n1);
	if (type == WEAPON)
		sprintf(&numbuf[strlen(numbuf)], ",%s%d", n2 < 0 ? "" : "+", n2);
	return numbuf;
}

/*
 * wield:
 *	Pull out a certain weapon
 */
void
wield(void)
{
	register Entity *item, *oweapon;
	register char *text_cursor;

	oweapon = equipped_weapon;
	if (!can_drop(equipped_weapon))
	{
		equipped_weapon = oweapon;
		return;
	}
	equipped_weapon = oweapon;
	if ((item = select_inventory_item("wield", WEAPON)) == NULL)
	{
bad:
		turn_consumed = FALSE;
		return;
	}

	if (item->item_category == ARMOR)
	{
		show_message("you can't wield armor");
		goto bad;
	}
	if (is_equipped(item))
		goto bad;

	text_cursor = describe_item(item, TRUE);
	equipped_weapon = item;
	message_by_verbosity2("now wielding %s (%c)", "you are now wielding %s (%c)",
		text_cursor, inventory_key(item));
}

/*
 * find_projectile_landing:
 *	Pick a random position around the given (y, x) coordinates
 */
static
int
find_projectile_landing(Entity *item, Position *landing_position)
{
	register int y, x, count = 0, character;
	Entity *onfloor;

	for (y = item->item_position.y - 1; y <= item->item_position.y + 1; y++) {
		for (x = item->item_position.x - 1; x <= item->item_position.x + 1; x++) {
			/*
			 * check to make certain the spot is empty, if it is,
			 * put the object there, set it in the level list
			 * and re-draw the room if he can see it
			 */
			if ((y == player_position.y && x == player_position.x) || outside_dungeon(y,x))
				continue;
			if ((character = terrain_at(y, x)) == FLOOR || character == PASSAGE) {
				if (random_below(++count) == 0) {
					landing_position->y = y;
					landing_position->x = x;
				}
				continue;
			}
			if (is_walkable_symbol(character)
				&& (onfloor = item_at(y, x))
				&& onfloor->item_category == item->item_category
				&& onfloor->item_stack_group
				&& onfloor->item_stack_group == item->item_stack_group)
			{
				onfloor->item_quantity += item->item_quantity;
				return 2;
			}
		}
	}
	return(count != 0);
}


//@ pause for a tick, ie, 1/18.2 secs (about 55ms)
void
tick_pause(void)
{
/*@ no more busy loops! :)

	register int otick;

	otick = tick;
	while (otick == tick)
#ifdef ROGUE_DOS_CLOCK
		;
#else
		update_protection_state();
#endif
 */
	screen_refresh();
	msleep(55);
}
