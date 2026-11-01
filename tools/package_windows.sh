#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the Windows x64 port and launcher and put a portable package in dist/.
# Run from anywhere, in an MSYS2 MINGW64 shell:
#   tools/package_windows.sh
# No disc image, BIOS, save, generated code or build output is copied.
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"

B=build/hyperduel
OUT=dist/Hyper-Duel-Windows-x64
MINGW_BIN=${MINGW_BIN:-/mingw64/bin}

make -j"$(nproc)"
make -j"$(nproc)" launcher

rm -rf "$OUT"
mkdir -p "$OUT/LICENSES"

cp "$B/hyperduel.exe" "$B/hyperduel-launcher.exe" "$OUT/"
for dll in SDL3.dll zlib1.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll libiconv-2.dll; do
    cp "$MINGW_BIN/$dll" "$OUT/"
done

cp LICENSE THIRD_PARTY_NOTICES.md README.md "$OUT/"
cp docs/STATUS.md docs/TESTING.md "$OUT/"
cp LICENSES/* "$OUT/LICENSES/"

cat > "$OUT/Play Hyper Duel.bat" <<'EOF'
@echo off
setlocal
cd /d "%~dp0"
start "" "%~dp0hyperduel-launcher.exe"
EOF

cat > "$OUT/FIRST-RUN.txt" <<'EOF'
Hyper Duel - Windows x64 portable package
==========================================

1. Put your own Hyper Duel (Japan) Saturn disc image (the .cue and its .bin files)
   anywhere you like. Do not copy it into this folder unless you want to.
2. Run "Play Hyper Duel.bat" (or hyperduel-launcher.exe).
3. On the launcher, choose the .cue file once, then press Play.
4. The launcher remembers the disc path. It only reads the image; it never copies or
   changes it.

The expected disc is Hyper Duel (Japan), product T-1809G, version V1.001.

Saves and settings live in %APPDATA%\Hyper Duel\ (backup.bin and settings.ini), not in
this folder. Logs are in %APPDATA%\Hyper Duel\logs\. Set HYPERDUEL_HOME to a folder to
keep them somewhere else.

Controls: keyboard arrows move; Return is Start; Z/X/C are A/B/C; A/S/D are X/Y/Z;
Q/E are L/R. On a gamepad, the lower face buttons are A/B/C, Start is Start. See the
launcher's Controllers page to change them.

At the title screen, press Start for Arcade mode; to open the mode menu, hold Down
and press Start (SATURN MODE / ARCADE MODE / OPTION).

This package contains no game data, no BIOS and no saves. See LICENSE and
THIRD_PARTY_NOTICES.md.
EOF

printf '%s\n' "package: $OUT"
find "$OUT" -maxdepth 2 -type f -printf '%P %s\n' | LC_ALL=C sort
