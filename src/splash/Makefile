TARGET:=splash
.DEFAULT_GOAL := default
CFLAGS?=-Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wcast-function-type -std=c11 -pedantic -Og -g
LDLIBS:=

# --silence-errors only needed for 'make clean' from parent dir
CFLAGS+=$(shell pkg-config --silence-errors sdl2 --cflags)
LDLIBS+=$(shell pkg-config --silence-errors sdl2 --libs)

OBJS=splash.o sdl_splash.o

all: default

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

%.o: %.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(OBJS:.o=.d)

clean:
	rm -f $(TARGET) *.o *.d

.PHONY: all default clean
