# Rogue (study fork)

Study fork of [MestreLion/roguepc](https://github.com/MestreLion/roguepc), a modern port of the original IBM PC Rogue source. Gameplay uses ncurses; SDL2 displays the optional opening picture. The historical C and DOS assembly are retained for study.

## Build and run

Requires a C17 compiler, GNU Make, pkg-config, ncursesw development files, and X11 development files. SDL2 development files are needed for `make sdl` or `make all`.

On Debian/Ubuntu:

```sh
sudo apt install build-essential pkg-config libncurses-dev libx11-dev libsdl2-dev
make -j
make -j sdl
./roguepc
```

`make` builds `src/rogue`; `make sdl` builds `src/rogue-sdl` and the standalone `src/splash/splash` viewer. `make all` builds both variants. The same targets work with `make -C src`.

Use a UTF-8 locale and a terminal of at least 80 columns by 25 rows. To play in the current terminal:

```sh
cd src
./rogue
```

`./roguepc` opens xterm or GNOME Terminal when available, otherwise uses the current terminal. `./roguepc --no-splash` selects the terminal-only executable. Pass game arguments after `--`, for example `./roguepc --no-splash -- -s` to view scores. The GNOME launcher expects the `RoguePC` terminal profile; direct execution does not need it.

Launchers run repository binaries from `src/`, where their picture, options, and score files live. A custom executable selected with `--rogue CMD` keeps the caller's working directory. `ROGUE_PIC` overrides the SDL picture path; the default is `./rogue.pic`.

Enter a name or press Enter at startup. Use `h j k l` to move left/down/up/right and `y u b n` diagonally. `i` shows inventory, `?`/F1 lists commands, `/`/F2 lists symbols, and `Q` quits after confirmation.

For a build without X11, use `make ROGUE_NO_X11=1`. `ROGUE_RELEASE=1` enables optimization; `ROGUE_CHARSET=1` selects ASCII with ncurses. Run `make clean` before changing compiler or build options.

## Configuration and limitations

An optional `rogue.opt` in the game's working directory accepts `key=value` lines and comments beginning with `#`:

```ini
name=Rodney
fruit=Slime Mold
menu=on
scorefile=rogue.scr
```

Names and fruit are limited to 23 bytes, score/save filenames to 14 bytes, and keyboard macros to 40 bytes. Longer values are truncated. Option names, `menu`, and `screen` are case-insensitive; `screen=bw` requests monochrome output.

Saving (`S`) and restoring (`-r` or a save filename) are explicitly disabled in this port. The original implementation depends on DOS memory layouts and needs replacement. Scores work, but their native C-struct file format is platform-dependent. The fake DOS screen (`!`/F10) has known scrolling/input limitations; type `rogue` to return.

## Study and checks

[original_code_analysis](./original_code_analysis) contains a copy of [Decoded: Rogue](https://www.maizure.org/projects/decoded-rogue), including line-by-line explanations. Commit `dd9179d` is the original source import; later commits show the porting changes.

The maintained C port uses descriptive symbols and module names: [game_state.c](src/game_state.c) holds globals, [level_generation.c](src/level_generation.c) builds levels, [platform.c](src/platform.c) handles platform services, and [screen.c](src/screen.c) implements terminal rendering. The original assembly and archived analysis retain their historical names; the assembly is reference material and is not linked into the Linux game.

Comments distinguish source-proven behavior from unresolved historical intent. The BIOS timer hook uses vector `0x1C` (table byte offset `0x70`); the trap display flag has three states. Historical DOS checksum bounds and physical disk-protection behavior are not established by the native stubs.

`make test` runs configuration and score-file regression tests with AddressSanitizer and UndefinedBehaviorSanitizer. It requires a compiler with those sanitizers; tests use temporary files under `/tmp`.
