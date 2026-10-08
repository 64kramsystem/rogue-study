/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	5.1 (Berkeley) 5/11/82
 */

/*
 * Platform services, standard-library adapters, and retained DOS interface types.
 * Game state and gameplay declarations live in rogue.h; native platform services
 * are implemented in platform.c. Original header name: extern.h.
 */

//@ header guard not in original, mainly for curses_common.h
#ifndef ROGUE_PLATFORM_H
#define ROGUE_PLATFORM_H

/*@
 * Functions from libc and their "overrides"
 */

/*@
 * Set SUSv4 compatibility (POSIX.1-2008 base and XSI extensions)
 *
 * Enable setenv() and getenv(), remove toascii() and isascii()
 * Enable wide-character support ("complex renditions") in curses, if available
 * Implicitely set _POSIX_C_SOURCE 200809L
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html
 * https://man7.org/linux/man-pages/man7/feature_test_macros.7.html
 * https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
 */
#undef  _XOPEN_SOURCE  //@ set to 600 by pkg-config ncurses{,w}
#define _XOPEN_SOURCE 700


//@ uintptr_t, uint16_t
#include <stdint.h>

//@ is{alpha,digit,upper,...}() and to{upper,lower,...}() families
#include <ctype.h>
#ifndef isascii
//@ Marked obsolescent in POSIX-2008, so not in <ctype.h> if C99 is used
#define isascii(c)	(((c) & ~0x7f) == 0)
#endif

//@ str{len,cat,cpy,cmp,chr}() and possibly others
#include <string.h>
#define copy_value(dest,source)	memmove(&(dest),&(source),sizeof(dest))
#define fill_bytes(dest,length,ch)	memset(dest,ch,length)

//@ sprintf(), f{open,read,seek,write,close}(), remove(), putchar()
//@ popen(), fgets(), pclose()
#include <stdio.h>

//@ exit(), atoi(), NULL, EXIT_*, malloc(), free(), abs(), setenv(), getenv()
#include <stdlib.h>

//@ errno, originally in begin.asm
#include <errno.h>

//@ pause(), access(), sleep(), close()
#include <unistd.h>
#define access(f)	access(f, F_OK)

//@ time(), nanosleep()
#include <time.h>

//@ vsprintf()
#include <stdarg.h>

//@ bool type, originally typedef unsigned char
#include <stdbool.h>

#ifdef __linux__
//@ open()
#include <fcntl.h>
//@ ioctl()
#include <sys/ioctl.h>
//@ KDGKBLED
#include <linux/kd.h>
#endif

//@ setlocale()
#include <locale.h>


/*@
 * Project includes, defines and typedefs
 */
#include "dos_interrupts.h"

//@ created for show_fake_dos(), but could also be used in save.c and load.c
#ifndef ROGUE_DOS_DRIVE
#ifndef ROGUE_CURRENT_DRIVE
#define ROGUE_CURRENT_DRIVE	('C' - 'A')
#endif
#ifndef ROGUE_LAST_DRIVE
#define ROGUE_LAST_DRIVE	('F' - 'A')
#endif
#endif  // ROGUE_DOS_DRIVE

//@ moved from curses.h so it's close to 'bool' definition
#ifndef TRUE
#define TRUE 	1
#define FALSE	0
#endif

#define msleep(ms)	sleep_nanoseconds(1000000L * ms)

#ifdef __GNUC__
//@ macro for dummy arguments in stub functions
#define UNUSED(arg) __attribute__((unused))arg
#define PRINTF_FORMAT(format_index, first_argument) \
	__attribute__((format(printf, format_index, first_argument)))
#else
#define UNUSED(arg) arg
#define PRINTF_FORMAT(format_index, first_argument)
#endif


/*@
 * Simplified version of <time.h> struct tm, to wrap and abstract it,
 * so local time source is opaque and easily replaceable.
 */
struct local_time {
	int second;		/* Seconds	[0-60] (1 leap second) */
	int minute;		/* Minutes	[0-59] */
	int hour;		/* Hours	[0-23] */
	int day;		/* Day		[1-31] */
	int month;		/* Month	[0-11] */
	int year;		/* Year */
};
typedef struct local_time LocalTime;

typedef uintptr_t	PointerBits;  //@ size of a real pointer
typedef uint16_t	DosOffset;  //@ size of a pointer in DOS, as Rogue relies on
/*
 *  MANX C compiler funnies
 *  @ moved from rogue.h
 */
typedef unsigned char byte;


/*
 * Function types
 */
//@ mach_dep.c originals
int	random_seed_from_clock(void);
int	dos_service(int function_number, int argument);
int	call_dos_interrupt(int interrupt_number, struct dos_registers *registers);
int	simulate_dos_interrupt(int interrupt_number, struct dos_registers *input_registers,
		struct dos_registers *output_registers);
void	setup_game_io(void);
void	clear_macro_input(void);
void	show_credits(void);
void	protection_tick(void);
char	*allocate_memory(unsigned int byte_count);
byte	read_game_key(void);
bool	set_dos_break_check(bool state);
#ifdef ROGUE_DOS_CLOCK
void	install_dos_timer_hook(void);
void	restore_dos_timer_hook(void);
#endif

//@ new functions
byte	swap_bits(byte data, unsigned first_bit, unsigned second_bit, unsigned width);
int 	keyboard_lock_flags(void);
long	epoch_seconds(void);
LocalTime  	*current_local_time(void);
void	sleep_nanoseconds(long nanoseconds);

//@ dos.asm
int	code_checksum(void);
byte	dos_read_byte(int offset, int segment);
void	dos_write_byte(int offset, int segment, byte value);
void	dos_write_port(int port, byte value);
byte	dos_read_port(int port);
void	dos_write_memory(void *data, unsigned int wordlength, unsigned int segment, unsigned int offset);
void	dos_read_memory(void *buffer, unsigned int wordlength, unsigned int segment, unsigned int offset);
void	halt_game(void);
void	update_protection_state(void);
void	install_dos_break_handler(void);

//@ moved from main.c
void	fatal(const char *message_text, ...) PRINTF_FORMAT(1, 2);

//@ moved from croot.c
void	exit_game(int update_status_line);


/*@
 * Global vars
 */
#ifdef ROGUE_DOS_CLOCK
extern unsigned int tick;  //@ from dos.asm
#endif
#ifdef ROGUE_DOS_CURSES
extern char skip_retrace_check;
#endif
extern int monochrome_requested;  //@ from main.c, originally declared in rogue.h
extern struct dos_registers *dos_regs; //@ from main.c, originally declared in swint.h
#ifdef ROGUE_DEBUG
extern bool print_int_calls;
#endif
#ifndef ROGUE_DOS_DRIVE
extern int current_drive;
extern int last_drive;
#endif

#endif //ROGUE_PLATFORM_H
