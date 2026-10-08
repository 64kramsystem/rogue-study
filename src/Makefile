#@@ the following note was preserved from original, everything afterwards is new
#
# Make file for building Rogue
# Michael Toy, AI Design, May 1984
# :bk=0
#
# NOTE: If new files are added or old files removed from these
#	object lists, you must modify the file objs as well.
#
#	To disbale stack overflow checking (thus getting a smidge more
#	speed.  Don't load in csav.o
#
TARGET:=rogue
.DEFAULT_GOAL := default
# ISO C17/18 requires GCC8 or greater: CC=gcc-8 make ...
CFLAGS:=-Wall -Wextra -Wformat-security -Wstrict-prototypes -Wold-style-definition -std=c17 -pedantic
# An empty show_message format deliberately clears the message line.
CFLAGS+=-Wno-format-zero-length
LDLIBS:=

# Defines from original Rogue code turned on by default here:
CFLAGS+=-DMINROG

# New defines: ROGUE_*
ifdef ROGUE_RELEASE
CFLAGS+=-s -O2
else
# -Og is enough to enable _FORTIFY_SOURCE=2
# To test it: gcc -E -Og -dM -x c /dev/null | grep FORTIFY
CFLAGS+=-Og -g3
endif
ifdef ROGUE_DEBUG
CFLAGS+=-DROGUE_DEBUG
endif
ifdef ROGUE_DEMO
CFLAGS+=-DROGUE_DEMO
endif
UNICODE=3
# Valid values for ROGUE_CHARSET:
# 1 - ASCII, requires ncurses. Looks like Rogue UNIX, but with color
# 2 - CP437, requires ncurses, configured terminal and CP437 font. Good luck!
# 3 - Unicode, requires ncursesw and Unicode font. The default, as set below.
# See screen_common.h
ROGUE_CHARSET:=$(UNICODE)
# Debian and others can use 'ncurses{,w}{5,6}-config' instead of 'pkg-config ncurses{,w}'
ifeq ($(ROGUE_CHARSET),$(UNICODE))
CFLAGS+=$(shell pkg-config ncursesw --cflags)
LDLIBS+=$(shell pkg-config ncursesw --libs)
else
CFLAGS+=-DROGUE_CHARSET=$(ROGUE_CHARSET)
CFLAGS+=$(shell pkg-config ncurses --cflags)
LDLIBS+=$(shell pkg-config ncurses --libs)
endif

# X11 library is required to determine keyboard LED status in an X terminal.
ifdef ROGUE_NO_X11
CFLAGS+=-DROGUE_NO_X11
else
CFLAGS+=$(shell pkg-config x11 --cflags)
LDLIBS+=$(shell pkg-config x11 --libs)
endif

export CFLAGS

OBJS=armor.o level_generation.o command.o platform.o endgame.o save.o \
	chase.o screen.o turn_scheduler.o turn_effects.o options.o game_state.o fake_dos.o fight.o \
	init.o messages.o entity_pool.o maze.o gameplay.o monsters.o move.o inventory.o passages.o \
	potions.o rings.o rooms.o scrolls.o wands.o strings.o items.o \
	weapons.o wizard.o slime.o dos_splash.o copy_protection.o

ASMS=zoom.o dos.o begin.o fio.o sbrk.o csav.o

default: $(TARGET)

all: default sdl

sdl: splash $(TARGET)-sdl

splash: splash/splash

# Share the loader target so parallel sub-makes never compile it twice.
splash/splash: splash/sdl_splash.o FORCE
	$(MAKE) -C splash splash

splash/sdl_splash.o: FORCE
	$(MAKE) -C splash sdl_splash.o

FORCE:

main-sdl.o: CFLAGS+=-DROGUE_SPLASH
main-sdl.o: main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

$(OBJS) main.o main-sdl.o: Makefile

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

$(TARGET)-sdl: LDLIBS+=$(shell pkg-config sdl2 --libs)
$(TARGET)-sdl: $(OBJS) main-sdl.o splash/sdl_splash.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TARGET): $(OBJS) main.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

-include $(OBJS:.o=.d) main.d main-sdl.d

clean:
	rm -f $(TARGET) $(TARGET)-sdl *.o *.d
	$(MAKE) -C splash $@

.PHONY: default all sdl splash clean FORCE
