CFLAGS = -std=c17 -Wall -Wextra -pedantic -g -O1
SANITIZERS = -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: test clean

test: test_env test_scores
	@run_dir=$$(mktemp -d /tmp/rogue-tests.XXXXXX) || exit 1; \
	status=0; \
	(cd "$$run_dir" && $(CURDIR)/test_env && $(CURDIR)/test_scores) || status=$$?; \
	rm -rf -- "$$run_dir"; \
	exit $$status

test_env: test_env.c ../src/options.c ../src/strings.c ../src/rogue.h ../src/platform.h
	$(CC) $(CFLAGS) $(SANITIZERS) test_env.c ../src/strings.c -o $@

test_scores: test_scores.c ../src/endgame.c ../src/game_state.c ../src/strings.c ../src/gameplay.c ../src/rogue.h ../src/platform.h ../src/screen.h ../src/screen_common.h
	$(CC) $(CFLAGS) $(SANITIZERS) -ffunction-sections -fdata-sections test_scores.c ../src/game_state.c ../src/strings.c ../src/gameplay.c -Wl,--gc-sections -o $@

clean:
	rm -f test_env test_scores
