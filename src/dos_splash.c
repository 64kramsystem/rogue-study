/*
 * This file contains the code to display the rogue picture files
 *
 * load.c	1.42	(A.I. Design)	2/12/84
 */

#include	"rogue.h"
#include "screen.h"

/*@
 * References for 3D8h and 3D9h:
 * http://minuszerodegrees.net/5150_5160/cards/5150_5160_cards.htm#cga
 * http://www.scribd.com/doc/251557562/CGA-IBM-Color-Graphics-Monitor-Adapter
 * http://www.seasip.info/VintagePC/cga.html
 */

/*@
 * 3D8h = CGA Select Mode port, 6-bits, write only:
 * Bit 0 - 0=40x25, 1=80x25 (for text mode, ignored if bit 1 = 0)
 * Bit 1 - 0=Text,  1=320x200 Graphics
 * Bit 2 - 0=Color, 1=Mono, or "burst color" for 3rd palette when in 320x200 **
 * Bit 3 - 1=(re-)enables video signal, which is disabled when mode changes
 * Bit 4 - 1=640x200 mono graphics mode
 * Bit 5 - 1=Enable blink attribute for text modes
 *
 * ** When in 320x200 mode, Bit 2 acts as "burst color" bit that activates
 *    an undocumented 3d palette (Default/Cyan/Red/Magenta)
 */
#define CGA_MODE_PORT    0x3d8  //@ CGA Select Mode port address
#define CGA_BURST_FLAG   0x004  //@ Bit 2 mask, for BW mode / 3rd palette burst bit
#define BIOS_CGA_MODE_OFFSET   0x065  //@ BIOS Data Area 40:65: current value of 3D8h port

/*@
 * 3D9h = CGA Select Color port, 6-bits, write only. For mode 4 (320x200x4c):
 * Bit 0 - Background/Default color, Blue  component
 * Bit 1 - Background/Default color, Green component
 * Bit 2 - Background/Default color, Red   component
 * Bit 3 - Intensified background color
 * Bit 4 - Alternate, intensified set of screen_color_count (the "i" palette variation)
 * Bit 5 - Active color set (Palette 0 or 1)
 *         Palette 0: 0-Background/Default, 1-Red,  2-Green,   3-Yellow
 *         Palette 1: 0-Background/Default, 1-Cyan, 2-Magenta, 3-White
 */
#define CGA_COLOR_PORT     0x3d9  //@ CGA Select Color port address
#define CGA_INTENSITY_FLAG 0x010  //@ Bit 4 mask, for intensified palette
#define CGA_PALETTE_FLAG 0x020  //@ Bit 5 mask, unused

static void	load_cga_picture(void);
static void	read_cga_memory(unsigned int segment);

//@ temp buffer to hold image file bytes
static char *picture_buffer;

/*@
 * block size used when reading image file
 * A packed CGA 320x200x2bit image (4 colors) requires 16384 bytes:
 *   16000 total of pixel data + 2 x 192-byte padding
 * Initial requested block size, 0x4000=16384, is enough to read file in 1 pass
 */
static int picture_block_size = 0x4000;
static FILE *picture_file;

/*@
 * Display the Rogue Enyx title image
 * - Set video mode to 320x200, 4 screen_color_count (CGA),
 * - Load 'rogue.pic' and display it for about 5 minutes
 *   or until a key is pressed,
 * - Return to previous video mode
 */
void
show_dos_splash(void)
{
	register int previous_video_mode = get_dos_video_mode();

	//@ 07h = Monochromatic (80x25 text) in MDA, Hercules, EGA, VGA
	if (previous_video_mode == 7 || (picture_file = fopen("rogue.pic", "r")) == NULL)
		return;
	//@ Allocate the largest possible block size for the store buffer,
	//@ halving the requested amount in each attempt
	while ((picture_buffer = malloc(picture_block_size)) == NULL)
		picture_block_size /= 2;
	//@ 04h = Graphics mode 320x200 4 colors in CGA,PCjr,EGA,MCGA,VGA
	set_dos_video_mode(4);

	load_cga_picture();
	fclose(picture_file);
#ifdef LOGFILE
	//@ originally a busy loop of 18 * 10 ticks
	sleep(10);
#else
	/*@
	 *  Blocking timeout mode does not work with standard ncurses, as the
	 *  underlying functions wtimeout() / wget_wch() only work properly after
	 *  curses initialization with initscr(), done later in main() by calling
	 *  initialize_screen(). As it is, it's non-blocking and returns immediately.
	 *  Not an issue considering the whole image display is dummy as there is no
	 *  (portable) way to switch to CGA graphics mode in standard C in 2020.
	 *
	 *  Originally a busy loop of 18 * 60 * 5 ticks with no_char() shortcut.
	 */
	getch_timeout(1000 * 60 * 5);
#endif  // LOGFILE
	set_dos_video_mode(previous_video_mode); //@ restore previous mode
	free(picture_buffer);
}

/*@
 * Load the file bytes to video memory
 * and adjust the video flags
 *
 * PIC/BSAVE format documentation
 * http://www.fileformat.info/format/pictor/egff.htm#PICTOR-DMYID.3.1.2
 * http://www.shikadi.net/moddingwiki/PIC_Format
 * http://www.shikadi.net/moddingwiki/Raw_CGA_Data#Interlaced_CGA_Data
 * http://en.wikipedia.org/wiki/BSAVE_%28bitmap_format%29#Graphics
 *
 * 'ROGUE.PIC', 16391 bytes, contains:
 * - BSAVE header - 7 bytes
 * - CGA Interlaced data, 2 blocks, even lines and odd lines. Each block is:
 *   - 8000 bytes of image
 *   -  192 bytes of padding
 * - (it does *not* contain the trailer byte 1Ah (CPM EOF)
 *
 *    7 bytes BSAVE header, ignored by read_cga_memory():
 *  BYTE Marker         Data type                  FDh = unpacked data
 *  WORD ScreenSegment  PC screen memory segment B800h = CGA video address
 *  WORD ScreenOffset   PC screen memory offset  0000h = no offset
 *  WORD DataSize       Size of screen data      4000h = 16384, 320x200x2bit
 *
 * 8000 bytes of image data for even rows
 *   BYTE ColorsX4      4 pixels per byte, each a 2-bit color. In palette 1i:
 *     00 Default (may select one out of 16 CGA colors, Black by default)
 *     01 Cyan
 *     02 Magenta
 *     11 White
 *
 * 192 bytes of padding, but PC Paint also stores metadata in this first block
 *    12B Signature     Editor used to create    'PCPaint V1.0'
 *   BYTE PaletteID     Current Palette number    05h in the bundled image; see the switch below
 *   BYTE BackColor     Color of Default(index 0) 00h = Black
 *   178B Padding       Padding                   55h x 178
 *
 * 8000 bytes of image data for odd rows
 *   Same format as even rows
 *
 * 192 bytes of padding
 *   192B Padding       Padding                   55h x 192
 */
static
void
load_cga_picture(void)
{
	int palette, background;
	int mode, burst;

	//@ Write the file. 0xb800 = Video memory address for CGA mode 04h
	read_cga_memory(0xb800);

	/*@
	 * read image palette and bgcolor from CGA memory that was just written
	 * with the file data. Offsets 8012 and 8013 are in the first 192-byte
	 * padding block, right after the 'PCPaint V1.0' signature
	 */
	palette = dos_read_byte(8012,0xB800);     //@ 5 = CGA palette 1i, see below
	background = dos_read_byte(8013,0xB800);  //@ 0 = Color index 0 (BG) is Black

	//@ Intensified palette, enable bit 4 for the CGA_COLOR_PORT write
	if (palette >= 3)
		background |= CGA_INTENSITY_FLAG;

	/*
	 * Only the burst flag computed here is used after the switch. Assignments back to
	 * palette are dead stores in this source. The native dos_read_byte/dos_write_port
	 * helpers are stubs, so this path cannot establish how retail DOS versions displayed
	 * the picture; the SDL loader provides the working native splash.
	 */
	burst   = 0;
	switch(palette)
	{
	case 2:
	case 5:
		burst = 1;
		/* fallthrough */
	case 0:
	case 3:
		palette = 1;  //@ ignored
		break;
	case 1:
	case 4:
		palette = 0;  //@ ignored
		break;
	}

	//@ Set background color and palette intensity
	dos_write_port (CGA_COLOR_PORT,background);

	//@ Read current video mode from BIOS Data Area, sans the burst bit
	mode = dos_read_byte(BIOS_CGA_MODE_OFFSET,0x40) & (~CGA_BURST_FLAG);
	if (burst == 1)
		mode = mode | CGA_BURST_FLAG;  //@ enable burst bit
	dos_write_byte(BIOS_CGA_MODE_OFFSET,0x40,mode);   //@ write new mode to BIOS Data Area
	dos_write_port(CGA_MODE_PORT,mode);           //@ write mode to CGA 6845 controller
}

/*@
		 * Load the file bytes into a memory segment
		 */
static
void
read_cga_memory(unsigned int segment)
{
	register unsigned offset = 0, blocks_read;

	if (!fread(picture_buffer, 7, 1, picture_file))	/* Ignore first seven bytes */
		fseek(picture_file, 7L, SEEK_SET);
	while ((blocks_read = fread(picture_buffer, picture_block_size, 1, picture_file))) {
		dos_write_memory(picture_buffer,blocks_read/2,segment,offset);
		if ((offset += blocks_read) >= 16384)
			break;
	}
}

/*@
 * Find the drive where game files are located, only used for copy protection.
 *
 * Return an int corresponding to the current drive or the "drive" value
 * from the fake environment (rogue.opt), where 0=A, 1=B, etc
 *
 * Also contained a no-op code which checked the existence of a file named
 * "jatgnas.8ys" in the root of such drive, but ignored the check result.
 *
 * Btw... what is this function doing here?
 */
int
find_copy_protection_drive(void)
{
#ifdef ROGUE_DOS_DRIVE
	int drive = dos_service(0x19, 0);  //@ Get Current Default Drive ignores the DX input.
#else
	int drive = current_drive;
#endif
	char configured_drive = copy_protection_drive[0];

	if (is_alpha(configured_drive))
	{
		/*@
		 * It looks like this block could be replaced with:
		 * drive = tolower(spec) - 'a';
		 */
		if (is_upper(configured_drive))
			drive = configured_drive - 'A';
		else
			drive = configured_drive - 'a';
	}
	/*
	 * The removed access(filename) probe discarded its result. No branch or return value
	 * in the original find_drive depends on the file's presence. Its historical purpose
	 * cannot be recovered from this function alone.
	 *
	 * Original filename construction and ignored probe:
	 * char filename[30];
	 * strcpy(filename,"a:jatgnas.8ys");
	 * filename[0] += (char)drive;
	 * access(filename);
	 */

	return drive;
}
