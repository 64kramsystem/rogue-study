TODO
===============================================================================
- Replace the DOS memory-dump save/restore implementation with explicit game-state serialization.
- Replace the native-struct score format with a portable format.
- Fix fake DOS scrolling and non-ASCII exit handling (see `src/fakedos.c`).
- Consider embedding `rogue.pic` data as a `.c` source file: `xxd rogue.pic > rogue_pic.c`
