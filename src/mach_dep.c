/*
 * Various installation dependent routines
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include	"rogue.h"
#include	"curses.h"

#ifndef ROGUE_NO_X11
#include <X11/Xlib.h>

/*@
 * Pointer to X display, if running under X.  Used to query keyboard LED status.
 */
Display *x11_display = NULL;
#endif

#ifdef ROGUE_DOS_CLOCK
#define TIMER_VECTOR_OFFSET 0x70  // IVT byte offset for INT 1Ch (4 * 0x1C).
static DosOffset saved_timer_vector[2];
/*@
 * Global tick counter
 * Automatically incremented by update_protection_state() 18.2 times per second
 * Originally set by dos.asm
 */
unsigned int tick = 0;
#endif
static int saved_break_check;

/*
 * Permanent stack data
 * @ originally defined in main.c
 */
static struct dos_registers dos_register_storage;
struct dos_registers *dos_regs = &dos_register_storage;

#ifdef ROGUE_DEBUG
/*
 * Used to suppress printing BIOS INT calls. Used by some curses calls to
 * prevent flooding output with INT 10h/2 (move cursor) and INT 10h/9h (write
 * character) debug messages when printing strings.
 *
 */
bool print_int_calls = TRUE;
#endif

#ifndef ROGUE_DOS_DRIVE
/*@
 * These were created for show_fake_dos() to replace DOS INT 19h and 0Eh calls, and
 * are independent from env file copy_protection_drive[], just like the original. Created as
 * externs to allow future integration with env file and run-time selection.
 */
int current_drive = ROGUE_CURRENT_DRIVE;  //@ current fake drive (A=0, B=1, ...)
int last_drive = ROGUE_LAST_DRIVE;  //@ last available drive
#endif


byte swap_bits(
	byte data,
	unsigned first_bit,      // positions of bit sequences to swap
	unsigned second_bit,
	unsigned length  // number of consecutive bits in each sequence
)
{
	byte differing_bits = ((data >> first_bit) ^ (data >> second_bit)) & ((1U << length) - 1);
	return data ^ ((differing_bits << first_bit) | (differing_bits << second_bit));
}


int keyboard_lock_flags(void)
{
	int state = 0;
	int fd;
#ifndef ROGUE_NO_X11
	XKeyboardState keyboard_state;

	if (x11_display)
	{
		//@ terminal emulator under X, such as xterm / gnome-terminal
		XGetKeyboardControl(x11_display, &keyboard_state);
		state = swap_bits(keyboard_state.led_mask, 0, 2, 1);
	}
	else
#endif
#ifdef __linux__
	{
		//@ TTY such as getty / linux console
		if ((fd = open("/dev/tty", O_RDONLY | O_NOCTTY)) == -1 ||
				ioctl(fd, KDGKBLED, &state) == -1)
		{
			state = 0;
		}
		close(fd);
	}
#endif
	return state << 4;
}


/*
 * The original dos.asm csum_ adds 16-bit words from the code segment, starting at
 * _Corg_ + 0x200. new_level compares its result with the expected checksum after
 * level one. The expected value is stored in the data segment, not in the code being
 * read, so changing that data initializer need not change the code bytes.
 *
 * The archived loop uses signed `cmp bx, si; jl csmore`. It repeats when the end
 * address is less than the current address; the source alone does not establish that
 * this traverses the intended code range. A linked DOS image is needed to resolve
 * its actual bounds. This native stub returns the expected historical constant.
 */
int
code_checksum()
{
	return -1632;
}


/*
 * Native stub for a byte write to a real-mode segment:offset address. The original
 * implementation is in dos.asm. Native memory is not addressed through DOS segments.
 */
void
dos_write_byte(offset, segment, value)
	int UNUSED(offset);
	int UNUSED(segment);
	byte UNUSED(value);
{
	;  // it was written, I promise!
}


/*@
 * Read a byte from a segment:offset memory address
 *
 * Dummy, always return 0
 *
 * Originally in dos.asm. It zeroed AH so return is explicitly typed as byte.
 *
 * Only used in load.c to read CGA (0xB800) and BIOS (0x40) data
 */
byte
dos_read_byte(offset, segment)
	int UNUSED(offset);
	int UNUSED(segment);
{
	return 0;  // we just rebooted, so...
}


/*@
 * Write a byte to an I/O port
 *
 * Dummy no-op
 *
 * Originally in dos.asm
 *
 * Asm equivalent function is as a wrapper to OUT x86 CPU instruction.
 * Only AL was sent, hence byte as argument type. Port must be 16-bit as it is
 * written to DX, so a type uint16_t could be used enforce this.
 */
void
dos_write_port(port, value)
	int UNUSED(port);
	byte UNUSED(value);
{
	;  // and it's out! :)
}


/*@
 * Read from an I/O port
 *
 * A dummy wrapper to the x86 IN instruction. Return 0
 */
byte
dos_read_port(port)
	int UNUSED(port);
{
	return 0;  // maybe it's not connected :P
}


/*@
 * Write data to memory starting at segment:offset address
 *
 * Length of data is measured in words (16-bit), the size of int in DOS
 *
 * Dummy no-op, see dos_write_byte().
 *
 * Originally in dos.asm.
 *
 * While original is technically "direct memory access", the name is misleading
 * as it has nothing to do with DMA channels. And despite documentation on
 * dos.asm, it has no particular ties to video: it is a general use memory
 * writer that happens to be most often used to write to video memory address.
 * Asm works by setting the arguments and calling REP MOVSW
 */
#if defined(ROGUE_DOS_CURSES) && defined(ROGUE_DEBUG)
void
dos_write_memory(data, wordlength, segment, offset)
	void * data;
	unsigned int wordlength;
	unsigned int segment;
	unsigned int offset;
{
	printf("dmaout(%p, %d, %04x:%04x)\n",
			data, wordlength, segment, offset);
}
#else
void
dos_write_memory(data, wordlength, segment, offset)
	void UNUSED(*data);
	unsigned int UNUSED(wordlength);
	unsigned int UNUSED(segment);
	unsigned int UNUSED(offset);
{
		; // blazing fast!
}
#endif


/*@
 * Read memory starting at segment:offset address and store contents in buffer
 *
 * Length of buffer is measured in words (16-bit), the size of int in DOS
 *
 * Dummy no-op, leave buffer unchanged.
 *
 * Originally in dos.asm. See notes on dos_write_memory()
 */
void
dos_read_memory(buffer, wordlength, segment, offset)
	void UNUSED(*buffer);
	unsigned int UNUSED(wordlength);
	unsigned int UNUSED(segment);
	unsigned int UNUSED(offset);
{
	;
}


/*@
 * Immediately halt execution and hang the computer
 *
 * Originally in dos.asm
 *
 * Asm version triggered the nasty combination of CLI and HLT, effectively
 * hanging the PC. Now it uses a harmless pause()
 */
void
halt_game()
{
	shutdown_screen();
	printf("HALT!\n");
	pause();
}


/*@
 * Hook quit() as the ISR for CTRL-BREAK interrupts
 *
 * Originally in dos.asm
 *
 * Hook is performed using DOS INT 21h/AH=25h - Set Interrupt Vector
 * AL = interrupt number to hook - 23h for CTRL-BREAK
 * DS = handler function segment
 * DX = handler function offset
 *
 * Actually it did not hook quit() directly, instead it hooked an asm wrapper
 * that called quit(). For simplicity, it now "hooks" quit(), as this is bogus
 * code anyway. See notes on update_protection_state().
 */
void
install_dos_break_handler()
{
	struct dos_registers reg;
	reg.ax = 0x2523;  //@ hooking to INT 23h
	reg.ds = 0x33;  //@ dummy value for dos.asm's CS register
	reg.dx = (DosOffset)(PointerBits)quit;  //@ see install_dos_timer_hook() for note on casting
	call_dos_interrupt(SW_DOS, &reg);
}


/*
 * setup_game_io:
 *	Get starting setup for all games
 */
void
setup_game_io()
{
	terse = FALSE;
	dungeon_bottom_row = 23;
	if (COLS == 40) {
		dungeon_bottom_row = 22;
		terse = TRUE;
	}
	expert = terse;
	/*
	 * Vector CTRL-BREAK to call quit()
	 */
	install_dos_break_handler();
	saved_break_check = set_dos_break_check(0);
#ifndef ROGUE_NO_X11
	if (getenv("DISPLAY"))
		x11_display = XOpenDisplay(NULL);
#endif
}


#ifdef ROGUE_DOS_CLOCK
/*
 * Default timer-cleanup callback. install_dos_timer_hook replaces this function with
 * restore_dos_timer_hook after saving the original vector; exit_game invokes it.
 */
void
no_timer_cleanup()
{
	return;
}

void (*restore_timer_hook)() = no_timer_cleanup;


/*
 * The original routine replaces interrupt vector 1Ch, the BIOS user timer callback.
 * TIMER_VECTOR_OFFSET (0x70) is a byte offset in the interrupt vector table, not an
 * interrupt number: each vector occupies four bytes, so 0x70 / 4 = 0x1C.
 *
 * IBM's BIOS Interface Technical Reference, "Interrupt 08H - System Timer", states
 * that INT 08h calls INT 1Ch approximately 18.2 times per second. This explains the
 * original tick rate without an RTC interrupt or timer reprogramming.
 *
 * The native DOS-memory helpers are stubs; the maintained build uses epoch_seconds
 * for timing and does not install this hook.
 */
void
install_dos_timer_hook()
{
	/*@
	 * CS register value. Originally an extern set by begin.asm
	 * Set to dummy value of a "Hello World!" program as reported by gdb
	 */
	DosOffset _csval = 0x33;

	/*@
	 * Craft the 4-byte CS:offset function pointer for update_protection_state()
	 * Array indexes are swapped (CS=1, offset=0) as it writes directly to IVT
	 *
	 * Using the actual update_protection_state() protected mode address and "casting" it to a
	 * DOS real mode 16-bit offset makes the compiler happy and produce a legit
	 * dos_write_memory() call. But obviously the values in new_vec are completely bogus.
	 */
	DosOffset new_vec[2];  //@ type must match saved_timer_vector

	new_vec[0] = (DosOffset)(PointerBits)update_protection_state;
	new_vec[1] = _csval;

	/*
	 * The original implementation writes the interrupt vector table directly. The saved
	 * four-byte segment:offset pointer is restored by restore_dos_timer_hook; the source
	 * does not establish why the authors chose direct access over DOS vector services.
	 */
	dos_read_memory(saved_timer_vector, 2, 0, TIMER_VECTOR_OFFSET);
	dos_write_memory(new_vec, 2, 0, TIMER_VECTOR_OFFSET);
	restore_timer_hook = restore_dos_timer_hook;
}


/*@
 * Restore the INT 1Ch vector to its original value, as saved by install_dos_timer_hook()
 * update_protection_state() would no longer be called, and thus tick will not be updated.
 */
void
restore_dos_timer_hook()
{
	dos_write_memory(saved_timer_vector, 2, 0, TIMER_VECTOR_OFFSET);
}
#endif  // ROGUE_DOS_CLOCK


/*
 * Update the copy-protection watchdog and authenticated-game state. The archived
 * DOS clock_ interrupt also checks the single-step vector and increments tick_.
 * The native build calls this routine explicitly through protection_tick; it does
 * not run an interrupt-driven watchdog or reproduce the single-step-vector check.
 */
void
update_protection_state()
{
#ifdef ROGUE_DOS_CLOCK
	//@ tick the old clock
	tick++;
#endif

	//@ anti debugging: halt after 20 ticks if protection_watchdog_ticks is set
	if (protection_watchdog_ticks && ++protection_watchdog_ticks > 20)
		halt_game();

	/*@
	 * Unlock copy protection if floppy check succeeded: set tombstone strings
	 * (name, killed by) to actual player name and death reason, and restore
	 * hit multiplier. Only a single tick is required to unlock.
	 * See show_death_screen()
	 */
	if (incoming_damage_multiplier != 1 && disk_authentication_marker == 0xD0D)
	{
		tombstone_death_cause = description_buffer;
		tombstone_player_name = player_name;
		incoming_damage_multiplier = 1;
	}
}

/*@
 * Return Epoch time as an integer, with second resolution
 * Simple wrapper to <time.h> time()
 */
long
epoch_seconds(void)
{
	return (long)time(NULL);
}


/*@
 * Return current local time as a pointer to a struct
 */
LocalTime *
current_local_time()
{
	static LocalTime result;
	time_t now = time(NULL);
	struct tm *system_time = localtime(&now);
	result.second = system_time->tm_sec;
	result.minute = system_time->tm_min;
	result.hour   = system_time->tm_hour;
	result.day    = system_time->tm_mday;
	result.month  = system_time->tm_mon;
	result.year   = system_time->tm_year + 1900;
	return &result;
}


/*@
 * Sleep for nanoseconds
 */
void
sleep_nanoseconds(long nanoseconds)
{
	struct timespec ts = {0, nanoseconds};
	nanosleep(&ts, NULL);
}


/*
 * The original DOS srand helper returns a seed derived from INT 21h/AH=2Ch time
 * registers. It is unrelated to the standard C srand(seed) API. The native helper
 * returns epoch seconds; repeated calls within one second therefore return the same
 * seed, unlike the DOS helper's hundredth-second input.
 */
/*
 * returns a seed for a random number generator
 */
int
random_seed_from_clock()
{
#ifdef DEBUG
	return ++initial_random_seed;
#else
	/*
	 * Get Time
	 */
#ifdef ROGUE_DOS_CLOCK
	dos_service(0x2C);
	return(dos_regs->cx + dos_regs->dx);
#else
	return (int)epoch_seconds();
#endif  // ROGUE_DOS_CLOCK
#endif  // DEMO
}


/*
 * clear_macro_input:
 *	Flush typebuf for traps, etc.
 */
void
clear_macro_input()
{
#ifdef CRASH_MACHINE
	dos_regs->ax = 0xc06;		/* clear keyboard input */
	dos_regs->dx = 0xff;		/* set input flag */
	call_dos_interrupt(SW_DOS, dos_regs);
#endif //CRASH_MACHINE
	pending_macro_input = "";
}

/*
 * Display the credits and read the player's name before generating the first level.
 */
void
show_credits()
{
	#define ULINE() if(is_color) lmagenta();else uline();

	char entered_name[25];

	set_cursor_visible(FALSE);
	clear();
	if (is_color)
		brown();
	box(0,0,LINES-1,COLS-1);
	bold();
	center(2,"ROGUE:  The Adventure Game");
	ULINE();
	center(4,"The game of Rogue was designed by:");
	high();
	center(6,"Michael Toy and Glenn Wichman");
	ULINE();
	center(9,"Various implementations by:");
	high();
	center(11,"Ken Arnold, Jon Lane and Michael Toy");
	ULINE();
#ifdef INTL
	center(14,"International Versions by:");
#else
	center(14,"Adapted for the IBM PC by:");
#endif
	high();
#ifdef INTL
	center(16,"Mel Sibony");
#else
	center(16,"A.I. Design");
#endif
	ULINE();
	if (is_color)
		yellow();
	center(19,"(C)Copyright 1985");
	high();
#ifdef INTL
	center(20,"AI Design");
#else
	center(20,"Epyx Incorporated");
#endif
	standend();
	if (is_color)
		yellow();
	center(21,"All Rights Reserved");
	if (is_color)
		brown();
	move(22, 0);
	addch(DVRIGHT);
	repeat_character(DHLINE, COLS-2);
	addch(DVLEFT);
	standend();
	mvaddstr(23,2,"Rogue's Name? ");
	screen_updates_suspended = TRUE;		/*  status line hack @ to disable updates */
	high();
	read_line(entered_name,23);
	if (*entered_name && *entered_name != ESCAPE)
		strcpy(player_name, entered_name);
	screen_updates_suspended = FALSE;  //@ re-enable status line updates
#ifdef ROGUE_DOS_CURSES
	blot_out(23,0,24,COLS-1);
#else
	move(23, 0);
	//@ a single clrtobol(), if available, could replace the next 3 lines
	clrtoeol();
	move(24, 0);
	clrtoeol();
#endif
	if (is_color)
		brown();
	mvaddch(22,0,LLWALL);
	mvaddch(22,COLS-1,LRWALL);
	standend();
}


/*@
 * Non-blocking function that return TRUE if no key was pressed.
 *
 * Similar to ! kbhit() from DOS <conio.h>. POSIX has no (easy) replacement,
 * but this function will no longer be needed when ncurses getch() is set non-
 * blocking mode via nodelay() or timeout()
 *
 * Originally in dos.asm, calling a BIOS INT, which is reproduced here.
 *
 * But as simulate_dos_interrupt() is just a stub that returns ax = 0, this function will
 * always return FALSE, indicating a key was pressed.
 *
 * No longer used, as read_game_key() now uses non-blocking input internally.
 *
 * BIOS INT 16h/AH=1, Get Keyboard Status
 * Return:
 * ZF = 0 if a key pressed (even Ctrl-Break). Not tested, install_dos_break_handler() handles that.
 * AH = scan code. 0 if no key was pressed
 * AL = ASCII character. 0 if special function key or no key pressed
 * So AX = 0 for no key pressed

bool
no_char()
{
	struct dos_registers reg;
	reg.ax = HIGH(1);
	return !(call_dos_interrupt(SW_KEY, &reg) == 0);
}
 */


/*
 * read_game_key:
 *	Return the next input character, from the macro or from the keyboard.
 */
byte
read_game_key()
{
	int terminal_key;
	byte character;

	if (*pending_macro_input) {
		update_keyboard_and_clock();
		screen_refresh();  //@ macros
		return(*pending_macro_input++);
	}
	/*
	 * while there are no characters in the type ahead buffer
	 * update the status line at the bottom of the screen
	 */
	do
	{
		update_keyboard_and_clock();  /* Rogue spends a lot of time here @ you bet! */
		screen_refresh();  //@ command input
	}
	while ((terminal_key = getch_timeout(250)) == NOCHAR);
	character = translate_key(terminal_key);
	if (character == ESCAPE)
		command_repeat_count = 0;
	return character;
}


int
dos_service(function_number, argument)
	int function_number, argument;
{
	register struct dos_registers *saved_registers;

	dos_regs->ax = function_number << 8;
	dos_regs->bx = dos_regs->cx = 0;
	dos_regs->dx = argument;
	saved_registers = dos_regs;
	call_dos_interrupt(SW_DOS,dos_regs);
	dos_regs = saved_registers;
	return(0xff & dos_regs->ax);
}

/*
 *  newmem - memory allocater
 *         - motto: allocate or die trying
 */
/*@ Deprecated, see the new allocate_memory() below
char *
allocate_memory(nbytes,clrflag)
	unsigned int nbytes;
	int clrflag;
{
	register char *newaddr;

	newaddr = sbrk(nbytes);
	if (newaddr == (void *)-1)
		fatal("No Memory");
	end_mem = newaddr + nbytes;
	if ((intptr)end_mem & 1)  //@ guarantee word (16-bit) alignment?
		end_mem = sbrk(1);
	return(newaddr);
}
 */

/*@
 * newmem - memory allocater
 *        - motto: use malloc() like any sane software or die in 1985
 *
 * Clients should call free() for allocated objects
 */
char *
allocate_memory(byte_count)
	unsigned int byte_count;
{
	void * memory;
	if ((memory = (char *) malloc(byte_count)) == NULL)
		fatal("No Memory");
	return (char *)memory;
}


int
call_dos_interrupt(interrupt_number, registers)
	int interrupt_number;
	struct dos_registers *registers;
{
	//@ DS register value. Originally an extern set by begin.asm, now a dummy
	int data_segment = 0x00;

	registers->ds = registers->es = data_segment;
	simulate_dos_interrupt(interrupt_number, registers, registers);
	return registers->ax;
}

/*@
 * simulate_dos_interrupt() - System Interrupt Call
 * This was available as a C library function in old DOS compilers
 * Created here as a stub: output general registers are zeroed,
 * index and segment register values are copied from input.
 * Return FLAGS register, or rather a dummy with reasonable values
 */
int
simulate_dos_interrupt(interrupt_number, input_registers, output_registers)
#if defined(ROGUE_DOS_CURSES) && defined(ROGUE_DEBUG)
	int interrupt_number;
#else
	int UNUSED(interrupt_number);
#endif
	struct dos_registers *input_registers, *output_registers;
{
#if defined(ROGUE_DOS_CURSES) && defined(ROGUE_DEBUG)
	if(print_int_calls)
		printf("INT %x,%2X\t"
				"al=%2X\t"
				"bx=%4X\t"
				"cx=%4X\t"
				"dx=%4X\t"
				"si=%4X\t"
				"di=%4X\t"
				"ds=%4X\t"
				"es=%4X\n",
				interrupt_number,
				HI(input_registers->ax),
				LOW(input_registers->ax),
				input_registers->bx,
				input_registers->cx,
				input_registers->dx,
				input_registers->si,
				input_registers->di,
				input_registers->ds,
				input_registers->es);
#endif
	output_registers->ax = 0;
	output_registers->bx = 0;
	output_registers->cx = 0;
	output_registers->dx = 0;
	output_registers->si = input_registers->si;
	output_registers->di = input_registers->di;
	output_registers->ds = input_registers->ds;
	output_registers->es = input_registers->es;

	// reserved flags and IF set, all others unset
	return 0xF22A;
}

bool
set_dos_break_check(state)
	bool state;
{
	struct dos_registers registers;
	int previous_state;

	registers.ax = 0x3300;
	call_dos_interrupt(SW_DOS,&registers);
	previous_state = registers.dx &0xFF;

	registers.ax = 0x3300;  // Both calls select the query subfunction; this does not set the supplied state.
	registers.dx = state;
	call_dos_interrupt(SW_DOS,&registers);

	return previous_state;
}

void
restore_game_io()
{
	set_dos_break_check(saved_break_check);
#ifndef ROGUE_NO_X11
	if (x11_display)
	{
		XCloseDisplay(x11_display);
		x11_display = NULL;
	}
#endif
}


/*
 * The archived one_tick() initializes its outer counter to zero and tests while(i++).
 * The first condition is false, so the loop body, inner counter, and halt call never
 * execute. The source proves this behavior but does not establish the author's intent.
 *
 * The port waits 27 milliseconds and runs update_protection_state once. This unlocks
 * normal damage and tombstone text after successful disk authentication.
 */
void
protection_tick()
{
/*@
	int otick = tick;
	int i=0,j=0;

	while(i++)
	{
		while (j++)
			if (otick != tick)
				return;
			else if (i > 2)
				halt_game();
	}
 */
	msleep(27);
	update_protection_state();
}


/*@
 * Originally the message would never be seen, as it used printw() after an
 * endwin(), and there was no other blocking call after it, so any  messages
 * would be cleared instantly after display.
 */
/*
 *  fatal: exit with a message
 *  @ moved from main.c, changed to use varargs and actually print the message
 */
void
fatal(const char *message_text, ...)
{
	va_list arguments;

	shutdown_screen();

	va_start(arguments, message_text);
	vprintf(message_text, arguments);
	va_end(arguments);
	exit_game(EXIT_SUCCESS);
}


/*@
 * The single point of exit for Rogue
 * renamed from exit() to avoid conflict with <stdlib.h>
 * moved from croot.c
 */
void exit_game(int update_status_line)
{
#ifdef ROGUE_DOS_CLOCK
	//@ restore the clock, it if was ever set
	(*restore_timer_hook)();
#endif
	shutdown_screen();
	restore_game_io();
	free_game_state();
#ifdef ROGUE_DEBUG
	printf("Exited normally\n");
#endif
	exit(update_status_line);
}
