#include "extern.h"

//@ extern char ctp_[]; //@ not needed anymore. could not find definition

/*@
 * Functions available in <ctype.h>
 *
isalpha(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x03); }
isupper(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x01); }
islower(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x02); }
isdigit(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x04); }
isspace(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x10); }
isprint(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0xc7); }

toascii(x)
{
	return (x&127);
}

toupper(chr)
	char chr;
{
	return(islower(chr)?((chr)-('a'-'A')):(chr));
}

tolower(chr)
	char chr;
{
	return(isupper(chr)?((chr)+('a'-'A')):(chr));
}
*/

//@ Locale-independent versions, as expected by Rogue
bool is_alpha(char character) { return (isascii(character) && isalpha(character)); }
bool is_upper(char character) { return (isascii(character) && isupper(character)); }
bool is_lower(char character) { return (isascii(character) && islower(character)); }
bool is_digit(char character) { return (isascii(character) && isdigit(character)); }
bool is_space(char character) { return (isascii(character) && isspace(character)); }
bool is_print(char character) { return (isascii(character) && isprint(character)); }

/*@
 * No exact match in signature and behavior from glibc or POSIX
 * Similar to <string.h> strncpy(), but not a drop-in equivalent.
 * snprintf() is perhaps a better replacement candidate.
 */
char *
copy_string_bounded(destination,source,max_characters)
	char *destination, *source;
	int max_characters;
{
	while (max_characters-->0 && *source)
		*destination++ = *source++;
	*destination = 0;
	/*
	 * lets return the address of the end of the string so
	 * we can use that info if we are going to cat on something else!!
	 */
	return (destination);
}


/*
 * redo Lattice token parsing routines
 */

//@ strip leading blanks
char *
skip_whitespace(text)
	char *text;
{
	while (is_space(*text))
		text++;
	return(text);
}

/*
 * remove trailing whitespace from the end of a line
 */
char *
trim_trailing_whitespace(text)
	char *text;
{
	register char *backup;

	backup = text + strlen(text);
	while (backup != text && is_space(*(--backup)))
		*backup = 0;
	return(text);
}

/*
 * lcase: convert a string to lower case
 */
void
lowercase_string(text)
	char *text;
{
	while ( (*text = tolower((unsigned char)*text)) )
		text++;
}
