# Testing

## Automated checks actually available

```sh
# Toolkit integrity (check-common needs a git work tree; see below)
make check-toolkit

# Interpreter against translated code, and two identical runs
export DISC="C:/path/to/Hyper Duel (Japan).cue"
make SR=../satrecomp check FRAMES=300
make SR=../satrecomp check FRAMES=6000 MOVIE=work/play3.txt
make SR=../satrecomp check FRAMES=6000 MOVIE=work/saturn3.txt
```

With the synced copy, `make check` stops at `check-common` because this project is not
a git work tree (no repository was created). Run the same checks directly:

```sh
GAME=hyperduel B=build/hyperduel DISC="C:/path/to/Hyper Duel (Japan).cue" \
  satrecomp/tools/check.sh 6000 work/saturn3.txt
```

The movies are local test inputs and are git-ignored. The movie format is one change
per line:

```
# frame pad1 pad2  (pressed bits, hex)
1200 0800 0000
1212 0000 0000
```

Start = `0800`, A = `0400`, B = `0100`, C = `0200`, Up = `1000`, Down = `2000`,
Left = `4000`, Right = `8000`.

## CD music and component tests

The two tests from the BGM fix package are in `tests/`:

```sh
SR=../satrecomp
g++ -std=c++20 -O2 -I "$SR/runtime" tests/test_music.cpp "$SR/runtime/music.cpp" -o work/test_music.exe
./work/test_music.exe

g++ -std=c++20 -O2 -include cassert -I "$SR" tests/test_cd.cpp \
  "$SR/machine/cd.cpp" "$SR/runtime/music.cpp" -o work/test_cd.exe
./work/test_cd.exe
```

`test_music` covers exact 588-sample sector playback, pause/resume, finite/infinite
repeat, serialization and the legacy track player. `test_cd` covers track:index and
FAD addressing, exclusive ends, repeat, pause/resume, completion interrupt, stop and
data-FIFO regression.

To trace the real game:

```sh
SATRECOMP_CD_LOG=1 SATRECOMP_MUSIC_LOG=1 SATRECOMP_SOUND_LOG=1 \
  build/hyperduel-dev/hyperduel.exe --disc "$DISC" --backup work/hyperduel/bgm.bin \
  --headless --frames 6000 --movie work/saturn3.txt

SATRECOMP_AUDIO_DUMP=work/bgm-out.raw SATRECOMP_CD_SOURCE_DUMP=work/bgm-src.raw \
  build/hyperduel-dev/hyperduel.exe --disc "$DISC" --backup work/hyperduel/bgm2.bin \
  --headless --frames 3000 --movie work/saturn3.txt
```

Expected Saturn trace: `PlaySectors FAD 112978..115679` (track 16) and
`8031..15382` (track 2), with `music: Pause`/`Resume` around CD commands. Arcade mode
issues no CD-DA track request in the tested path; its SCSP audio is unchanged.

## Headless run with screenshots

```sh
build/hyperduel/hyperduel.exe \
  --disc "C:/path/to/Hyper Duel (Japan).cue" \
  --backup work/hyperduel/backup.bin \
  --headless --frames 6000 --movie work/saturn3.txt \
  --shot-every 100:work/shots/frame --stats
```

Expected landmarks from the runs done here:

| Frame | Screen |
| --- | --- |
| ~300 | Technosoft logo |
| ~900-1200 | S-FORGEL Project |
| ~1500-2400 | Hyper Duel title |
| ~2500 | mode menu (when Down+Start is held at the title) |
| ~2800+ | Saturn Buster-Gear select or Arcade gameplay |
| ~3600+ | Saturn gameplay; Start pauses |
| ~3900 | Arcade gameplay with HUD, enemies and bullets |

Frame numbers are for the local disc and these exact movies; they are not universal.

## Manual checklist still needed

These were **not** performed interactively; they are the next validation steps.

### Arcade mode

- [ ] Title → Start → Arcade mode starts.
- [ ] Move in all eight directions; ship follows.
- [ ] A fires; B/C behaviour matches the Saturn port.
- [ ] Transform between Fighter and Armor; sprite, speed and shot pattern change.
- [ ] Pause with Start; unpause; menu remains usable.
- [ ] Collect P/T/G items; power-up and helper units appear.
- [ ] Clear stage 1, reach the boss, defeat it, see the stage transition.
- [ ] Game over / continue / ranking screen.
- [ ] Credits, score, high score and stage HUD are correct.

### Saturn mode

- [ ] Title: hold Down and press Start; mode menu appears.
- [ ] Select SATURN MODE; 3D player select appears.
- [ ] Select Keith, Lisa and Lloyd in turn; each starts with the right ship.
- [ ] Armor-mode lock-on / extra button behaves as the Saturn version.
- [ ] Difficulty, credits and rounds options in OPTION are selectable and persist.
- [ ] Stage 1 gameplay, boss and transition.

### Audio

- [ ] Music is present in the title, select and stage scenes.
- [ ] Sound effects match the actions (shots, explosions, pickups).
- [ ] No growing delay between picture and sound over ten minutes.
- [ ] CD audio does not restart or stutter at stage transitions.

### Saves and shutdown

- [ ] Change an option, quit cleanly, relaunch, confirm it persisted.
- [ ] High score / ranking persists in `backup.bin`.
- [ ] The launcher's save folder is writable and outside the install folder.
- [ ] Closing the window exits the port without leaving a process behind.

### Packaging

- [ ] Run `Play Hyper Duel.bat` from a folder whose path contains spaces.
- [ ] Run the port with a working directory different from its executable directory.
- [ ] Confirm no `disc/`, `*.cue`, `*.bin`, `backup.bin`, `work/` or `build/` is in the
      package.
- [ ] Confirm the launcher's "choose disc" check accepts T-1809G / V1.001 and rejects
      another game.
