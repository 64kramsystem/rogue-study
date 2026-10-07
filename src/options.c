/*
 * Env.c:   routines to set up environment
 *          Jon Lane  -  10/31/83
 */

/*@
 * setenv() and putenv() are different from their counterparts at <stdlib.h>:
 * "Environment" is read from a text file and manipulated in a custom struct
 */

#include "rogue.h"  //@ could be "extern.h" if not for some strings.c functions

#define ERROR   -1
#define MATCH    0
#define OPTION_COUNT	 8
#define FOREVER	 1

//@ made static. could also be hardcoded in struct environment element array
static char name_option_key[] = "name";
static char save_option_key[] = "savefile";
static char score_option_key[] = "scorefile";
static char macro_option_key[] = "macro";
static char fruit_option_key[] = "fruit";
static char drive_option_key[] = "drive";
static char menu_option_key [] = "menu";
static char screen_option_key[]   = "screen";

//@ public extern'ed vars
char player_name[] = "Rodney\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
char score_filename[]  =  "rogue.scr\0\0\0\0\0";
char save_filename[]   =   "rogue.sav\0\0\0\0\0";
char keyboard_macro[]    =   "v\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
char favorite_fruit[]    =  "Slime Mold\0\0\0\0\0\0\0\0\0\0\0\0\0";
char copy_protection_drive[]  =  "?";
char menu_option[]   =  "on\0";
char screen_option[]    =  "\0w fast";

static
struct option_entry {
	char *key;
	char *value;
	int  strlen;
} options[OPTION_COUNT] = {
	{name_option_key,	player_name,		23},
	{score_option_key,	score_filename,	14},
	{save_option_key,	save_filename,		14},
	{macro_option_key,	keyboard_macro,		40},
	{fruit_option_key,	favorite_fruit,		23},
	{drive_option_key,	copy_protection_drive,	 1},
	{menu_option_key,	menu_option,		 3},
	{screen_option_key,	screen_option,	 7},
};

static byte	read_option_character(void);
static void	set_option_value(char *label, char *string);

//@ already static in original
static FILE *options_file;
static byte option_character;
static int option_parse_state;
static char option_key_buffer[11], option_value_buffer[MACROSZ];
static char *option_key_cursor, *option_value_cursor;

//@ renamed from setenv() to avoid collision with <stdlib.h>
/*
 *  setenv_file: read in environment from a file
 *
 *        envfile - name of file that contains data to be
 *                  put in the environment
 *
 *        STATUS  - setenv return
 *                  @@ FALSE on failure to open envfile, TRUE otherwise
 */
bool
load_options_file(filename)
	char *filename;
{
	register char separator;

	protection_tick();	/* if he tries to disable the clock */
	if ((options_file = fopen(filename, "r")) == NULL)
	{
		return FALSE;
	}

	while ( FOREVER )
	{
		/*
		 * Look for another label
		 */
		option_parse_state = 0;
		option_key_cursor = option_key_buffer;
		option_value_cursor = option_value_buffer;

		/*
		 * Skip white space, this is the only state (pstate == 0)
		 * where eof will not be aborted
		 */
		while (is_space(read_option_character()))
			;
		if (option_character == 0)
			break;
		option_parse_state = 3;
		/*
		 * Skip comments.
		 */
		if (option_character == '#') {
			while (read_option_character() != '\n')
				;
			continue;
		}
		option_parse_state = 1;
		/*
		 * start of label found
		 */
		*option_key_cursor = option_character;
		while ((separator = read_option_character()) != '=' && separator != '-') {
			if (separator == '\n')
				fatal("rogue.opt: incorrect file format\n");
			if ((!is_space(*option_key_cursor) || !is_space(option_character))
			    && option_key_cursor < &option_key_buffer[sizeof(option_key_buffer) - 2])
				*(++option_key_cursor) = option_character;
		}
		if (!is_space(*option_key_cursor))
			option_key_cursor++;
		*option_key_cursor = 0;

		/*
		 * Looking for corresponding string
		 */
		option_parse_state = 2;
		while (is_space(read_option_character()) && option_character != '\n')
			;

		/*
		 * Start of string found
		 */
		if (option_character != '\n') {
			*option_value_cursor = option_character;
			while (read_option_character() != '\n')
				if ((!is_space(*option_value_cursor) || !is_space(option_character))
				    && option_value_cursor < &option_value_buffer[sizeof(option_value_buffer) - 2])
					*(++option_value_cursor) = option_character;
			if (!is_space(*option_value_cursor))
				option_value_cursor++;
		}
		*option_value_cursor = 0;
		lowercase_string(option_key_buffer);
		set_option_value(option_key_buffer,option_value_buffer);
		/* printf("env: found (%s) = (%s)\n",blabel,bstring); */
	}
	fclose(options_file);
	lowercase_string(menu_option);
	lowercase_string(screen_option);
	return TRUE;
}

/*
		 *  Peekc -
		 *  Return the next char associated with
		 *  efd (environment file descripter
		 *
		 *  This routine has some knowledge of the
		 *  file parsing state so that it knows
		 *  if there has been a premature eof.  This
		 *  way I can avoid checking for premature eof
		 *  every time a character is read.
		 */
static
byte
read_option_character(void)
{
	option_character = 0;
	if (!fread(&option_character, 1, 1, options_file)) {
		if (ferror(options_file))
			fatal("rogue.opt: could not read file\n");
		/*
		 * When looking for the end of the string,
		 * Let the eof look like newlines
		 */
		if (option_parse_state >= 2)
			option_character = '\n';
		else if (option_parse_state != 0)
			fatal("rogue.opt: incorrect file format\n");
	}
	if (option_character == 26)  //@ EOF char, common in text files back then.
		option_character = '\n';
	return(option_character);
}

#ifdef LUXURY
/*
 * Getenv: UNIX compatable call
 *
 *	  label - label of thing in environment
 *
 *	  STATUS - returns the string associated with the label
 *			   or NULL (0) if it is not present
 *
 * @ used only by the unused is_set(), so safe to remove. renamed from getenv()
 * @ to avoid conflict with <stdlib.h>, and also made static
 */
static
char *
get_option_value(label)
	char *label;
{
	register int i;

	for (i=0 ; i<OPTION_COUNT ; i++ )
	{
		if ( strcmp(label,options[i].key) == MATCH )
			return(options[i].value);
	}
	return(NULL);
}
#endif

//@ renamed from putenv() to avoid collision with <stdlib.h>
/*
 * set_option_value: Put something into the "fake" environment struct
 *
 *	  label  - label of thing in environment
 *	  string - string associated with the label
 *
 *	  No meaningful return codes to save data space
 *	  Just ingnores strange labels
 */
void
set_option_value(label,string)
	char *label, *string;
{
	register int i;

	for (i=0 ; i<OPTION_COUNT ; i++)
	{
		if ( strcmp(label,options[i].key) == MATCH )
			copy_string_bounded(options[i].value, string, options[i].strlen);
	}
}

#ifdef LUXURY
//@ unused, safe to remove, made static
static
bool
is_set(label,string)
	char *label,*string;
{
	return(!strcmp(string,getenv(label)));
}
#endif
