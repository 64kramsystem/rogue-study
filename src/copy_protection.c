/*
 * C copy protection routines
 */

#include	"rogue.h"

/*
 * The original dos.asm clock_ handler increments protection_watchdog_ticks while it
 * is nonzero and halts after it exceeds 20. This routine disables the watchdog around
 * BIOS disk reads and clears it on every return. At the BIOS timer rate, the limit is
 * roughly one second; it does not measure disk-read time.
 */
int protection_watchdog_ticks;

#ifdef ROGUE_ORIGINAL_COPY_PROTECTION
#define UNDEFINED	0
#define DONTCARE	0

#define	CRC		0x10

static struct dos_registers track_read_request = {
	0x206,
	0,
	0x2701,
	UNDEFINED,
	DONTCARE,
	DONTCARE,
	DONTCARE,
	0xF800
} ;

static struct dos_registers signature_read_request = {
	0x201,
	UNDEFINED,
	0x2707,
	UNDEFINED,
	DONTCARE,
	DONTCARE,
	DONTCARE,
	UNDEFINED
} ;

static struct dos_registers crc_read_request = {
	0x201,
	UNDEFINED,
	0x27F1,
	UNDEFINED,
	DONTCARE,
	DONTCARE,
	DONTCARE,
	UNDEFINED
} ;

/*@
 * Current value of the Data Segment register DS
 * Return a dummy value of 0
 * Originally in dos.asm
 */
int
dos_data_segment(void)
{
	return 0;
}
#endif

#ifndef ROGUE_ORIGINAL_COPY_PROTECTION
void
authenticate_game_disk(int UNUSED(drive))
{
	disk_authentication_marker = 0xD0D;  //@ success marker: 0xD0D stands for "Dungeons Of Doom"
	protection_watchdog_ticks = 0;
}
#else
void
authenticate_game_disk(int drive)
{
	int i, flags;
	struct dos_registers read_registers;
	char sector_bytes[512];
	char signature_bytes[32];

	protection_watchdog_ticks++;
	track_read_request.dx = signature_read_request.dx = crc_read_request.dx = drive;
	signature_read_request.es = crc_read_request.es = dos_data_segment();
	signature_read_request.bx = (DosOffset)(PointerBits)(&signature_bytes[0]);  //@ bogus address to fit bx
	crc_read_request.bx = (DosOffset)(PointerBits)(&sector_bytes[0]);  //@ ditto

	//@ read sectors until first success, try up to 7 times
	for (i=0,flags=CF;i<7 && (flags&CF);i++)
	{
		read_registers = track_read_request;
		protection_watchdog_ticks = 0;
		flags = simulate_dos_interrupt(SW_DSK,&read_registers,&read_registers);
		protection_watchdog_ticks++;
	}
	//@ return if no success
	if (CF&flags)
	{
		protection_watchdog_ticks = 0;
		return;
	}
	//@ read sectors until first success, try up to 3 times
	for (i=0,flags=CF;i<3 && (flags&CF);i++)
	{
		read_registers = signature_read_request;
		protection_watchdog_ticks = 0;
		flags = simulate_dos_interrupt(SW_DSK,&read_registers,&read_registers);
		protection_watchdog_ticks++;
	}
	//@ return if no success
	if (CF&flags)
	{
		protection_watchdog_ticks = 0;
		return;
	}
	//@ try up to 4 times to get a CRC failure on read
	for (i=0;i<4;i++)
	{
		read_registers = crc_read_request;
		protection_watchdog_ticks = 0;
		flags = simulate_dos_interrupt(SW_DSK,&read_registers,&read_registers);
		protection_watchdog_ticks++;
		//@ failure read by bad CRC is expected and required for validation!
		if ((flags&CF) && HI(read_registers.ax) == CRC)
		{
			if (memcmp(&signature_bytes[0],&sector_bytes[0x8c],32) == 0)
				disk_authentication_marker = 0xD0D;
			protection_watchdog_ticks = 0;
			return;
		}
	}
	protection_watchdog_ticks = 0;
}
#endif
