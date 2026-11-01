# Component tests for the CD-DA fix

These two tests came with the `Hyper-Duel-BGM-Fix` package. They isolate the music
player and the CD command implementation without booting the game.

From the project root in MSYS2 MINGW64:

```sh
SR=../satrecomp
g++ -std=c++20 -O2 -I "$SR/runtime" tests/test_music.cpp "$SR/runtime/music.cpp" -o work/test_music.exe
./work/test_music.exe

g++ -std=c++20 -O2 -include cassert -I "$SR" tests/test_cd.cpp \
  "$SR/machine/cd.cpp" "$SR/runtime/music.cpp" -o work/test_cd.exe
./work/test_cd.exe
```

`test_music` checks exact 588-sample sector playback, pause/resume, finite and
infinite repeat, serialization and the legacy track-based player. `test_cd` checks
track:index and FAD addressing, exclusive range ends, repeat, pause/resume, the
completion interrupt, stop, and that data-sector FIFO reads still work. The
`-include cassert` is needed because the packaged test omits `<cassert>`.
