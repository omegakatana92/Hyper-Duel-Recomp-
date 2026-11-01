# Development log

## 2026-11-01 — first boot and gameplay

### Inspection

- Toolkit: the installed satrecomp 0.2.0 source tree. It has no `.git` directory, so a
  pristine git snapshot (excluding the user's `games/` disc images) was made in a
  sibling development checkout and used as the editable toolkit.
- Read `README.md`, `docs/NEXT_GAME.md`, `MACHINE.md`, `VIDEO.md`, `SOUND.md`,
  `VERIFYING.md`, `build.mk`, `CMakeLists.txt`, `tools/sync-toolkit.sh`,
  `tools/check.sh`, and the launcher sources. `example/` is absent from the installed
  0.2.0 source, so the game repository was built from the installed `build.mk`
  conventions.
- Build environment: MSYS2 MINGW64, GCC 16.2.0, CMake 4.4.4, Ninja 1.13.2,
  SDL3 3.4.18, zlib 1.3.2. This matches the toolkit's Windows CI package set.
- Disc: all 36 tracks named by the CUE exist. Header: `T-1809G`, `V1.001`,
  `19961002`, area `J`.

### Project

- Created this repository with `GAME=hyperduel`, `SR=satrecomp`, a git-ignored
  `local.mk` holding the absolute CUE path, the game configuration, a minimal
  `games/hyperduel/src/game.cpp`, and distribution notices.
- The toolkit copy in `satrecomp/` was made by `sync-toolkit.sh` from the development
  checkout; `make check-toolkit` passes.

### First boot

- Interpreter-only boot reached `0KERNEL.BIN`, then stalled polling CD command
  `0x51` (GetSectorNumber), which the toolkit stubbed to zero.
- Implemented, in the separate toolkit checkout, the CD buffer commands
  `0x50`-`0x53` and `0x60`-`0x63`, sector storage, the data port (including the
  `0x05818000` alias used by SCU DMA), and an environment-gated command log
  (`SATRECOMP_CD_LOG=1`). The game then read its files through its own GFS loader.
- Next blocker: indirect call to BIOS pointer `0x0600026C` (stub, harmless) followed
  by `GAME.BIN` execution. That was not a crash; the game reached the Technosoft logo.

### BGM fix (CD-DA)

The requested `Hyper-Duel-BGM-Fix` package arrived locally. Its patch conflicted with
the local CD-buffer work, so it was merged into the editable checkout instead of
force-applied.
The merged behaviour:

- CD command `0x10` now distinguishes FAD+length parameters (bit 23) from normal
  `track:index` parameters. The latter resolve to the track's FAD range, not a masked
  FAD.
- CD-DA is read as raw 2352-byte audio sectors through
  `music::SetSectorReader`/`music::PlaySectors`, and consumed at exactly 588 stereo
  samples per sector on the SCSP's 44.1 kHz clock.
- Audio playback never fills the data FIFO; the existing data-read path is unchanged.
- Pause, all-ones resume, stop, finite/infinite repeat, and seek cancellation are
  handled. `Disc::Track::endFAD` excludes the next track's 2-second pregap.
- The new music range fields are serialized. Pre-patch save states are not compatible.

Diagnostics are opt-in: `SATRECOMP_CD_LOG=1` (commands, track ranges, FAD playback),
`SATRECOMP_MUSIC_LOG=1` (play/pause/resume/stop, sample counts, sector failures),
`SATRECOMP_SOUND_LOG=1` (CD input, SCSP routing/gain), `SATRECOMP_SCSP_LOG=1`
(key-ons), `SATRECOMP_CD_SOURCE_DUMP=path` and `SATRECOMP_AUDIO_DUMP=path`.
`SATRECOMP_CD_DIRECT=1` is a diagnostic only; it is not used by normal builds.

Results:

- `tests/test_music.cpp` and `tests/test_cd.cpp` from the package pass (the latter
  needs `-include cassert` with this GCC).
- `saturn3` 6000 frames: interpreter == translated; two runs equal.
  `PlaySectors FAD 112978..115679` (track 16) and `8031..15382` (track 2).
  The output correlates 0.54 with the dumped CD source; the source matches the actual
  track 16/2 bytes.
- `saturn4` 12000 frames: tracks 16 → 2 → 36; pause/resume commands observed.
- `arcade4` 6000 frames: Arcade audio is byte-identical to the pre-fix build (SCSP BGM
  and SFX unchanged); no CD-DA track request was observed in Arcade mode.
- 20 Saturn and 80 Arcade screenshots are byte-identical to the pre-fix build,
  confirming no graphics or input regression.
- The synced/default build (`build/hyperduel/hyperduel.exe`) passed `check.sh` on
  `saturn3` and `play3` for 6,000 frames (interpreter == translated, two runs equal)
  and played the same CD-DA ranges. The Windows package was rebuilt from it and tested
  from a path with spaces.

### Validation reached

- Screenshots: Technosoft logo (frame 300), S-FORGEL screen (900-1200), Hyper Duel
  title (1500-2400), mode menu (2,450 bytes, frame 2500), Saturn 3D player select
  (2800+), Arcade and Saturn gameplay, pause, game over.
- Input movies `play3.txt` (Arcade path) and `saturn3.txt` (Saturn path) exercise
  Start, A/B, directions and pause.
- `make check FRAMES=6000 MOVIE=work/play3.txt` and
  `... MOVIE=work/saturn3.txt`: interpreter and translated code identical; two runs
  identical.
- Translated build: 19,290 blocks, 2,150 multi-block functions, 257,998 instructions.
  Saturn movie: 100.00% recompiled, 0 interpreted. 6,000 frames in ~32 s headless
  software rendering.
- Audio dump: 50.16 s, non-silent logo and gameplay sections.
