CFLAGS = -std=c17 -Wall -Wextra -pedantic -g -O1
SANITIZERS = -fsanitize=address,undefined -fno-omit-frame-pointer
GAME_SOURCES = $(filter-out ../src/main.c ../src/croot.c,$(wildcard ../src/*.c))
GAME_FLAGS = -DMINROG -DROGUE_NO_X11 $(shell pkg-config ncursesw --cflags)
GAME_LIBS = $(shell pkg-config ncursesw --libs)
TURN_WRAPPERS = update_status_line update_player_view read_game_key regenerate_health \
	screen_refresh run_recurring_actions run_delayed_actions search teleport \
	show_message random_below

.PHONY: test clean

test: test_env test_scores test_turns
	@run_dir=$$(mktemp -d /tmp/rogue-tests.XXXXXX) || exit 1; \
	status=0; \
	(cd "$$run_dir" && $(CURDIR)/test_env && $(CURDIR)/test_scores && $(CURDIR)/test_turns) || status=$$?; \
	rm -rf -- "$$run_dir"; \
	exit $$status

test_env: test_env.c ../src/options.c ../src/strings.c ../src/rogue.h ../src/platform.h
	$(CC) $(CFLAGS) $(SANITIZERS) test_env.c ../src/strings.c -o $@

test_scores: test_scores.c ../src/endgame.c ../src/game_state.c ../src/strings.c ../src/gameplay.c ../src/rogue.h ../src/platform.h ../src/screen.h ../src/screen_common.h
	$(CC) $(CFLAGS) $(SANITIZERS) -ffunction-sections -fdata-sections test_scores.c ../src/game_state.c ../src/strings.c ../src/gameplay.c -Wl,--gc-sections -o $@

test_game_main.o: ../src/main.c $(wildcard ../src/*.h)
	$(CC) $(CFLAGS) $(SANITIZERS) $(GAME_FLAGS) -Dmain=rogue_program_main -c $< -o $@

test_turns: test_turns.c test_game_main.o $(GAME_SOURCES) $(wildcard ../src/*.h)
	$(CC) $(CFLAGS) $(SANITIZERS) $(GAME_FLAGS) test_turns.c test_game_main.o $(GAME_SOURCES) \
		$(foreach symbol,$(TURN_WRAPPERS),-Xlinker --wrap=$(symbol)) $(GAME_LIBS) -o $@

clean:
	rm -f test_env test_scores test_turns test_game_main.o
