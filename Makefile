.DEFAULT_GOAL := default
.PHONY: default all sdl splash test clean

default all sdl splash:
	$(MAKE) -C src $@

test:
	$(MAKE) -C tests test

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
