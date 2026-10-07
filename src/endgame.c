/*
 * File for the fun ends
 * Death or a total win
 *
 * rip.c	1.4 (A.I. Design)	12/14/84
 */

#include "rogue.h"
#include "screen.h"

//@ moved from rogue.h
#define MAX_HIGH_SCORES	10
struct score_entry {
	char player_name[38];
	int experience_level;
	int gold;
	int fate;
	int dungeon_level;
};

#ifndef DEMO
static FILE *score_file;
#endif

static void	read_scores(struct score_entry *scores);
static bool	write_scores(struct score_entry *scores);
static void	display_scores(int new_rank, struct score_entry *scores);
static int	insert_score(struct score_entry *new_score, struct score_entry *scores);

/*
 * update_high_scores:
 *	Figure score and post it.
 */
/* VARARGS2 */
void
update_high_scores(int amount, int end_reason, char death_cause)
{
#ifndef DEMO
#ifndef WIZARD
	struct score_entry player_score = {0}, scores[MAX_HIGH_SCORES];
	register int rank=0;
	char response = ' ';
	bool written = TRUE;


	screen_updates_suspended = TRUE;

	if (amount || end_reason || death_cause)
	{
		wait_for_enter("see rankings");
	}
	while ((score_file = fopen(score_filename, "rb")) == NULL)
	{
		printw("\n");
		if (score_disabled || (amount == 0))
			return;
		print_highlighted_text("No scorefile: %Create %Retry %Abort");
reread:
		switch(response = read_game_key())
		{
		case 'c':
		case 'C':
			score_file = fopen(score_filename, "wb");
			if (score_file == NULL) {
				printw("\nCould not create scorefile: %s\n", strerror(errno));
				break;
			}
			if (fclose(score_file) != 0)
				printw("\nCould not close scorefile: %s\n", strerror(errno));
			break;
		case 'r':
		case 'R':
			break;
		case 'a':
		case 'A':
			return;
		default:
			goto reread;
		}
	}
	printw("\n");
	read_scores(scores);

	if (score_disabled != TRUE)
	{
		strcpy(player_score.player_name,player_name);
		player_score.gold = amount;
		player_score.fate = end_reason ? end_reason : death_cause;
		player_score.dungeon_level = deepest_level;
		player_score.experience_level  = player_stats.experience_level;
		rank = insert_score(&player_score, scores);
	}
	fclose(score_file);
	if (rank > 0) {
		if ((score_file = fopen(score_filename, "wb")) != NULL) {
			written = write_scores(scores);
			if (fclose(score_file) != 0)
				written = FALSE;
		} else
			written = FALSE;
	}
	display_scores(rank, scores);
	if (!written)
		printw("\nCould not write scorefile\n");
#ifndef ROGUE_DOS_CURSES
	wait_for_enter("exit");
	printw("\n");
#endif
#endif //WIZARD
#endif //DEMO
}

#ifndef DEMO
#ifndef WIZARD
static
void
read_scores(struct score_entry *scores)
{
	struct score_entry entry;
	int i;

	memset(scores, 0, MAX_HIGH_SCORES * sizeof(*scores));
	for (i = 0; i < MAX_HIGH_SCORES; i++) {
		if (fread(&entry, sizeof(entry), 1, score_file) != 1)
			break;
		/* Old files store native structs, so validate before indexing or printing. */
		if (entry.gold <= 0 || entry.experience_level < 1
		    || (size_t)entry.experience_level > rank_name_count || entry.dungeon_level < 1
		    || memchr(entry.player_name, '\0', sizeof(entry.player_name)) == NULL)
			break;
		scores[i] = entry;
	}
}

static
bool
write_scores(struct score_entry *scores)
{
	register int i;

	for (i=0;(i<MAX_HIGH_SCORES) && scores->gold;i++,scores++)
	{
		if (fwrite(scores, sizeof(struct score_entry), 1, score_file) <= 0)
			return FALSE;
	}
	return TRUE;
}

static
void
display_scores(int new_rank, struct score_entry *scores)
{
	register int i;
	int row;
	char death_description[30];
	char *special_message;

#ifdef ROGUE_DOS_CURSES
	switch_page(old_page_no);
#endif
	clear();
	high();
	if (dos_screen_mode == 7)
		standout();
	mvaddstr(0,0,"Guildmaster's Hall Of Fame:");
	standend();
	yellow();
	mvaddstr(2,0,"Gold");

	for (i=0;i<MAX_HIGH_SCORES;i++,scores++)
	{
		special_message = NULL;
		brown();
		if (new_rank - 1 == i)
		{
			if (dos_screen_mode == 7)
				standout();
			else
				yellow();
		}
		if (scores->gold <=0 )
			break;
		row = 4 + ((COLS==40)?(i * 2):i);
		move (row,0);
		printw("%d ",scores->gold);
		move (row,6);
		if (new_rank - 1 != i)
			red();
		printw("%s",scores->player_name);
		if ((new_rank) - 1 != i)
			brown();
		if (scores->dungeon_level >= 26)  //@ There is AMULETLEVEL, you know?
			special_message = " Honored by the Guild";

		if (is_alpha(scores->fate))
		{
			sprintf(death_description," killed by %s",
				death_cause_name((0xff & scores->fate), TRUE));
			if (COLS == 40 && strlen(death_description) > 23)
				strcpy(death_description," killed");
		}
		else
		{
			switch(scores->fate)
			{
				case 2:
					special_message = " A total winner!";
					break;
				case 1:
					strcpy(death_description," quit");
					break;
				default:
					strcpy(death_description," wierded out");
					break;
			}
		}
		if ((signed)(strlen(scores->player_name) + 10 +
			strlen(rank_names[scores->experience_level-1])) < COLS)
		{
			if (scores->experience_level > 1 && (strlen(scores->player_name)))
				printw(" \"%s\"",rank_names[scores->experience_level - 1]);
		}
		if (COLS == 40)
			move(row+1,6);
		if (special_message == NULL)
			printw("%s on level %d",death_description,scores->dungeon_level);
		else
			addstr(special_message);
	}
	standend();
	if (COLS == 80)
		addstr("\n\n\n\n");
}

static
int
insert_score(struct score_entry *new_score, struct score_entry *scores)
{
	int i = MAX_HIGH_SCORES - 1;
	int insert = MAX_HIGH_SCORES;

	/* Use an index: decrementing a pointer before oldlist is undefined in C. */
	for (; i >= 0 && new_score->gold > scores[i].gold; i--) {
		insert = i;
		if (i < MAX_HIGH_SCORES - 1)
			scores[i + 1] = scores[i];
	}
	if (insert == MAX_HIGH_SCORES)
		return 0;
	scores[insert] = *new_score;
	return insert + 1;
}
#endif //WIZARD
#endif //DEMO

/*
	 * show_death_screen:
	 *	Do something really fun when he dies
	 */
void
show_death_screen(char death_cause)
{
	char text[MAXSTR];
#ifndef DEMO
	register int year;

	player_gold -= player_gold / 10;

#ifdef ROGUE_DOS_CURSES
	switch_page(old_page_no);
	clear();
#endif
	drop_curtain();
	if (is_color)
		brown();
	box((COLS==40)?1:7,(COLS-28)/2,22,(COLS+28)/2);
	standend();

	center(10, "REST");
	center(11, "IN");
	center(12, "PEACE");
	red();
	center(21, "  *    *      * ");
	green();
	center(22, "___\\/(\\/)/(\\/ \\\\(//)\\)\\/(//)\\\\)//(\\__");
	standend();

	if (dos_screen_mode == 7)
		uline();
	center(14, tombstone_player_name);
	standend();

	/*@
	 * This looks like a no-op, but it's not: it makes sure prbuf, used
	 * internally in death_cause_name(), contains the actual death reason.
	 * tombstone_death_cause, the string used here, is re-assigned by update_protection_state()
	 * to point to
	 * prbuf if copy protection checks are successful. Otherwise, it contains
	 * the default "pirated" message. The same method is used with tombstone_player_name
	 * above.
	 */
	death_cause_name(death_cause, TRUE);

	strcpy(text,"killed by");

	center(15,text);
	center(16, tombstone_death_cause);

	sprintf(text, "%u Au", player_gold);
	center(18, text);

#ifdef ROGUE_DOS_CLOCK
	dos_regs->ax = 0x2a << 8;
	call_dos_interrupt(SW_DOS,dos_regs);
	year = dos_regs->cx;
#else
	year = current_local_time()->year;
#endif
	sprintf(text, "%u", year);
	center(19, text);
	raise_curtain();
	move(LINES-1, 0);
	update_high_scores(player_gold, 0, death_cause);
#else //DEMO
	register char *killer;
	demo(0);
	killer = death_cause_name(death_cause, TRUE);

	strcpy(text,"This time you were killed by");
	strcat(text," ");
	strcat(text,killer);
	if (strlen(text) > (COLS-2))
		center(6,"This time you were killed");
	else
		center(6, text);
	move(LINES-2,0);
#ifndef ROGUE_DOS_CURSES
	wait_for_enter("exit");
	printw("\n");
#endif
#endif //DEMO
	exit_game(EXIT_SUCCESS);
}

/*
 * show_victory_screen:
 *	Code for a winner
 */
void
show_victory_screen(void)
{
#ifndef DEMO
	register Entity *item;
	register int worth = 0;
	register byte column;
	register int previous_gold;

#ifdef ROGUE_DOS_CURSES
	switch_page(old_page_no);
#endif
	clear();
#ifdef MINROG
	if (!terse)
	{
	standout();
	printw("                                                               \n");
	printw("  @   @               @   @           @          @@@  @     @  \n");
	printw("  @   @               @@ @@           @           @   @     @  \n");
	printw("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
	printw("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
	printw("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
	printw("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
	printw("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
	}
	printw("                                                               \n");
	printw("     Congratulations, you have made it to the light of day!    \n");
	standend();
	printw("\nYou have joined the elite ranks of those who have escaped the\n");
	printw("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
	printw("a great profit and are admitted to the fighters guild.\n");
#else
	printw("Congratulations!\n\nYou have made it to the light of day!\n\n\n\n");
	printw("You journey home and sell all your\n");
	printw("loot at a great profit and are\n");
	printw("admitted to the fighters guild.\n\n\n");
#endif //MINROG
	mvaddstr(LINES - 1, 0, "--Press space to continue--");
	wait_for_key(' ');
	clear();
	mvaddstr(0, 0, "   Worth  Item");
	previous_gold = player_gold;
	for (column = 'a', item = player_inventory; item != NULL; column++, item = next(item))
	{
	switch (item->item_category)
	{
		when FOOD:
			worth = 2 * item->item_quantity;
		when WEAPON:
			switch (item->item_subtype)
			{
				when MACE: worth = 8;
				when SWORD: worth = 15;
				when CROSSBOW: worth = 30;
				when ARROW: worth = 1;
				when DAGGER: worth = 2;
				when TWO_HANDED_SWORD: worth = 75;
				when DART: worth = 1;
				when BOW: worth = 15;
				when BOLT: worth = 1;
				when SPEAR: worth = 5;
				break;
			}
			worth *= 3 * (item->item_hit_bonus + item->item_damage_bonus) + item->item_quantity;
			item->item_flags |= ITEM_IDENTIFIED;
		when ARMOR:
			switch (item->item_subtype)
			{
				when LEATHER: worth = 20;
				when RING_MAIL: worth = 25;
				when STUDDED_LEATHER: worth = 20;
				when SCALE_MAIL: worth = 30;
				when CHAIN_MAIL: worth = 75;
				when SPLINT_MAIL: worth = 80;
				when BANDED_MAIL: worth = 90;
				when PLATE_MAIL: worth = 150;
				break;
			}
			worth += (9 - item->item_modifier) * 100;
			worth += (10 * (armor_classes[item->item_subtype] - item->item_modifier));
			item->item_flags |= ITEM_IDENTIFIED;
		when SCROLL:
			worth = scroll_definitions[item->item_subtype].value;
			worth *= item->item_quantity;
			if (!scroll_identified[item->item_subtype])
				worth /= 2;
			scroll_identified[item->item_subtype] = TRUE;
		when POTION:
			worth = potion_definitions[item->item_subtype].value;
			worth *= item->item_quantity;
			if (!potion_identified[item->item_subtype])
				worth /= 2;
			potion_identified[item->item_subtype] = TRUE;
		when RING:
			worth = ring_definitions[item->item_subtype].value;
			if (item->item_subtype == RING_ADD_STRENGTH || item->item_subtype == RING_DAMAGE ||
				item->item_subtype == RING_PROTECTION || item->item_subtype == RING_DEXTERITY)
			{
				if (item->item_modifier > 0)
					worth += item->item_modifier * 100;
				else
					worth = 10;
			}
			if (!(item->item_flags & ITEM_IDENTIFIED))
				worth /= 2;
			item->item_flags |= ITEM_IDENTIFIED;
			ring_identified[item->item_subtype] = TRUE;
		when STICK:
			worth = wand_definitions[item->item_subtype].value;
			worth += 20 * item->item_charges;
			if (!(item->item_flags & ITEM_IDENTIFIED))
				worth /= 2;
			item->item_flags |= ITEM_IDENTIFIED;
			wand_identified[item->item_subtype] = TRUE;
			when AMULET:
			worth = 1000;
			break;
	}
	if (worth < 0)
		worth = 0;
	move(column - 'a' + 1, 0);
	printw( "%c) %5d  %s", column, worth, describe_item(item, FALSE));
	player_gold += worth;
	}
	move(column - 'a' + 1, 0);
	printw("   %5u  Gold Pieces          ", previous_gold);
	update_high_scores(player_gold, 2, 0);
#endif //DEMO
	exit_game(EXIT_SUCCESS);
}

/*
 * death_cause_name:
 *	Convert a code to a monster name
 */
char *
death_cause_name(byte cause, bool include_article)
{
	register char *cause_name;
	register bool needs_article;

	cause_name = description_buffer;
	needs_article = TRUE;
	switch (cause)
	{
	when 'a':
		cause_name = "arrow";
	when 'b':
		cause_name = "bolt";
	when 'd':
		cause_name = "dart";
	when 's':
		cause_name = "starvation";
		needs_article = FALSE;
	when 'f':
		cause_name = "fall";
	otherwise:
		if (is_monster_symbol(cause))
			cause_name = monster_definitions[cause-'A'].name;
		else
		{
			cause_name = "God";
			needs_article = FALSE;
		}
	}
	if (include_article && needs_article)
	sprintf(description_buffer, "a%s ", article_suffix(cause_name));
	else
	description_buffer[0] = '\0';
	strcat(description_buffer, cause_name);
	return description_buffer;
}

#ifdef DEMO
/*
 * For the demonstration version of rogue we really want to
 * Print out a message when the game ends telling them how
 * order the game.
 */
void
demo(int endtype)
{
	char demobuf[81];

#ifdef ROGUE_DOS_CURSES
	switch_page(old_page_no);
#endif
	clear();
	if (is_color)
		brown();
	box(0,0,LINES-2,COLS-1);
	bold();
	center(2,"ROGUE:  The Adventure Game");
	standend();
	if (is_color)
		lmagenta();
	sprintf(demobuf,"Sorry, %s but this is just a demonstration",player_name);
	if (terse)
		sprintf(demobuf,"Sorry, this is just a demonstration");
	center(4,demobuf);
	if (endtype == 1)   /* quiter */
	{
		sprintf(demobuf,"You quit with %u pieces of Gold",player_gold);
		center(6,demobuf);
	} else if (endtype == DEMOTIME) {
		sprintf(demobuf,"You ended with %u gold pieces",player_gold);
		center(6,demobuf);
	}
	if (terse)
			center(8,"If you're interested in doing some");
		else
			center(8,"But, if you're interested in doing some");
	center(9,"more exploring in the Dungeons of Doom");
	if (is_color)
		red();
	center(11,"Please Contact:                      ");
	if (!is_color)
		uline();
	else
		standend();
	center(13,"A. I. Design");
	center(14,"P.O. Box  3685");
	center(15,"Santa Clara, California 95055");
	if (is_color)
		red();
	center(17,"(408) 296-1634");
	if (is_color)
		yellow();
	else
		standend();
	center(19,"(C) Copyright 1983");
	high();
	center(20,"Artificial Intelligence Design");
	if (is_color)
		yellow();
	else
		standend();
	center(21,"All Rights Reserved");
	if (endtype == 0)
		return;
	move(LINES-2,0);
#ifndef ROGUE_DOS_CURSES
	wait_for_enter("exit");
	printw("\n");
#endif
	exit_game(EXIT_SUCCESS);
}
#endif //DEMO
