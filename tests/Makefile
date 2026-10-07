CFLAGS = -std=c17 -Wall -Wextra -pedantic -g -O1
SANITIZERS = -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: test clean

test: test_env test_scores
	@run_dir=$$(mktemp -d /tmp/rogue-tests.XXXXXX) || exit 1; \
	status=0; \
	(cd "$$run_dir" && $(CURDIR)/test_env && $(CURDIR)/test_scores) || status=$$?; \
	rm -rf -- "$$run_dir"; \
	exit $$status

test_env: test_env.c ../src/env.c ../src/strings.c ../src/rogue.h ../src/extern.h
	$(CC) $(CFLAGS) $(SANITIZERS) test_env.c ../src/strings.c -o $@

test_scores: test_scores.c ../src/rip.c ../src/extern.c ../src/strings.c ../src/misc.c ../src/rogue.h ../src/extern.h ../src/curses.h ../src/curses_common.h
	$(CC) $(CFLAGS) $(SANITIZERS) -ffunction-sections -fdata-sections test_scores.c ../src/extern.c ../src/strings.c ../src/misc.c -Wl,--gc-sections -o $@

clean:
	rm -f test_env test_scores
