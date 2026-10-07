/*
 * Rogue definitions and variable declarations
 *
 * rogue.h	1.4 (AI Design) 12/14/84
 */


#include "platform.h"

/*
 *  Options set for PC rogue
 */

/*
 * copy protection
 */
#define ENABLE_COPY_PROTECTION_CHECKS
#define EXPECTED_CODE_CHECKSUM	-1632
#ifdef ENABLE_COPY_PROTECTION_CHECKS
#define UNAUTHENTICATED_DAMAGE_MULTIPLIER 6
#else
#define UNAUTHENTICATED_DAMAGE_MULTIPLIER 1
#endif //ENABLE_COPY_PROTECTION_CHECKS

/*
 * if DEBUG or WIZARD is changed
 * might as well recompile everything
 */
#define HELP
#ifdef ROGUE_DEMO
#define DEMO
#else
#undef DEMO
#endif
#define DEMOTIME 10
/*
 * DEMO
 *      recompile:
 *          save.c
 *	    rip.c
 *          io.c
 *          main.c
 */
#define REV 1
#define VER 48

/*
 * If CODECSUM is changed recompile extern.c
 */
#define SCOREFILE "rogue.scr"
#define SAVEFILE  "rogue.sav"
#define ENVFILE	  "rogue.opt"
#define IBM
#define MACROSZ 41

#define message_by_verbosity0 message_by_verbosity
#define message_by_verbosity1 message_by_verbosity
#define message_by_verbosity2 message_by_verbosity
#define message_by_verbosity3 message_by_verbosity
#define message_by_verbosity4 message_by_verbosity

/*
 * Maximum number of different things
 */
#define MAXROOMS	9
#define MAXTHINGS	9
#define MAXOBJ		9
#define MAXPACK		23
#define MAXTRAPS	10
#define AMULETLEVEL	26
#define	NUMTHINGS	7	/* number of types of things */
#define MAXPASS		13	/* upper limit on number of passages */
#define MAXNAME		20  /* Maximum Length of a scroll */
#define MAXITEMS	83  /* Maximum number of randomly generated things */
#define BUFSIZE		128 /*@ moved from curses.h */

/*
 * All the fun defines
 */
#define shint		int		/* short integer (for very small #s) */
#define when		break;case
#define otherwise	break;default
#define until(expr)	while(!(expr))
#define next(ptr)	(*ptr).next_entity
#define prev(ptr)	(*ptr).previous_entity
#ifdef UNIX
#define visible_entity_at(y,x)	(monster_at(y,x) != NULL ? monster_at(y,x)->actor_disguise : terrain_at(y,x))
#define distance_squared(y1,x1,y2,x2) (((x2)-(x1))*((x2)-(x1))+((y2)-(y1))*((y2)-(y1)))
#endif
#ifdef UNIX
#define positions_equal(a,b)		((a).x == (b).x && (a).y == (b).y)
#else
#define positions_equal(a,b)		compare_positions(&(a),&(b))
#endif
#define player_position		player.actor_position
#define player_stats		player.actor_stats
#define player_inventory		player.actor_inventory
#define player_room		player.actor_room
#define player_max_hit_points		player.actor_stats.max_hit_points
#define attach(a,b)	list_attach(&a,b)
#define detach(a,b)	list_detach(&a,b)
#define free_list(a)	list_free(&a)
#define max(a,b)	((a) > (b) ? (a) : (b))
#define has_actor_flag(entity,flag)	(((entity).actor_flags & (flag)) != 0)
#define GOLDCALC	(random_below(50 + 10 * dungeon_level) + 2)
#define hand_has_ring(h,r)	(equipped_rings[h] != NULL && equipped_rings[h]->item_subtype == r)
#define wearing_ring(r)	(hand_has_ring(LEFT, r) || hand_has_ring(RIGHT, r))
#define is_stackable_category(type) 	(type==POTION || type==SCROLL || type==FOOD || type==GOLD)
#define terrain_at(y,x)	(terrain_map[map_index(y,x)])
#define cell_flags_at(y,x)	(cell_flags[map_index(y,x)])
#define position_yx(cp)		(cp).y, (cp).x
#define is_floor_tile(c)	((c) == FLOOR || (c) == PASSAGE)
#define is_passage_room(rp)	(((rp)->flags&ROOM_ABSENT) && ((rp)->flags&ROOM_MAZE) == 0)
#ifdef WIZARD
#define debug		if (wizard) show_message
#endif
/*@
 * And some new fun defines...
 */
#define is_monster_symbol(ch)	(((ch) >= 'A') && ((ch) <= 'Z'))

/*
 * Various constants
 */
#define BEARTIME	randomize_duration(3)
#define SLEEPTIME	randomize_duration(5)
#define HEALTIME	randomize_duration(30)
#define HOLDTIME	randomize_duration(2)
#define WANDERTIME	randomize_duration(70)
#define HUHDURATION	randomize_duration(20)
#define SEEDURATION	randomize_duration(300)
#define HUNGERTIME	randomize_duration(1300)
#define MORETIME	150
#define STOMACHSIZE	2000
#define STARVETIME	850
#define LEFT		0
#define RIGHT		1
#define BOLT_LENGTH	6
#define LAMPDIST	3

/*
 * Save against things
 */
#define VS_POISON	00
#define VS_PARALYZATION	00
#define VS_LUCK		01
#define VS_DEATH	00
#define VS_BREATH	02
#define VS_MAGIC	03

/*
 * Various flag bits
 */
/* flags for rooms */
#define ROOM_DARK	 0x0001		/* room is dark */
#define ROOM_ABSENT	 0x0002		/* room is gone (a corridor) */
#define	ROOM_MAZE	 0x0004		/* room is a maze */

/* flags for objects */
#define ITEM_CURSED 0x0001		/* object is cursed */
#define ITEM_IDENTIFIED	 0x0002		/* player knows details about the object */
#define ITEM_VORPAL_FLASHED 0x0004		/* has the vorpal weapon flashed */
#define ITEM_LEGACY_VORPAL_FLAG	 0x0008	/* Set on vorpalization; no reader in this source. */
#define ITEM_THROWABLE	 0x0010		/* object is a missile type */
#define ITEM_STACKABLE	 0x0020		/* object comes in groups */
#define ITEM_SLAYER_REVEALED 0x0040		/* Do you know who the enemy of the object is */

/* flags for creatures */
#define ACTOR_BLIND	 0x0001		/* creature is blind */
#define ACTOR_DETECTS_MONSTERS 0x0002		/* hero can detect unseen monsters */
#define ACTOR_CHASING	 0x0004		/* creature is running at the player */
#define ENTITY_ENCOUNTERED	 0x0008	/* Latched after monster encounter or item pickup. */
#define ACTOR_INVISIBLE	 0x0010		/* creature is invisible */
#define ACTOR_AGGRESSIVE	 0x0020		/* creature can wake when player enters room */
#define ACTOR_GREEDY	 0x0040		/* creature runs to protect gold */
#define ACTOR_HELD	 0x0080		/* creature has been held */
#define ACTOR_CONFUSED	 0x0100		/* creature is confused */
#define ACTOR_REGENERATES	 0x0200		/* creature can regenerate */
#define ACTOR_CAN_CONFUSE	 0x0400		/* creature can confuse */
#define ACTOR_SEES_INVISIBLE	 0x0800		/* creature can see invisible creatures */
#define ACTOR_CANCELLED	 0x1000		/* creature has special qualities cancelled */
#define ACTOR_SLOWED	 0x2000		/* creature has been slowed */
#define ACTOR_HASTED	 0x4000		/* creature has been hastened */
#define ACTOR_FLIES	 0x8000		/* creature is of the flying type */

/*
 * Flags for level map
 */
#define CELL_PASSAGE		0x040		/* is a passageway */
#define CELL_MAZE		0x020		/* have seen this corridor before */
#define CELL_REVEALED		0x010		/* what you see is what you get */
#define PASSAGE_NUMBER_MASK		0x00f		/* passage number mask */
#define TRAP_TYPE_MASK		0x007		/* trap number mask */

/*
 * Trap types
 */
#define TRAP_TRAPDOOR	00
#define TRAP_ARROW	01
#define TRAP_SLEEP_GAS	02
#define TRAP_BEAR	03
#define TRAP_TELEPORT	04
#define TRAP_DART	05
#define NTRAPS	6

/*
 * Potion types
 */
#define POTION_CONFUSION	0
#define POTION_PARALYSIS	1
#define POTION_POISON	2
#define POTION_GAIN_STRENGTH	3
#define POTION_SEE_INVISIBLE	4
#define POTION_HEALING	5
#define POTION_MONSTER_DETECTION		6
#define	POTION_MAGIC_DETECTION 	7
#define	POTION_GAIN_LEVEL		8
#define POTION_EXTRA_HEALING		9
#define POTION_HASTE		10
#define POTION_RESTORE_STRENGTH	11
#define POTION_BLINDNESS		12
#define POTION_THIRST_QUENCHING		13
#define MAXPOTIONS	14

/*
 * Scroll types
 */
#define SCROLL_CONFUSE_MONSTER	0
#define SCROLL_MAPPING		1
#define SCROLL_HOLD_MONSTERS		2
#define SCROLL_SLEEP		3
#define SCROLL_ENCHANT_ARMOR		4
#define SCROLL_IDENTIFY		5
#define SCROLL_SCARE_MONSTER		6
#define SCROLL_FOOD_DETECTION		7
#define SCROLL_TELEPORT		8
#define SCROLL_ENCHANT_WEAPON		9
#define SCROLL_CREATE_MONSTER	10
#define SCROLL_REMOVE_CURSE	11
#define SCROLL_AGGRAVATE_MONSTERS		12
#define SCROLL_BLANK		13
#define SCROLL_VORPALIZE	14
#define MAXSCROLLS	15

/*
 * Weapon types
 */
#define MACE		0
#define SWORD		1
#define BOW		2
#define ARROW		3
#define DAGGER		4
#define TWO_HANDED_SWORD	5
#define DART		6
#define CROSSBOW	7
#define BOLT		8
#define SPEAR		9
#define FLAME		10	/* fake entry for dragon breath (ick) */
#define MAXWEAPONS	10	/* this should equal FLAME */

/*
 * Armor types
 */
#define LEATHER		0
#define RING_MAIL	1
#define STUDDED_LEATHER	2
#define SCALE_MAIL	3
#define CHAIN_MAIL	4
#define SPLINT_MAIL	5
#define BANDED_MAIL	6
#define PLATE_MAIL	7
#define MAXARMORS	8

/*
 * Ring types
 */
#define RING_PROTECTION	0
#define RING_ADD_STRENGTH	1
#define RING_SUSTAIN_STRENGTH	2
#define RING_SEARCHING	3
#define RING_SEE_INVISIBLE	4
#define RING_ADORNMENT		5
#define RING_AGGRAVATION		6
#define RING_DEXTERITY	7
#define RING_DAMAGE	8
#define RING_REGENERATION		9
#define RING_SLOW_DIGESTION	10
#define RING_TELEPORTATION	11
#define RING_STEALTH	12
#define RING_MAINTAIN_ARMOR	13
#define MAXRINGS	14

/*
 * Rod/Wand/Staff types
 */

#define WAND_LIGHT	0
#define WAND_STRIKING		1
#define WAND_LIGHTNING	2
#define WAND_FIRE		3
#define WAND_COLD		4
#define WAND_POLYMORPH	5
#define WAND_MAGIC_MISSILE	6
#define WAND_HASTE_MONSTER	7
#define WAND_SLOW_MONSTER	8
#define WAND_DRAIN_LIFE	9
#define WAND_NOTHING		10
#define WAND_TELEPORT_AWAY	11
#define WAND_TELEPORT_TO	12
#define WAND_CANCELLATION	13
#define MAXSTICKS	14

/*
 * Now we define the structures and types
 */

/*
 * Help list
 * @ this was unused in original. Now improved and put to good use
 */
struct help_entry {
	byte symbol_text[6];  //@ either (ch) or (ch,sep,ch2) appended with ": "
	char *description;
};

/*
 * Coordinate data type
 */
typedef struct {
	shint x;
	shint y;
} Position;

/*@
 * Data type for strength values and modifiers
 * That's very generous from Rogue devs to allow full 16-bits (uint in 1985)
 * for strength, considering normal play would not get even remotely close to 8
 */
typedef unsigned int Strength;

/*
 * Stuff about magic items
 */

struct item_definition {
	char *name;
	shint probability;
	short value;
};

struct item_label {
	char storage[MAXNAME+1];
};

/*
 * Room structure
 */
struct room {
	Position origin;			/* Upper left corner */
	Position size;			/* Size of room */
	Position gold_position;			/* Where the gold is */
	int gold_amount;			/* How much the gold is worth */
	short flags;			/* Info about the room */
	shint exit_count;			/* Number of exits */
	Position exits[12];			/* Where the exits are */
};

/*
 * Structure describing a fighting being
 */
struct combat_stats {
	Strength strength;			/* Strength */
	long experience;				/* Experience */
	shint experience_level;			/* Level of mastery */
	shint armor_class;			/* Armor class */
	shint hit_points;			/* Hit points */
	char *damage_dice;			/* String describing damage done */
	shint max_hit_points;			/* Max hit points */
};

/*
 * Structure for monsters and player
 */
union entity {
	struct {
	union entity *next, *previous;	/* Next pointer in link */
	Position position;			/* Position */
	char move_this_turn;			/* If slowed, is it a turn to move */
	char species;			/* What it is */
	byte disguise;		/* What mimic looks like */
	byte previous_tile;			/* Character that was where it was */
	Position *destination;			/* Where it is running to */
	short flags;			/* State word */
	struct combat_stats stats;		/* Physical description */
	struct room *room;		/* Current room for thing */
	union entity *inventory;		/* What the thing is carrying */
	} actor_data;
	struct {
	union entity *next, *previous;	/* Next pointer in link */
	shint category;			/* What kind of object it is */
	Position position;			/* Where it lives on the screen */
	char *text;			/* What it says if you read it */
	char launcher;			/* What you need to launch it */
	char *melee_damage;		/* Damage if used like sword */
	char *thrown_damage;		/* Damage if thrown */
	shint quantity;			/* Count for plural objects */
	shint subtype;			/* Which object of a type it is */
	shint hit_bonus;			/* Plusses to hit */
	shint damage_bonus;			/* Plusses to damage */
	short modifier;			/* Armor class, ring bonus, wand charges, or gold. */
	short flags;			/* Information about objects */
	char slays_species;			/* If it is enchanted, who it hates */
	shint stack_group;			/* Group number for this object */
	} item_data;
};

typedef union entity Entity;

#define next_entity		actor_data.next
#define previous_entity		actor_data.previous
#define actor_position		actor_data.position
#define actor_move_this_turn		actor_data.move_this_turn
#define actor_species		actor_data.species
#define actor_disguise	actor_data.disguise
#define actor_previous_tile		actor_data.previous_tile
#define actor_destination		actor_data.destination
#define actor_flags		actor_data.flags
#define actor_stats		actor_data.stats
#define actor_inventory		actor_data.inventory
#define actor_room		actor_data.room
#define item_category		item_data.category
#define item_position		item_data.position
#define item_text		item_data.text
#define item_launcher	item_data.launcher
#define item_melee_damage	item_data.melee_damage
#define item_thrown_damage	item_data.thrown_damage
#define item_quantity		item_data.quantity
#define item_subtype		item_data.subtype
#define item_hit_bonus		item_data.hit_bonus
#define item_damage_bonus		item_data.damage_bonus
#define item_modifier		item_data.modifier
#define item_charges	item_modifier
#define item_gold_amount	item_modifier
#define item_flags		item_data.flags
#define item_stack_group		item_data.stack_group
#define item_slays_species		item_data.slays_species

/*
 * Array containing information on all the various types of monsters
 */
struct monster_definition {
	char *name;			/* What to call the monster */
	shint carry_probability;			/* Probability of carrying something */
	unsigned short flags;			/* Things about the monster */
	struct combat_stats stats;		/* Initial stats */
};

/*
 * External variables
 * @ all in extern.c unless noted (init.c, env.c, croot.c, main.c, protect.c)
 */
extern int peak_entity_count;
extern int dungeon_bottom_row;
extern int status_layout_dirty;
extern int version_major, version_minor;
extern int is_me;
extern int next_item_label;
extern bool pending_trapdoor_fall;

// Shared message fragments.
extern char empty_string[], *pronoun_it, *pronoun_you, *out_of_memory_message;

extern char *scroll_labels[], *potion_labels[], *ring_labels[], *wand_labels[];
extern char flytrap_damage_dice[];

extern bool carrying_amulet, turn_consumed, repeating_command, door_stop, expert, auto_run_enabled, scroll_lock_run_enabled,
			first_run_step, score_disabled, playing, running, remember_message, saw_amulet, terse;

//@ originally a bool. See extern.c, move.c, misc.c
extern unsigned char trap_display_state;
#ifdef WIZARD
bool wizard;
#endif

extern bool potion_identified[], ring_identified[], scroll_identified[], wand_identified[];

extern char *armor_names[], *vorpal_flash_message, *rank_names[], previous_message[],
		*vorpal_flash_intensity, *potion_colors[], *ring_gemstones[], run_direction, *pending_macro_input, pickup_symbol,
		*weapon_names[], *wand_materials[], *wand_kinds[];

extern struct help_entry command_help[], symbol_help[];
extern const size_t rank_name_count;

extern int	armor_probabilities[], armor_classes[], command_repeat_count, initial_random_seed, food_remaining,
		flytrap_damage, next_stack_group, hunger_state, inventory_count,
		dungeon_level, deepest_level, message_column, incapacitated_turns, levels_without_food, immobile_turns,
		trap_count, player_gold, healing_turns, allocated_entity_count;

extern long random_state;

//@ related to copy protection
extern int incoming_damage_multiplier;
extern char *tombstone_player_name, *tombstone_death_cause;
extern int disk_authentication_marker;
extern char *unused_player_name;  // Declared and defined, with no callers or readers in this source.
extern int expected_code_checksum;

extern Entity *equipped_armor, *equipped_rings[], *equipped_weapon,
		*level_items, *level_monsters, player;

extern Position	action_direction, previous_player_position;

extern struct room	*previous_player_room, passages[], rooms[];

extern struct combat_stats	maximum_player_stats;

extern struct monster_definition	monster_definitions[];

extern struct item_definition	potion_definitions[], ring_definitions[], scroll_definitions[],
				item_category_probabilities[], wand_definitions[];

extern struct item_label scroll_titles[], item_label_storage[];

#ifdef LOG
extern int captains_log;
#endif //LOG

/*@
 * Definition commented dos_write_port:
 * extern bool askme, fight_flush, jump, passgo, slow_invent;
 * extern char *release;
 *
 * Not found:
 * extern bool in_shell;
 * extern char file_name[], home[], outbuf[];
 * extern int lastscore;
 */


//@ env.c
extern char menu_option[], s_fruit[], score_filename[], save_filename[], s_macro[];
extern char copy_protection_drive[], screen_option[];
extern char favorite_fruit[], keyboard_macro[], player_name[];
// The archived s_name declaration has no matching definition in the imported sources.


//@ init.c
extern char *combat_name_buffer, *description_buffer;
extern byte *terrain_map, *cell_flags;
extern long *experience_thresholds;
extern char *message_buffer;
extern Entity *entity_pool;
extern int   *entity_slot_used;
extern char *ring_bonus_buffer;
//@ extern char *_top, *_base;  //@ not found
/*@
 * Deprecated:
 * extern char *end_mem;
 */


//@ protect.c
extern int protection_watchdog_ticks;  //@ used in update_protection_state(), originally set by dos.asm


/*
 * Function types
 *
 * @ curses.c has its own header
 * @ mach_dep.c functions are declared in extern.h
 */

//@ armor.c
void	wear_armor(void);
void	remove_armor(void);
void	advance_turn(void);

//@ chase.c
void	move_monsters(void);
void	move_chasing_monster(Entity *monster);
void	choose_chase_step(Entity *monster, Position *destination);
void	start_monster_chase(Position *runner);
bool	player_can_see_monster(Entity *monster);
bool	diagonal_move_allowed(Position *start_position, Position *end_position);
bool	player_can_see_position(int y, int x);
struct room	*room_at(Position *position);
Position	*choose_monster_destination(Entity *entity);

//@ command.c
void	process_turn(void);
void	show_repeat_count(void);
void	execute_command(void);

//@ daemon.c
void	schedule_recurring_action(void (*func)());
void	run_recurring_actions(void);
void	schedule_delayed_action(void (*func)(), int time);
void	extend_delayed_action(void (*func)(), int xtime);
void	cancel_delayed_action(void (*func)());
void	run_delayed_actions(void);

//@ daemons.c
void	regenerate_health(void);
void	start_wander_checks(void);
void	check_wandering_spawn(void);
void	end_confusion(void);
void	end_monster_detection(void);
void	end_blindness(void);
void	end_haste(void);
void	consume_food(void);

//@ env.h
bool	load_options_file(char *filename);

//@ fakedos.c
void	show_fake_dos(void);

//@ fight.c
bool	player_attack(Position *monster_position, char monster_symbol, Entity *weapon, bool thrown);
bool	attack_hits(int attacker_level, int defender_armor, int hit_bonus);
bool	resolve_attack_damage(Entity *attacker, Entity *defender, Entity *weapon, bool thrown);
bool	actor_saving_throw(int which, Entity *entity);
bool	player_saving_throw(int which);
bool	is_magic(Entity *item);
void	monster_attack(Entity *monster);
void	check_experience_level(void);
void	report_hit(char *attacker_name, char *defender_name);
void	report_miss(char *attacker_name, char *defender_name);
void	gain_experience_level(void);
void	report_projectile_hit(Entity *weapon, char *monster_name, char *present_verb, char *past_verb);
void	remove_monster(Position *position, Entity *monster, bool killed);
void	kill_monster(Entity *monster, bool print_message);
int	strength_hit_bonus(Strength strength);
int	strength_damage_bonus(Strength strength);

//@ init.c
void	init_player(void);
void	initialize_item_probabilities(void);
void	initialize_potion_colors(void);
void	initialize_scroll_titles(void);
void	initialize_ring_gemstones(void);
void	initialize_wand_materials(void);
void	allocate_game_state(void);
void	free_game_state(void);
char	*random_syllable(void);
char	random_character(char *string);

//@ io.c
void	message_by_verbosity(const char *tfmt, const char *format, ...);
void	show_message(const char *format, ...);
void	show_message_v(const char *format, va_list arguments);
void	append_message(const char *format, ...);
void	append_message_v(const char *format, va_list arguments);
void	wait_for_enter(const char *message_text);
void	finish_message(void);
void	show_more_prompt(char *message_text);
void	display_wrapped_message(int message_row, char *message_text);
void	display_message_segment(int message_row, char *scroll_start, char *scroll_end);
void	update_status_line(void);
void	wait_for_key(byte character);
void	show_overlay_message(char *message);
void	print_highlighted_text(char *text);
void	update_keyboard_and_clock(void);
char	*describe_key(byte character);
char	*verbose_text(char *text);

//@ list.c
Entity	*allocate_entity(void);
void	list_detach(Entity **list, Entity *item);
void	list_attach(Entity **list, Entity *item);
void	list_free(Entity **list_head);
int	release_entity(Entity *item);

//@ load.c
void	show_dos_splash(void);
int	find_copy_protection_drive(void);

#ifdef ROGUE_SPLASH
//@ load_sdl.c - not in original
int	show_sdl_splash(const char* path);
#endif //ROGUE_SPLASH

//@ main.c
void	exit_game_message(void);
void	run_game(char *saved_game_path);
void	quit(void);
void	leave(void);
int	random_below(int range);
int	roll_dice(int number, int sides);

//@ maze.c
void	draw_maze(struct room *room);
void	expand_maze_frontier(int y, int x);
void	add_maze_frontier(int y, int x);
void	connect_maze_frontier(void);
void	carve_maze_cell(int y, int x);
bool	is_maze_passage(int y, int x);
bool	inside_maze_bounds(int y, int x);

//@ misc.c
void	update_player_view(bool wakeup);
void	eat_food(void);
void	change_player_strength(int adjustment);
void	adjust_strength(Strength *strength, int adjustment);
void	aggravate_monsters(void);
void	prompt_item_label(bool identified, char **label);
void	show_help(struct help_entry *entries);
void	search(void);
void	descend_stairs(void);
void	ascend_stairs(void);
void	name_item_type(void);
void	edit_keyboard_macro(char *buffer, int capacity);
Entity	*item_at(int y, int x);
bool	add_haste(bool potion);
bool	is_equipped(Entity *item);
bool	read_direction(void);
bool	decode_direction(byte character, Position *direction);
bool	is_walkable_symbol(byte character);
bool	compare_positions(Position *a, Position *b);
bool	outside_dungeon(int y, int x);
char	*trap_name(byte type);
char	*article_suffix(char *text);
char	display_item_symbol(Entity *item);
shint	sign(int value);
byte	visible_entity_at(int y, int x);
int	randomize_duration(int base_duration);
int	distance_squared(int y1, int x1, int y2, int x2);
int	map_index(int y, int x);
#ifdef ME
bool	me(void);
#endif
#ifdef TEST
bool	istest(void);
#endif

//@ monsters.c
char	random_monster_species(bool wander);
char	random_vorpal_enemy(void);
void	new_monster(Entity *monster, byte type, Position *spawn_position);
void	reset_flytrap_damage(void);
void	spawn_wandering_monster(void);
void	give_monster_item(Entity *monster);
Entity	*wake_monster(int y, int x);
Entity	*monster_at(int y, int x);

//@ move.c
void	start_player_run(byte character);
void	move_player(int dy, int dx);
void	wake_room_monsters(struct room *room);
void	fall_to_next_level(char *message_text);
void	random_move(Entity *actor, Position *destination);

//@ new_leve.c
void	generate_level(void);
void	populate_level_items(void);
int	random_room_index(void);

//@ pack.c
Entity	*select_inventory_item(char *purpose, int type);
void	add_to_inventory(Entity *item, bool silent);
void	pick_up_item(byte character);
void	collect_gold(int value);
byte	show_inventory(Entity *items, int type, char *line_prefix);
byte	inventory_key(Entity *target_item);

//@ passages.c
void	connect_rooms(int first_room_index, int second_room_index);
void	generate_passages(void);
void	place_door(struct room *room, Position *position);
void	number_passages(void);
void	mark_connected_passage(int y, int x);
void	carve_passage_cell(shint y, shint x);

//@ potions.c
void	drink_potion(void);
void	reveal_invisible_monsters(void);
void	apply_thrown_potion(Entity *item, Entity *target);
bool	set_monster_detection(bool turn_off);

//@ protect.c
#ifndef ROGUE_ORIGINAL_COPY_PROTECTION
void	authenticate_game_disk(int UNUSED(drive));
#else
void	authenticate_game_disk(int drive);
#endif

//@ rings.c
void	put_on_ring(void);
void	remove_ring(void);
char	*format_ring_bonus(Entity *item);
int	ring_food_cost();

//@ rip.c
void	update_high_scores(int amount, int end_reason, char death_cause);
void	show_death_screen(char death_cause);
void	show_victory_screen(void);
char	*death_cause_name(byte cause, bool include_article);
#ifdef DEMO
void	demo(int endtype);
#endif //DEMO

//@ rooms.c
void	generate_rooms(void);
void	draw_room(struct room *room);
void	random_room_position(struct room *room, Position *position);
void	enter_room(Position *position);
void	leave_room(Position *position);

//@ save.c
void	save_game(void);
void	restore_game(char *savefile);

//@ scrolls.c
void read_scroll(void);

//@ slime.c
void	slime_split(Entity *slime);
bool	find_monster_spawn_position(int origin_y, int origin_x, Position *spawn_position);

//@ sticks.c
void	initialize_wand(Entity *wand);
void	zap_wand(void);
void	drain_monsters(void);
void	fire_bolt(Position *start, Position *direction, char *name);
char	*format_wand_charges(Entity *item);

//@ strings.c
bool	is_alpha(char character);
bool	is_upper(char character);
bool	is_lower(char character);
bool	is_digit(char character);
bool	is_space(char character);
bool	is_print(char character);
char	*copy_string_bounded(char *destination, char *source, int max_characters);
char	*skip_whitespace(char *text);
char	*trim_trailing_whitespace(char *text);
void	lowercase_string(char *text);

//@ things.c
char	*describe_item(Entity *item, bool dropped);
void	drop_item(void);
void	show_discoveries(void);
bool	can_drop(Entity *item);
Entity	*generate_item(void);
byte	add_line(char *use, char *format, char *arg);
byte	end_line(char *use);

//@ weapons.c
void	throw_item(int ydelta, int xdelta);
void	animate_projectile(Entity *item, int ydelta, int xdelta);
void	drop_projectile(Entity *item, bool print_message);
void	init_weapon(Entity *weapon, byte type);
void	wield(void);
void	tick_pause(void);
char	*format_item_bonus(int n1, int n2, char type);
bool	hit_monster(int y, int x, Entity *item);

//@ wizard.c
void	identify_item(void);
int	teleport(void);
#ifdef WIZARD
void	create_obj();
#endif //WIZARD


/*@ functions declared but not found
int	auto_save();
int	tstp();
Entity	*find_mons();
char	*balloc();
 */
