# Building

## Windows 11 x64

Use an **MSYS2 MINGW64** shell. The package set is the one satrecomp's Windows CI
uses:

```sh
pacman -S --needed make git \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-python mingw-w64-x86_64-sdl3 mingw-w64-x86_64-zlib
```

From the project root:

```sh
make -j$(nproc)          # translated code (if any) + build/hyperduel/hyperduel.exe
make -j$(nproc) launcher # build/hyperduel/hyperduel-launcher.exe
```

`make` uses the toolkit copy in `satrecomp/`, which must be the copy made by
`satrecomp/tools/sync-toolkit.sh` (`make check-toolkit` verifies it). To develop the
toolkit itself, build against the separate checkout:

```sh
make SR=../satrecomp -j$(nproc)
```

That writes to `build/hyperduel-dev/` instead of `build/hyperduel/`.

## Disc path

The disc is never part of the repository. Put the absolute path in the git-ignored
`local.mk`:

```make
DISC := C:/path/to/Hyper Duel (Japan)/Hyper Duel (Japan).cue
```

`make run`, `make record` and `make check` then use it. For `make check`, also export
`DISC` if the check script asks for it:

```sh
export DISC="C:/path/to/Hyper Duel (Japan).cue"
make SR=../satrecomp check FRAMES=600 MOVIE=work/play3.txt
```

The plain `make check` (synced `SR=satrecomp`) also runs `check-common`, whose
`sync-common.sh` calls `git -C` and therefore needs the game directory to be a git
work tree. This project intentionally has no created repository. Use the development
toolkit form above, or run the check script directly against either build:

```sh
GAME=hyperduel B=build/hyperduel DISC="C:/path/to/Hyper Duel (Japan).cue" \
  satrecomp/tools/check.sh 6000 work/saturn3.txt
```

## Linux x86_64 (not yet tested)

The same CMake targets build on Linux. The package list satrecomp's CI installs:

```sh
sudo apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build pkg-config zlib1g-dev \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev \
  libxtst-dev libxkbcommon-dev libwayland-dev wayland-protocols libdecor-0-dev \
  libegl-dev libgl-dev libvulkan-dev libpulse-dev libasound2-dev libudev-dev \
  libdbus-1-dev libpipewire-0.3-dev
```

Install SDL 3 (system package or the toolkit's pinned 3.4.16 source build), then:

```sh
make -j"$(nproc)"
make -j"$(nproc)" launcher
```

`make app` is the **macOS app bundle** target in satrecomp 0.2.0; it does not create a
Linux AppImage. A Steam Deck AppImage still needs a packaging step (AppDir + linuxdeploy
or equivalent) and a real Linux test. Do not claim Steam Deck compatibility from a
cross-build alone.

## Generated code

`work/hyperduel/blocks.txt` is the block database (git-ignored and never shipped).
`work/hyperduel/gen/blocks_*.cpp` is generated from it. It is large (roughly 250k
instructions for the recorded paths) and takes about 13 minutes to compile with GCC
16 at `-O3` on this machine. The generated code is game-derived and must not be
committed or distributed.

## Packaging

```sh
tools/package_windows.sh
```

produces `dist/Hyper-Duel-Windows-x64/` with the port, launcher, the MinGW/SDL/zlib
runtime DLLs, license texts and first-run instructions. It copies no disc image,
BIOS, save or build output.
