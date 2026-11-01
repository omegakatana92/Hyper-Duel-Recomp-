#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the Linux x86_64 port and launcher and package a Steam Deck AppImage.
#
# Requires a Linux x86_64 environment with the project's dependencies, a working
# SDL3 CMake package (static SDL3 preferred) and appimagetool:
#   APPIMAGETOOL=/path/to/appimagetool tools/package_appimage.sh
#
# Build on the oldest glibc baseline you intend to support. The release AppImage is
# built on Ubuntu 22.04 (glibc 2.35), which is older than current SteamOS.
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"

GAME=hyperduel
B=build/$GAME
OUT=dist/Hyper-Duel-v0.1.0-SteamDeck-x86_64.AppImage
APPDIR=dist/Hyper-Duel.AppDir
APPIMAGETOOL=${APPIMAGETOOL:-appimagetool}

make -j"$(nproc)"
make -j"$(nproc)" launcher

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" \
         "$APPDIR/usr/share/licenses/hyper-duel"

cp "$B/$GAME" "$B/$GAME-launcher" "$APPDIR/usr/bin/"
cp assets/hyper-duel.png "$APPDIR/hyper-duel.png"
cp assets/hyper-duel.png "$APPDIR/usr/share/icons/hicolor/256x256/apps/hyper-duel.png"
cp assets/hyper-duel.desktop "$APPDIR/hyper-duel.desktop"
cp assets/hyper-duel.desktop "$APPDIR/usr/share/applications/hyper-duel.desktop"

# C++ runtime is the only non-base runtime library we add here. SDL3 is linked
# statically; X11/Wayland/audio/GL are provided by SteamOS (and are not replaced by
# the AppImage, so graphics drivers and libc stay the host's).
needed=$(for exe in "$B/$GAME" "$B/$GAME-launcher"; do
    ldd "$exe" 2>/dev/null
done | awk '{print $1}' | sort -u)
for lib in libstdc++.so.6 libgcc_s.so.1 libatomic.so.1 libz.so.1; do
    if printf '%s\n' "$needed" | grep -qx "$lib"; then
        p=$(ldconfig -p 2>/dev/null | awk -v n="$lib" '$1 == n {print $NF; exit}')
        [ -n "$p" ] && cp -L "$p" "$APPDIR/usr/lib/"
    fi
done
for exe in "$APPDIR/usr/bin/$GAME" "$APPDIR/usr/bin/$GAME-launcher"; do
    patchelf --set-rpath '$ORIGIN/../lib' "$exe"
done

cp LICENSE THIRD_PARTY_NOTICES.md "$APPDIR/usr/share/licenses/hyper-duel/"
cp LICENSES/* "$APPDIR/usr/share/licenses/hyper-duel/"
cp satrecomp/LICENSE "$APPDIR/usr/share/licenses/hyper-duel/satrecomp-LICENSE"

cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
# AppImage entry point: everything is relative to APPDIR, never the current directory.
HERE=$(dirname "$(readlink -f "$0")")
APPDIR=${APPDIR:-$HERE}
export APPDIR
export LD_LIBRARY_PATH="$APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$APPDIR/usr/bin/hyperduel-launcher" "$@"
EOF
chmod +x "$APPDIR/AppRun"

ARCH=x86_64 VERSION=0.1.0 "$APPIMAGETOOL" --no-appstream "$APPDIR" "$OUT"
chmod +x "$OUT"
echo "built $OUT"
