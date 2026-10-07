#include <assert.h>  // assert
#include <errno.h>   // errno
#include <stdarg.h>  // variable arguments: va_*, ...
#include <stdio.h>   // f*, *printf, NULL, EOF, stderr
#include <string.h>  // str*, memcmp

#include <SDL.h>


// A.K.A floor(log2(x)) or logb(x)
int integer_log2(int value)
{
	int exponent = 0;
	while (value >>= 1) exponent++;
	return exponent;
}


void print_error(const char *format, ...)
{
	char message_text[1000];

	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message_text, sizeof(message_text), format, arguments);
	va_end(arguments);

	fprintf(stderr, "error: %s\n", message_text);
}


void print_warning(const char *format, ...)
{
	char message_text[1000];

	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message_text, sizeof(message_text), format, arguments);
	va_end(arguments);

	fprintf(stderr, "warning: %s\n", message_text);
}


void report_picture_read_error(FILE* file, const char* path, int size, const char* type)
{
	if (feof(file))  // too small
		print_error("invalid BSAVE %s, PIC must have %d bytes: %s",
				type, size, path);
	else
		print_error("could not read file: %s\n", path);
	fclose(file);
}


// path: Path to rogue PIC splash image. Fallback to "./rogue.pic" if NULL.
//       rogue's main() calls this using ROGUE_PIC environment variable,
//       which is NULL if not set.
int show_sdl_splash(const char* path)
{
	// Independent constants
	const int CGA_WIDTH      = 320;  // Columns in graphics mode 4
	const int CGA_HEIGHT     = 200;  // Rows
	const int CGA_BG_COLOR   =   0;  // First color in palette is the background
	const int CGA_FIELDS     =   2;  // Interleaf: even and odd lines (rows)
	const int CGA_PADDING    = 192;  // 192 bytes of padding after each field
	const int CGA_COLORS[][3]= {     // Palette 1i (intensified), not all colors
		{  0,   0,   0},  // Black
		{ 85, 255, 255},  // Light Cyan
		{255,  85, 255},  // Light Magenta
		{255, 255, 255}   // White
	};

	// Derived constants
	const int CGA_NUM_COLORS = sizeof(CGA_COLORS) / sizeof(*CGA_COLORS);  // 4
	const int CGA_BIT_DEPTH  = integer_log2(CGA_NUM_COLORS);  // 2 bits per color
	const int CGA_PPB        = 8 / CGA_BIT_DEPTH;  // 4 pixels per Byte
	const int CGA_DATA_SIZE  = CGA_WIDTH * CGA_HEIGHT / CGA_PPB;          // 16000
	const int CGA_SIZE       = CGA_DATA_SIZE + CGA_FIELDS * CGA_PADDING;  // 16384

	// BSAVE format constants
	// https://en.wikipedia.org/wiki/BSAVE#Typical_variations
	const unsigned char BSAVE_HEADER[] = {
		'\xfd',          // ID Flag = file descriptor identifier bsaved file
		'\x00', '\xb8',  // = 0xB800, CGA video RAM segment
		'\x00', '\x00',  // = 0x0000, CGA video RAM offset
		'\x00', '\x40'   // = 0x4000, CGA video RAM size, CGA_SIZE
	};
	const char* const PIC_SIG   = "PCPaint V1.0";
	const int SIG_OFFSET        = CGA_DATA_SIZE / CGA_FIELDS;       //  8000
	const int BSAVE_SIZE        = sizeof(BSAVE_HEADER) + CGA_SIZE;  // 16391

	// Oh, the wonders of modern platforms and megabytes of stack space!
	// A modern GNU/Linux's 8 MB *stack* is almost as large as the *hard drive*
	// of an 1985 IBM XT, and more than 10 times its RAM. Food for thought...
	unsigned char data[CGA_SIZE];

	// Fallback path to rogue splash image: "rogue.pic" in current directory
	const char* const PIC_PATH  = "./rogue.pic";

	FILE*         file     = NULL;
	SDL_Window*   window   = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Event     event;

	if (!path)
		path = PIC_PATH;

	if ((file = fopen(path, "rb")) == NULL) {
		print_error("%s: %s", strerror(errno), path);
		return 0;
	}

	// Read and check file header
	if (!fread(data, sizeof(BSAVE_HEADER), 1, file)) {
		report_picture_read_error(file, path, BSAVE_SIZE, "header");
		return 0;
	}
	if (memcmp(data, BSAVE_HEADER, sizeof(BSAVE_HEADER))) {
		print_warning("invalid header, possibly not a valid PIC image in "
				"BSAVE format: %s", path);
	}

	// Read file data
	if (!fread(data, sizeof(data), 1, file)) {
		report_picture_read_error(file, path, BSAVE_SIZE, "data");
		return 0;
	}

	// Check extra data
	if (fgetc(file) != EOF)
		print_warning("file is larger than %d bytes, possibly not a valid "
				"PIC image in BSAVE format: %s", BSAVE_SIZE, path);
	fclose(file);

	if (strncmp(PIC_SIG, (char *)&data[SIG_OFFSET], (int)strlen(PIC_SIG)) != 0)
		print_warning("invalid PIC signature at offset 0x%X, expected '%s' in: %s",
				sizeof(BSAVE_HEADER) + SIG_OFFSET, PIC_SIG, path);

	if (   SDL_Init(SDL_INIT_VIDEO) != 0
	    || SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear") != SDL_TRUE
	    || SDL_CreateWindowAndRenderer(0, 0,
			SDL_WINDOW_FULLSCREEN_DESKTOP, &window, &renderer) != 0
	) {
		SDL_Quit();
		print_error("could not initialize SDL: %s", SDL_GetError());
		return 0;
	}
	SDL_RenderSetLogicalSize(renderer, CGA_WIDTH, CGA_HEIGHT);
	assert(CGA_BG_COLOR >= 0 && CGA_BG_COLOR < CGA_NUM_COLORS);
	SDL_SetRenderDrawColor(
		renderer,
		CGA_COLORS[CGA_BG_COLOR][0],
		CGA_COLORS[CGA_BG_COLOR][1],
		CGA_COLORS[CGA_BG_COLOR][2],
		SDL_ALPHA_OPAQUE
	);
	SDL_RenderClear(renderer);  // Clear the screen

	// Decode the image into the renderer
	// Could be done in fewer nested loops, but this is much easier to follow.
	int data_offset = 0, field, y, x, pixel_offset;
	unsigned char color_index, previous_color_index = CGA_BG_COLOR;
	// Field loop: 2 blocks of even and odd lines
	for (field = 0; field < CGA_FIELDS; field++, data_offset += CGA_PADDING) {
		// Line (row) loop. Screen Y is (y + field)
		for (y = 0; y < CGA_HEIGHT; y += CGA_FIELDS) {
			// Byte (column) loop. Screen X is (x + p)
			for (x = 0; x < CGA_WIDTH; x += CGA_PPB, data_offset++) {
				assert(data_offset < CGA_SIZE);
				// Pixel loop. Each byte contains 4 pixels
				for (pixel_offset = 0; pixel_offset < CGA_PPB; pixel_offset++) {
					// "unpack" the pixels: (d >> 6, 4, 2, 0) & 0b00000011;
					color_index = (data[data_offset] >> ((CGA_PPB - pixel_offset - 1) * CGA_BIT_DEPTH))
							& (CGA_NUM_COLORS-1);
					assert(color_index < CGA_NUM_COLORS);
					// Set color, if needed
					if (color_index != previous_color_index) {
						SDL_SetRenderDrawColor(
							renderer,
							CGA_COLORS[color_index][0],
							CGA_COLORS[color_index][1],
							CGA_COLORS[color_index][2],
							SDL_ALPHA_OPAQUE
						);
						previous_color_index = color_index;
					}
					// Draw!
					SDL_RenderDrawPoint(renderer, x+pixel_offset, y+field);
				}
			}
		}
	}
	assert(data_offset == CGA_SIZE);
	SDL_RenderPresent(renderer);

	// Wait for 5 minutes or until keyboard/mouse button press
	while (SDL_GetTicks() < 1000 * 60 * 5) {
		if (   SDL_PollEvent(&event)
		    && (   event.type == SDL_QUIT
		        || event.type == SDL_KEYUP
		        || event.type == SDL_MOUSEBUTTONUP
		       )
		)
			break;
		SDL_Delay(16);  // ~60FPS
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 1;
}
