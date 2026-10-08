/*
 * global variable initializaton
 *
 * init.c	1.4 (A.I. Design) 12/14/84
 */

#include "rogue.h"
#include "screen.h"

Entity *entity_pool;
int   *entity_slot_used;

/*
 * init_player:
 *	Roll up the rogue
 */
void
init_player(void)
{
	register Entity *item;
	copy_value(player_stats,maximum_player_stats);
	food_remaining = HUNGERTIME;
	/*
	 * initialize things
	 */
	fill_bytes(entity_pool,MAXITEMS*sizeof(Entity),0);
	fill_bytes(entity_slot_used,MAXITEMS*sizeof(int),0);
	/*
	 * Give the rogue his weaponry.  First a mace.
	 */
	item = allocate_entity();
	item->item_category = WEAPON;
	item->item_subtype = MACE;
	init_weapon(item, MACE);
	item->item_hit_bonus = 1;
	item->item_damage_bonus = 1;
	item->item_flags |= ITEM_IDENTIFIED;
	item->item_quantity = 1;
	item->item_stack_group = 0;
	add_to_inventory(item, TRUE);
	equipped_weapon = item;
	/*
	 * Now a +1 bow
	 */
	item = allocate_entity();
	item->item_category = WEAPON;
	item->item_subtype = BOW;
	init_weapon(item, BOW);
	item->item_hit_bonus = 1;
	item->item_damage_bonus = 0;
	item->item_quantity = 1;
	item->item_stack_group = 0;
	item->item_flags |= ITEM_IDENTIFIED;
	add_to_inventory(item, TRUE);
	/*
	 * Now some arrows
	 */
	item = allocate_entity();
	item->item_category = WEAPON;
	item->item_subtype = ARROW;
	init_weapon(item, ARROW);
	item->item_quantity = random_below(15) + 25;
	item->item_hit_bonus = item->item_damage_bonus = 0;
	item->item_flags |= ITEM_IDENTIFIED;
	add_to_inventory(item, TRUE);
	/*
	 * And his suit of armor
	 */
	item = allocate_entity();
	item->item_category = ARMOR;
	item->item_subtype = RING_MAIL;
	item->item_modifier = armor_classes[RING_MAIL] - 1;
	item->item_flags |= ITEM_IDENTIFIED;
	item->item_quantity = 1;
	item->item_stack_group = 0;
	equipped_armor = item;
	add_to_inventory(item, TRUE);
	/*
	 * Give him some food too
	 */
	item = allocate_entity();
	item->item_category = FOOD;
	item->item_quantity = 1;
	item->item_subtype = 0;
	item->item_stack_group = 0;
	add_to_inventory(item, TRUE);
}

/*
 * Contains definitions and functions for dealing with things like
 * potions and scrolls
 */

static char *potion_color_choices[] = {
	"amber",
	"aquamarine",
	"black",
	"blue",
	"brown",
	"clear",
	"crimson",
	"cyan",
	"ecru",
	"gold",
	"green",
	"grey",
	"magenta",
	"orange",
	"pink",
	"plaid",
	"purple",
	"red",
	"silver",
	"tan",
	"tangerine",
	"topaz",
	"turquoise",
	"vermilion",
	"violet",
	"white",
	"yellow"
};

#define NCOLORS (sizeof potion_color_choices / sizeof (char *))

static char *consonants = "bcdfghjklmnpqrstvwxyz";
static char *vowels = "aeiou";

typedef struct {
	char	*name;
	int		value;
} Gemstone;

static Gemstone stones[] = {
	{ "agate",		 25},
	{ "alexandrite",	 40},
	{ "amethyst",	 50},
	{ "carnelian",	 40},
	{ "diamond",	300},
	{ "emerald",	300},
	{ "germanium",	225},
	{ "granite",	  5},
	{ "garnet",		 50},
	{ "jade",		150},
	{ "kryptonite",	300},
	{ "lapis lazuli",	 50},
	{ "moonstone",	 50},
	{ "obsidian",	 15},
	{ "onyx",		 60},
	{ "opal",		200},
	{ "pearl",		220},
	{ "peridot",	 63},
	{ "ruby",		350},
	{ "sapphire",	285},
	{ "stibotantalite",	200},
	{ "tiger eye",	 50},
	{ "topaz",		 60},
	{ "turquoise",	 70},
	{ "taaffeite",	300},
	{ "zircon",	 	 80}
};

#define NSTONES (sizeof stones / sizeof (Gemstone))

static char *wooden_wand_materials[] = {
	"avocado wood",
	"balsa",
	"bamboo",
	"banyan",
	"birch",
	"cedar",
	"cherry",
	"cinnibar",
	"cypress",
	"dogwood",
	"driftwood",
	"ebony",
	"elm",
	"eucalyptus",
	"fall",
	"hemlock",
	"holly",
	"ironwood",
	"kukui wood",
	"mahogany",
	"manzanita",
	"maple",
	"oaken",
	"persimmon wood",
	"pecan",
	"pine",
	"poplar",
	"redwood",
	"rosewood",
	"spruce",
	"teak",
	"walnut",
	"zebrawood"
};

#define NWOOD (sizeof wooden_wand_materials / sizeof (char *))

static char *metal_wand_materials[] = {
	"aluminum",
	"beryllium",
	"bone",
	"brass",
	"bronze",
	"copper",
	"electrum",
	"gold",
	"iron",
	"lead",
	"magnesium",
	"mercury",
	"nickel",
	"pewter",
	"platinum",
	"steel",
	"silver",
	"silicon",
	"tin",
	"titanium",
	"tungsten",
	"zinc"
};

#define NMETAL (sizeof metal_wand_materials / sizeof (char *))

/*
 * initialize_item_probabilities
 *	Initialize the probabilities for types of things
 */
void
initialize_item_probabilities(void)
{
	register struct item_definition *definition;

	for (definition = &item_category_probabilities[1]; definition <= &item_category_probabilities[NUMTHINGS-1]; definition++)
		definition->probability += (definition-1)->probability;
}

/*
 * initialize_potion_colors:
 *	Initialize the potion color scheme for this time
 */
void
initialize_potion_colors(void)
{
	unsigned int i, j;
	bool used[NCOLORS];

	for (i = 0; i < NCOLORS; i++)
		used[i] = FALSE;
	for (i = 0; i < MAXPOTIONS; i++)
	{
		do
			j = random_below(NCOLORS);
		while (used[j]);
		used[j] = TRUE;
		potion_colors[i] = potion_color_choices[j];
		potion_identified[i] = FALSE;
		potion_labels[i] = (char *)&item_label_storage[next_item_label++];
		if (i > 0)
			potion_definitions[i].probability += potion_definitions[i-1].probability;
	}
}

/*
 * initialize_scroll_titles:
 *	Generate the names of the various scrolls
 */
void
initialize_scroll_titles(void)
{
	 int syllable_count;
	 register char *title_cursor, *syllable;
	 int i, word_count;

	for (i = 0; i < MAXSCROLLS; i++)
	{
	title_cursor = description_buffer;
	word_count = random_below(terse?3:4) + 2;
	while (word_count--)
	{
		syllable_count = random_below(2) + 1;
		while (syllable_count--)
		{
		syllable = random_syllable();
		if (&title_cursor[strlen(syllable)] > &description_buffer[MAXNAME-1])
		{
			word_count = 0;
			break;
		}
		while (*syllable)
			*title_cursor++ = *syllable++;
		}
		*title_cursor++ = ' ';
	}
	*--title_cursor = '\0';
	/*
	 * I'm tired of thinking about this one so just in case .....
	 */
	description_buffer[MAXNAME] = 0;
	scroll_identified[i] = FALSE;
	scroll_labels[i] = (char *)&item_label_storage[next_item_label++];
	strcpy((char *)(&scroll_titles[i]), description_buffer);
	if (i > 0)
		scroll_definitions[i].probability += scroll_definitions[i-1].probability;
	}
}

/*
 * random_syllable()
 *   -- generate a random sylable
 */
char *
random_syllable(void)
{
	static char syllable[4];

	syllable[3] = 0;
	syllable[2] = random_character(consonants);
	syllable[1] = random_character(vowels);
	syllable[0] = random_character(consonants);
	return (syllable);
}

/*
 * random_character()
 *    return random character in given string
 */
char
random_character(char *string)
{
	return(string[random_below(strlen(string))]);
}

/*
 * initialize_ring_gemstones:
 *	Initialize the ring stone setting scheme for this time
 */
void
initialize_ring_gemstones(void)
{
	unsigned int i, j;
	bool used[NSTONES];

	for (i = 0; i < NSTONES; i++)
		used[i] = FALSE;
	for (i = 0; i < MAXRINGS; i++)
	{
		do
			j = random_below(NSTONES);
		while (used[j]);
		used[j] = TRUE;
		ring_gemstones[i] = stones[j].name;
		ring_identified[i] = FALSE;
		ring_labels[i] = (char *)&item_label_storage[next_item_label++];
		if (i > 0)
			ring_definitions[i].probability += ring_definitions[i-1].probability;
		ring_definitions[i].value += stones[j].value;
	}
}

/*
 * initialize_wand_materials:
 *	Initialize the construction materials for wands and staffs
 */
void
initialize_wand_materials(void)
{
	unsigned int i, j;
	register char *text;
	bool metused[NMETAL], woodused[NWOOD];

	for (i = 0; i < NWOOD; i++)
		woodused[i] = FALSE;
	for (i = 0; i < NMETAL; i++)
		metused[i] = FALSE;
	for (i = 0; i < MAXSTICKS; i++)
	{
		for (;;)
			if (random_below(2) == 0)
			{
				j = random_below(NMETAL);
				if (!metused[j])
				{
					wand_kinds[i] = "wand";
					text = metal_wand_materials[j];
					metused[j] = TRUE;
					break;
				}
			}
			else
			{
				j = random_below(NWOOD);
				if (!woodused[j])
				{
					wand_kinds[i] = "staff";
					text = wooden_wand_materials[j];
					woodused[j] = TRUE;
					break;
				}
			}
		wand_materials[i] = text;
		wand_identified[i] = FALSE;
		wand_labels[i] = (char *)&item_label_storage[next_item_label++];
		if (i > 0)
			wand_definitions[i].probability += wand_definitions[i-1].probability;
	}
}

/*
 * Declarations for allocated things
 */
long *experience_thresholds;		/* Pointer to array of experience level */
char *combat_name_buffer;			/* Temp buffer used in fighting */
char *message_buffer;		/* Message buffer for show_message() */
char *description_buffer;		/* Printing buffer used everywhere */
char *ring_bonus_buffer;		/* Buffer used by ring code */
//@ Deprecated:
//@ char *end_mem;	/* Pointer to end of memory */


/*
 *  Declarations for data space that must be saved and restored exaxtly
 */
byte *terrain_map;
byte *cell_flags;

/*
 * allocate_game_state()
 *   Allocate things data space
 */
void
allocate_game_state(void)
{
	register long *threshold;

	/*@
	 * Do not change the relation between the allocated pointer and its
	 * associated size constant! If the sizes need to be changed, do so by
	 * altering the value in the #define'd constant. For example, msgbuf is
	 * expected to have a BUFSIZE size, but BUFSIZE can be re-#define'd to
	 * another value. Also, for safety, never decrease its value.
	 */

	//@ data that is saved to and restored from saved game files:
	cell_flags = (byte *) allocate_memory((MAXLINES-3)*MAXCOLS);
	terrain_map = (byte *) allocate_memory((MAXLINES-3)*MAXCOLS);
	entity_pool = (Entity *)allocate_memory(sizeof(Entity) * MAXITEMS);
	entity_slot_used = (int *)allocate_memory(MAXITEMS*sizeof(int));

	//@ data discarded and re-created on new and restored games:
	combat_name_buffer = allocate_memory(MAXSTR);
	message_buffer = allocate_memory(BUFSIZE);
	description_buffer = allocate_memory(MAXSTR);
	ring_bonus_buffer = allocate_memory(6);
	experience_thresholds = (long *)allocate_memory(20 * sizeof (long));
	for (threshold = experience_thresholds+1, *experience_thresholds = 10L; threshold < experience_thresholds + 19; threshold++)
		*threshold = *(threshold-1) << 1;
	*threshold = 0L;
}


void
free_game_state(void)
{
	free(cell_flags);
	free(terrain_map);
	free(entity_pool);
	free(entity_slot_used);
	free(combat_name_buffer);
	free(message_buffer);
	free(description_buffer);
	free(ring_bonus_buffer);
	free(experience_thresholds);
}
