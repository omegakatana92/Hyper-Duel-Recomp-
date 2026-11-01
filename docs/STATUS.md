# Hyper Duel recompilation — status

Updated: 2026-11-01 (local build)

## What works

- **Boot:** the native BIOS path (`runtime/hle_bios.cpp`) starts the disc, the CD file
  system reads the ISO 9660 data track, and `0KERNEL.BIN` loads `GAME.BIN`.
- **Title and modes:** the Technosoft / S-FORGEL screens, the Hyper Duel title, the
  mode menu (`SATURN MODE`, `ARCADE MODE`, `OPTION`) and both modes' player select and
  gameplay have been reached and captured.
- **Arcade mode:** title → character/gameplay path, enemies, bullets, explosions, HUD,
  pause and game over.
- **Saturn mode:** mode select → 3D Buster-Gear player select → gameplay with pause,
  movement and fire input.
- **Controls:** Start, A/B/C, directions and pause were exercised through input movies.
- **Audio:** the SCSP/68000 output is non-silent during logo and gameplay. Arcade BGM is
  SCSP-driven; Saturn BGM is CD-DA. The CD-DA path now plays raw audio sectors on the
  sound generator's 44.1 kHz clock, separately from the data FIFO.
- **Translation:** 19,290 SH-2 blocks (2,150 multi-block functions, 257,998
  instructions) are translated for the recorded paths.

## Checks actually run

| Check | Command | Result |
| --- | --- | --- |
| ROM/disc header | read the first sector with a script | `T-1809G`, `V1.001`, `19961002`, area `J` |
| CUE tracks | PowerShell parse of the CUE | 36/36 `.bin` files present |
| Build | `make SR=../satrecomp` | succeeds (Windows x64, MSYS2 MINGW64) |
| Boot equivalence | `make SR=../satrecomp check FRAMES=300` | interpreter == translated; two runs equal |
| Arcade path equivalence | `make SR=../satrecomp check FRAMES=6000 MOVIE=work/play3.txt` | interpreter == translated; two runs equal |
| Saturn path equivalence | `make SR=../satrecomp check FRAMES=6000 MOVIE=work/saturn3.txt` | interpreter == translated; two runs equal |
| Coverage | `--stats` on the translated build | `saturn3`, 6,000 frames: 100.00% recompiled, 0 interpreted |
| Screenshots | `--shot-every` | title, mode select, player select, Arcade gameplay, Saturn gameplay, pause |
| Default synced build | `build/hyperduel/hyperduel.exe --headless --frames 600` | 600 frames, 100.00% recompiled, 0 interpreted |
| Default synced check | `satrecomp/tools/check.sh 6000 work/saturn3.txt` with `B=build/hyperduel` | interpreter == translated; two runs equal |
| Package port | run from `Hyper Duel Package Test` with CWD `C:\Windows\Temp` | 300 frames, 100.00% recompiled, no missing DLLs |
| Package launcher | `--screenshot` with the real CUE | disc shown as `Hyper Duel (Japan), verified.`; controller detected |
| Disc check | same launcher with a Rabbit CUE | rejected as "a different game (RABBIT)"; Play disabled |
| CD-DA component test | provided `tests/test_music.cpp` | sector PCM, exact 588 samples, pause/resume, finite/infinite loops, serialization |
| CD command component test | provided `tests/test_cd.cpp` | track/FAD play, exclusive end, repeat, pause/resume, data FIFO regression |
| Saturn BGM | `saturn3` + CD/music/audio dumps | `PlaySectors FAD 112978..115679` then `8031..15382`; output/source correlation 0.54 |
| Saturn transitions | `saturn4` 12,000 frames | tracks 16 → 2 → 36; BGM pauses/resumes with the game's CD commands |
| Arcade BGM/SFX regression | `arcade4` audio dump | byte-identical to the pre-patch Arcade audio; SCSP BGM and SFX unchanged |
| Graphics/input regression | 20 Saturn + 80 Arcade PNGs | all identical to the pre-BGM-fix build |
| Linux build | Ubuntu 22.04 WSL2, GCC 11.4, static SDL3 3.4.16 | port + launcher build; max ELF symbol `GLIBC_2.34` |
| Linux checks | `check.sh 6000` on `saturn3` and `play3` | interpreter == translated; two runs equal |
| Linux BGM | `saturn3` audio/source dumps | same FAD ranges; output/source correlation 0.5445 |
| Linux Arcade | `arcade4` audio dump | byte-identical to the Windows Arcade audio |
| AppImage extract/run | `--appimage-extract` and `--appimage-extract-and-run` | both work; 600-frame boot 100% recompiled |
| AppImage BGM | extracted bundled port, `saturn3` | `PlaySectors FAD 112978..115679`; correlation 0.5445 |
| AppImage launcher | `AppRun --screenshot` under Xvfb | disc shown as verified; Play enabled |
| AppImage launch-to-runtime | `AppRun --play` under Xvfb | port launched with external CUE, saves and settings paths |

The `make check` runs used `DISC` pointing at the local cue. With the development
toolkit (`SR=../satrecomp`) `make check` passes. With the synced copy, the plain
`make check` target stops at `check-common` because this project is intentionally not a
git work tree; running the same `satrecomp/tools/check.sh` directly (as above) passes.
`make check-rewrites` currently prints that no rewritten routines exist; that is a
placeholder, not a validation of rewrites.

## Package built

`dist/Hyper-Duel-Windows-x64/` contains `hyperduel.exe`, `hyperduel-launcher.exe`,
`SDL3.dll`, `zlib1.dll`, the MinGW runtime DLLs, `LICENSE`, `THIRD_PARTY_NOTICES.md`,
`LICENSES/`, `FIRST-RUN.txt`, `Play Hyper Duel.bat`, `README.md`, `STATUS.md` and
`TESTING.md`. It contains no disc image, BIOS, save, generated C++ or build output.
`dist/Hyper-Duel-Windows-x64.zip` is the same tree.

- Port SHA-256: `E1DDA24B291DD770B908B47029093804272A1A699BA781C02DB202F648A5D79B`
- Launcher SHA-256: `089A0705ACEA186D0D146495122F0DD291499D9E59F40A11D002CBAB3BA71E0C`

The package was run from a path containing spaces with a working directory different
from the executable's; Saturn CD-DA started (track 16 FAD 112978..115679) and Arcade
audio was byte-identical to the pre-fix build.

The Steam Deck candidate is:

- `dist/Hyper-Duel-v0.1.0-SteamDeck-x86_64.AppImage`
- SHA-256: `1113E77F8F8DFD2DB1EC298F57A339BA11CAF9B48A6A8868C684F1D5C6DCF70B`

It is built on Ubuntu 22.04 (glibc 2.35, GCC 11.4) with static SDL3 3.4.16. Its ELF
symbols require at most `GLIBC_2.34`; it bundles `libstdc++`, `libgcc_s` and `libz`,
and uses the host's glibc, Mesa/graphics drivers and X11/Wayland/audio libraries.
The AppImage is delivered for Steam Deck testing but is not published.

## Not yet validated

- Full playthroughs of all eight stages, bosses, endings, both loops and both modes.
- An in-game CD-DA loop boundary longer than the captured stage: the component test
  covers finite/infinite looping, and tracks 16 → 2 → 36 were observed, but a full
  stage that reaches a track's last sector was not captured.
- Persistent saves/high scores (the backup file is written; its in-game save/load path
  has not been exercised).
- VDP2 colour calculation: the software renderer reports roughly 70-80 lines per frame
  drawn without the requested effect. Sprites, backgrounds, scrolling, priorities and
  colour offset are drawn; transparency between VDP2 layers is approximate.
- Reference verification against Ymir: the unpublished reference patch was not found
  locally, so no `make verify` was claimed or performed.
- A windowed session with the real SDL audio device, frame pacing and audio underruns.
- Actual Steam Deck hardware: Gaming Mode/gamescope, Deck controller bindings, Deck
  audio device and long interactive play. The AppImage itself was built and tested in
  WSL2 only.

## Known limitations in the toolkit changes

The CD-block buffer commands (`0x50`-`0x53`, `0x60`-`0x63`) are implemented in the
separate toolkit checkout and copied here by `sync-toolkit.sh`. They model one buffer
partition and the sector lengths Hyper Duel asks for (2,048-byte user data). Multiple
partitions, filters and raw-sector modes are not modelled. This was enough for Hyper
Duel's GFS loader; it has not been regression-tested with Fighting Vipers (whose disc
is not available here).

## Next concrete actions

1. Play through Arcade and Saturn modes interactively; record the remaining blocks and
   run `make check` on each movie.
2. Implement/validate VDP2 colour calculation for the lines Hyper Duel requests.
3. Identify the CD-DA play/stop calls and route them through `runtime/music.h`, or
   confirm the Saturn soundtrack is SCSP-only.
4. Exercise save/load and ranking screens, then verify `backup.bin` persistence.
5. Rebuild the synced/default target and assemble the Windows package.
6. Prepare Linux/Steam Deck build only after the Windows package is validated.
