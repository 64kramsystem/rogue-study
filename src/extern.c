/*
 * global variable initializaton
 *
 * @(#)extern.c	5.2 (Berkeley) 6/16/82
 */

#include "rogue.h"
#include "curses.h"

#ifdef LOG
int captains_log = FALSE;
#endif //LOG
#ifdef SDEBUG
int is_com;
#endif //SDEBUG
/*
 * revno: current revision level
 * verno: current version of a particular rev
 */
int version_major = REV;
int version_minor = VER;

/*
 * All this should be low as possible in memory so that
 * we can save the min
 */
char *weapon_names[MAXWEAPONS + 1] = {	/* Names of the various weapons */
	"mace",
	"long sword",
	"short bow",
	"arrow",
	"dagger",
	"two handed sword",
	"dart",
	"crossbow",
	"crossbow bolt",
	"spear",
	NULL				/* fake entry for dragon's breath */
};
char *armor_names[MAXARMORS] = {		/* Names of armor types */
	"leather armor",
	"ring mail",
	"studded leather armor",
	"scale mail",
	"chain mail",
	"splint mail",
	"banded mail",
	"plate mail"
};

int armor_probabilities[MAXARMORS] = {		/* Chance for each armor type */
	20,
	35,
	50,
	63,
	75,
	85,
	95,
	100
};
int armor_classes[MAXARMORS] = {		/* Armor class for each armor type */
	8,
	7,
	7,
	6,
	5,
	4,
	4,
	3
};

struct item_definition scroll_definitions[MAXSCROLLS] = {
	{ "monster confusion",	 8, 140 },
	{ "magic mapping",		 5, 150 },
	{ "hold monster",		 3, 180 },
	{ "sleep",			 5,   5 },
	{ "enchant armor",		 8, 160 },
	{ "identify",		27, 100 },
	{ "scare monster",		 4, 200 },
	{ "food detection",		 4,  50 },
	{ "teleportation",		 7, 165 },
	{ "enchant weapon",		10, 150 },
	{ "create monster",		 5,  75 },
	{ "remove curse",		 8, 105 },
	{ "aggravate monsters",	 4,  20 },
	{ "blank paper",		 1,   5 },
	{ "vorpalize weapon",	 1, 300 }
};

struct item_definition potion_definitions[MAXPOTIONS] = {
	{ "confusion",		 8,   5 },
	{ "paralysis",		10,   5 },
	{ "poison",			 8,   5 },
	{ "gain strength",		15, 150 },
	{ "see invisible",		 2, 100 },
	{ "healing",		15, 130 },
#ifdef DEMO
	{ "advertisement",           6, 130 },
#else
	{ "monster detection",	 6, 130 },
#endif //DEMO
	{ "magic detection",	 6, 105 },
	{ "raise level",		 2, 250 },
	{ "extra healing",		 5, 200 },
	{ "haste self",		 4, 190 },
	{ "restore strength",	14, 130 },
	{ "blindness",		 4,   5 },
	{ "thirst quenching",	 1,   5 }
};

struct item_definition ring_definitions[MAXRINGS] = {
	{ "protection",		 9, 400 },
	{ "add strength",		 9, 400 },
	{ "sustain strength",	 5, 280 },
	{ "searching",		10, 420 },
	{ "see invisible",		10, 310 },
	{ "adornment",		 1,  10 },
	{ "aggravate monster",	10,  10 },
	{ "dexterity",		 8, 440 },
	{ "increase damage",	 8, 400 },
	{ "regeneration",		 4, 460 },
	{ "slow digestion",		 9, 240 },
	{ "teleportation",		 5,  30 },
	{ "stealth",		 7, 470 },
	{ "maintain armor",		 5, 380 }
};

struct item_definition wand_definitions[MAXSTICKS] = {
	{ "light",			12, 250 },
	{ "striking",		 9,  75 },
	{ "lightning",		 3, 330 },
	{ "fire",			 3, 330 },
	{ "cold",			 3, 330 },
	{ "polymorph",		15, 310 },
	{ "magic missile",		10, 170 },
	{ "haste monster",		 9,   5 },
	{ "slow monster",		11, 350 },
	{ "drain life",		 9, 300 },
	{ "nothing",		 1,   5 },
	{ "teleport away",		 5, 340 },
	{ "teleport to",		 5,  50 },
	{ "cancellation",		 5, 280 }
};

#ifdef HELP
/*@
 * Original code used CP437 codes hard coded inside the help strings,
 * instead of the #define'd char constants for FLOOR, PLAYER etc.
 * To support the constants, H_*() macros were created and helpcoms/helpobjs
 * array type has changed from string to struct h_list.
 *
 * Ironically, struct h_list already existed in rogue.h, but it was unused in
 * code, so perhaps original authors either abandoned the idea or were halfway
 * through implementing it.
 */
#define H_STR(str)	{"", str}
#define H_CHSTR(ch, str)	{{ch, ':', ' ', '\0'}, str}
#define H_CH2STR(ch1, ch2, sep, str)	{{ch1, sep, ch2, ':', ' ', '\0'}, str}
#define H_END	{"", ""}
struct help_entry command_help[] = {
	H_STR("F1     list of commands"),
	H_STR("F2     list of symbols"),
	H_STR("F3     repeat command"),
	H_STR("F4     repeat message"),
	H_STR("F5     rename something"),
	H_STR("F6     recall what's been discovered"),
	H_STR("F7     inventory of your possessions"),
	H_STR("F8     <dir> identify trap type"),
	H_STR("F9     The Any Key (definable)"),
	H_STR("Alt F9 defines the Any Key"),
	H_STR("F10    Supervisor Key (fake dos)"),
	H_STR("Space  Clear -More- message"),
	H_STR("\x11\xd9     the Enter Key"),
	H_STR("\x1b      left"),
	H_STR("\x19      down"),
	H_STR("\x18      up"),
	H_STR("\x1a      right"),
	H_STR("Home   up & left"),
	H_STR("PgUp   up & right"),
	H_STR("End    down & left"),
	H_STR("PgDn   down & right"),
	H_STR("Scroll Fast Play mode"),
	H_STR(".      rest"),
	H_STR(">      go down a staircase"),
	H_STR("<      go up a staircase"),
	H_STR("Esc    cancel command"),
	H_STR("d      drop object"),
	H_STR("e      eat food"),
	H_STR("f      <dir> find something"),
	H_STR("q      quaff potion"),
	H_STR("r      read paper"),
	H_STR("s      search for trap/secret door"),
	H_STR("t      <dir> throw something"),
	H_STR("w      wield a weapon"),
	H_STR("z      <dir> zap with a wand"),
	H_STR("B      run down & left"),
	H_STR("H      run left"),
	H_STR("J      run down"),
	H_STR("K      run up"),
	H_STR("L      run right"),
	H_STR("N      run down & right"),
	H_STR("U      run up & right"),
	H_STR("Y      run up & left"),
	H_STR("W      wear armor"),
	H_STR("T      take armor off"),
	H_STR("P      put on ring"),
	H_STR("Q      quit"),
	H_STR("R      remove ring"),
	H_STR("S      save game"),
	H_STR("^      identify trap"),
	H_STR("?      help"),
	H_STR("/      key"),
	H_STR("+      throw"),
	H_STR("-      zap"),
	H_STR("Ctrl t terse message format"),
	H_STR("Ctrl r repeat message"),
	H_STR("Del    search for something hidden"),
	H_STR("Ins    <dir> find something"),
	H_STR("a      repeat command"),
	H_STR("c      rename something"),
	H_STR("i      inventory"),
	H_STR("v      version number"),
	H_STR("!      Supervisor Key (fake DOS)"),
	H_STR("D      list what has been discovered"),
	H_END
};

struct help_entry symbol_help[] = {
	H_CHSTR(FLOOR,   "the floor"),
	H_CHSTR(PLAYER,  "the hero"),
	H_CHSTR(FOOD,    "some food"),
	H_CHSTR(AMULET,  "the amulet of yendor"),
	H_CHSTR(SCROLL,  "a scroll"),
	H_CHSTR(WEAPON,  "a weapon"),
	H_CHSTR(ARMOR,   "a piece of armor"),
	H_CHSTR(GOLD,    "some gold"),
	H_CHSTR(STICK,   "a magic staff"),
	H_CHSTR(POTION,  "a potion"),
	H_CHSTR(RING,    "a magic ring"),
	H_CHSTR(0xB2,    "a passage"),  //@ surprisingly it's not PASSAGE
	/* make sure in 40 or 80 column none of line draw set connects */
	/* this is currently in column 1 for 80 */
	H_CHSTR(DOOR,    "a door"),
	H_CHSTR(ULWALL,  "an upper left corner"),
	H_CHSTR(TRAP,    "a trap"),
	H_CHSTR(HWALL,   "a horizontal wall"),
	H_CHSTR(LRWALL,  "a lower right corner"),
	H_CHSTR(LLWALL,  "a lower left corner"),
	H_CHSTR(VWALL,   "a vertical wall"),
	H_CHSTR(URWALL,  "an upper right corner"),
	H_CHSTR(STAIRS,  "a stair case"),
	H_CH2STR(MAGIC, BMAGIC, ',', "safe and perilous magic"),
	H_CH2STR('A',   'Z',    '-', "26 different monsters"),
	H_END
};
#endif //HELP
/*
 * Names of the various experience levels
 */

char *rank_names[] = {
	"",
	"Guild Novice",
	"Apprentice",
	"Journeyman",
	"Adventurer",
	"Fighter",
	"Warrior",
	"Rogue",
	"Champion",
	"Master Rogue",
	"Warlord",
	"Hero",
	"Guild Master",
	"Dragonlord",
	"Wizard",
	"Rogue Geek",
	"Rogue Addict",
	"Schmendrick",
	"Gunfighter",
	"Time Waster",
	"Bug Chaser"
};
const size_t rank_name_count = sizeof(rank_names) / sizeof(*rank_names);

/*
 * Lattice C compiler funnies
 */
int peak_entity_count = 0;
int status_layout_dirty = FALSE;

bool turn_consumed;				/* True if we want after daemons */
bool score_disabled;				/* Was a wizard sometime */
bool repeating_command;			/* The last command is repeated */
bool scroll_identified[MAXSCROLLS];		/* Does he know what a scroll does */
bool potion_identified[MAXPOTIONS];		/* Does he know what a potion does */
bool ring_identified[MAXRINGS];			/* Does he know what a ring does */
bool wand_identified[MAXSTICKS];		/* Does he know what a stick does */
bool carrying_amulet = FALSE;			/* He has the amulet */
bool saw_amulet = FALSE;	    /* He has seen the amulet */
/* bool askme = TRUE; */			/* Ask about unidentified things */
bool door_stop = FALSE;			/* Stop running when we pass a door */
bool auto_run_enabled = FALSE;			/* Run until you see something */
bool scroll_lock_run_enabled = FALSE;			/* Toggle for find (see above) */
/* bool fight_flush = TRUE;	*/	/* True if toilet input */
bool first_run_step = FALSE;			/* First move after setting door_stop */
/* bool jump = FALSE;	*/		/* Show running as series of jumps */
/* bool passgo = TRUE;	*/		/* Follow passages */
bool playing = TRUE;			/* True until he quits */
bool running = FALSE;			/* True if player is running */
bool remember_message = TRUE;			/* Remember last msg */
/* bool slow_invent = FALSE; */		/* Inventory one line at a time */
bool terse = FALSE;
bool expert = FALSE;
#ifdef ME
int is_me;
#endif
/*@
 * `was_trapped` was originally a bool, which in original code was typedef'd as
 * unsigned char. As it is used in ++ increment and > test, I've reverted it
 * to its original (real) type. See be_trapped() in move.c and look() in misc.c
 */
unsigned char trap_display_state = FALSE;		/* Was a trap sprung */
#ifdef WIZARD
bool wizard = FALSE;			/* True if allows wizard commands */
#endif
bool pending_trapdoor_fall = FALSE;
char pickup_symbol;				/* Thing the rogue is taking */
char run_direction;				/* Direction player is running */
/* now names are associated with fixed pointers */
struct item_label scroll_titles[MAXSCROLLS];			/* Names of the scrolls */
char *potion_colors[MAXPOTIONS];		/* Colors of the potions */
char *ring_gemstones[MAXRINGS];		/* Stone settings of the rings */
char *wand_materials[MAXSTICKS];		/* What sticks are made of */
/* char *release;	*/			/* Release number of rogue */
char previous_message[BUFSIZE];				/* The last message printed */
char *scroll_labels[MAXSCROLLS];		/* Players guess at what scroll is */
char *potion_labels[MAXPOTIONS];		/* Players guess at what potion is */
char *ring_labels[MAXRINGS];		/* Players guess at what ring is */
char *wand_labels[MAXSTICKS];		/* Players guess at what wand is */
/* storage array for guesses */
struct item_label item_label_storage[MAXSCROLLS+MAXPOTIONS+MAXRINGS+MAXSTICKS];
int next_item_label = 0;
char *wand_kinds[MAXSTICKS];		/* Is it a wand or a staff */

int dungeon_bottom_row;			/* Last Line used for map  */
int deepest_level;				/* Deepest player has gone */
int trap_count;				/* Number of traps on this level */
int initial_random_seed;				/* Dungeon number */
int dungeon_level = 1;				/* What level rogue is on */
int player_gold = 0;				/* How much gold the rogue has */
int message_column = 0;				/* Where cursor is on top line */
int immobile_turns = 0;			/* Number of turns held in place */
int incapacitated_turns = 0;			/* Number of turns asleep */
int inventory_count = 0;				/* Number of things in pack */
int allocated_entity_count = 0;				/* Total dynamic memory bytes */
int levels_without_food = 0;			/* Number of levels without food */
int command_repeat_count = 0;				/* Number of times to repeat command */
int flytrap_damage = 0;			/* Number of time fungi has hit */
int healing_turns = 0;				/* Number of quiet turns */
int food_remaining;				/* Amount of food in hero's stomach */
int next_stack_group = 2;				/* Current group number */
int hunger_state = 0;			/* How hungry is he */
int expected_code_checksum = EXPECTED_CODE_CHECKSUM;
long random_state;				/* Random number seed */

int incoming_damage_multiplier = UNAUTHENTICATED_DAMAGE_MULTIPLIER;
int disk_authentication_marker = 1;
char *tombstone_player_name = "Software Pirate";
char *tombstone_death_cause = "Copy Protection Mafia";
char *unused_player_name;

/* WINDOW *hw;				 Used as a scratch window */

Position previous_player_position;				/* Position before last look() call */
Position action_direction;				/* Change indicated to get_dir() */

Entity *equipped_armor;			/* What a well dresssed rogue wears */
Entity *equipped_rings[2];			/* Which rings are being worn */
Entity *equipped_weapon;			/* Which weapon he is weilding */

struct room *previous_player_room;			/* Roomin(&oldpos) */
struct room rooms[MAXROOMS];		/* One for each room -- A level */

#define XX  {0, 0}
#define ___ {XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX} //@ 12 exits
struct room passages[MAXPASS] =		/* One for each passage */
{
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ROOM_ABSENT|ROOM_DARK, 0, ___ }
};
#undef ___
#undef XX


#define INIT_STATS { 16, 0, 1, 10, 12, "1d4", 12 }

struct combat_stats maximum_player_stats = INIT_STATS;	/* The maximum for the player */

Entity player;				/* The rogue */
Entity *level_items = NULL;			/* List of objects on this level */
Entity *level_monsters = NULL;			/* List of monsters on the level */

/*@
 * Original code did not define a value for s_maxhp member of stats struct.
 * s_maxhp from this monster template is unused, just like s_hpt, as its value
 * was randomly chosen for each new generated monster. To make compilers happy,
 * value is now set to a dummy ___ value, the same convention used in original
 * code for s_hpt.
 */
#define ___ 1
#define XX 10
struct monster_definition monster_definitions[26] =
{
	/* Name		 CARRY	FLAG    str, exp, lvl, amr, hpt, dmg, maxhp */
	{ "aquator",	0,	ACTOR_AGGRESSIVE,	{ XX, 20,   5,   2, ___, "0d0/0d0", ___ } },
	{ "bat",	 	0,	ACTOR_FLIES,	{ XX,  1,   1,   3, ___, "1d2", ___ } },
	{ "centaur",	 15,	0,	{ XX, 25,   4,   4, ___, "1d6/1d6", ___ } },
	{ "dragon",	 100,	ACTOR_AGGRESSIVE,	{ XX,6800, 10,  -1, ___, "1d8/1d8/3d10", ___ } },
	{ "emu",	 0,	ACTOR_AGGRESSIVE,	{ XX,  2,   1,   7, ___, "1d2", ___ } },
		/* NOTE: the damage is %%% so that xstr won't merge this */
		/* string with others, since it is written on in the program */
	{ "venus flytrap",0,	ACTOR_AGGRESSIVE,	{ XX, 80,   8,   3, ___, "%%%d0", ___ } },
	{ "griffin",	 20,	ACTOR_AGGRESSIVE|ACTOR_FLIES|ACTOR_REGENERATES,	{XX,2000, 13, 2,___, "4d3/3d5/4d3", ___ } },
	{ "hobgoblin",	 0,	ACTOR_AGGRESSIVE,	{ XX,  3,   1,   5, ___, "1d8", ___ } },
	{ "ice monster", 0,	ACTOR_AGGRESSIVE,	{ XX,  15,   1,   9, ___, "1d2", ___ } },
	{ "jabberwock",  70,	0,	{ XX,4000, 15,   6, ___, "2d12/2d4", ___ } },
	{ "kestral",	 0,	ACTOR_AGGRESSIVE|ACTOR_FLIES, { XX,  1,   1,   7, ___, "1d4", ___ } },
	{ "leprechaun",	 ACTOR_GREEDY,	0,	{ XX, 10,   3,   8, ___, "1d2", ___ } },
	{ "medusa",	 40,	ACTOR_AGGRESSIVE,	{ XX,200,   8,   2, ___, "3d4/3d4/2d5", ___ } },
	{ "nymph",	 100,	0,	{ XX, 37,   3,   9, ___, "0d0", ___ } },
	{ "orc",	 15,	ACTOR_GREEDY,{ XX,  5,   1,   6, ___, "1d8", ___ } },
	{ "phantom",	 0,ACTOR_INVISIBLE,{ XX,120,   8,   3, ___, "4d4", ___ } },
	{ "quagga",	 30,	ACTOR_AGGRESSIVE,	{ XX, 32,   3,   2, ___, "1d2/1d2/1d4", ___ } },
	{ "rattlesnake", 0,	ACTOR_AGGRESSIVE,	{ XX,  9,   2,   3, ___, "1d6", ___ } },
	{ "slime",	 	 0,	ACTOR_AGGRESSIVE,	{ XX,  1,   2,   8, ___, "1d3", ___ } },
	{ "troll",	 50,	ACTOR_REGENERATES|ACTOR_AGGRESSIVE,{ XX, 120, 6, 4, ___, "1d8/1d8/2d6", ___ } },
	{ "ur-vile",	 0,	ACTOR_AGGRESSIVE,	{ XX,190,   7,  -2, ___, "1d3/1d3/1d3/4d6", ___ } },
	{ "vampire",	 20,	ACTOR_REGENERATES|ACTOR_AGGRESSIVE,{ XX,350,   8,   1, ___, "1d10", ___ } },
	{ "wraith",	 0,	0,	{ XX, 55,   5,   4, ___, "1d6", ___ } },
	{ "xeroc",30,	0,	{ XX,100,   7,   7, ___, "3d4", ___ } },
	{ "yeti",	 30,	0,	{ XX, 50,   4,   6, ___, "1d6/1d6", ___ } },
	{ "zombie",	 0,	ACTOR_AGGRESSIVE,	{ XX,  6,   2,   8, ___, "1d8", ___ } }
};
char flytrap_damage_dice[10];
#undef ___
#undef XX

/*@
 * Not to be confused with _things[], which is an array of THINGS on the level
 * This one serves to choose the type of random items. The actual probability
 * is redefined in init_things(), and the only user is new_thing().
 * To make compilers happy, the unused mi_worth is set using ___, as per
 * original code convention.
 */
#define ___ 1
struct item_definition item_category_probabilities[NUMTHINGS] = {
	{ 0,			27, ___ },	/* potion */
	{ 0,			30, ___ },	/* scroll */
	{ 0,			17, ___ },	/* food */
	{ 0,			 8, ___ },	/* weapon */
	{ 0,			 8, ___ },	/* armor */
	{ 0,			 5, ___ },	/* ring */
	{ 0,			 5, ___ }	/* stick */
};
#undef ___

/*
 * Common strings
 */
char empty_string[] = "";
char *pending_macro_input = empty_string;

char *vorpal_flash_intensity = " of intense white light";
char *vorpal_flash_message = "your %s gives off a flash%s";
char *pronoun_it = "it";
char *pronoun_you = "you";
char *out_of_memory_message = "Not enough Memory";
//@ char *smsg = "\r\n*** Stack Overflow ***\r\n$"; //only used in csav.asm
