#!/bin/bash
# Builds the mod from scratch and packs the zip for the SD card.
#   ./build.sh            -> dist/letssing2026_saltar_intro.zip (and dist/atmosphere/...)
# Needs: git, make, python3 and devkitA64 with npdmtool (devkitPro; DEVKITPRO defaults to /opt/devkitpro).
set -euo pipefail

EXLAUNCH_URL="${EXLAUNCH_URL:-https://github.com/shadowninja108/exlaunch.git}"   # or a local clone of it
EXLAUNCH_COMMIT="f9f4b0d"
PROGRAM_ID="0100EE4020D18000"

ROOT="$(cd "$(dirname "$0")" && pwd)"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
NPDMTOOL="$(command -v npdmtool || echo "$DEVKITPRO/tools/bin/npdmtool")"

case "$ROOT" in
    *"'"*) echo "The folder path has an apostrophe ($ROOT): exlaunch's Makefile can't build there. Move the folder." >&2
           exit 1 ;;
esac
[ -x "$DEVKITPRO/devkitA64/bin/aarch64-none-elf-gcc" ] || { echo "devkitA64 not found in $DEVKITPRO" >&2; exit 1; }
[ -x "$NPDMTOOL" ] || { echo "npdmtool not found (devkitPro switch-tools)" >&2; exit 1; }

BUILD="$ROOT/build/skipintro"   # the folder name is the module name (crash reports show "skipintro + 0x...")
if [ ! -d "$BUILD/.git" ]; then
    mkdir -p "$ROOT/build"
    git clone --quiet "$EXLAUNCH_URL" "$BUILD"
fi
git -C "$BUILD" checkout --quiet --force "$EXLAUNCH_COMMIT"
git -C "$BUILD" clean --quiet -fdx
git -C "$BUILD" apply "$ROOT/exlaunch.patch"
cp "$ROOT/source/main.cpp" "$BUILD/source/program/main.cpp"
make -C "$BUILD" -j"$(nproc)"

OUT="$ROOT/dist/atmosphere/contents/$PROGRAM_ID/exefs"
rm -rf "$ROOT/dist"
mkdir -p "$OUT"
cp "$BUILD/deploy/subsdk9" "$OUT/subsdk9"
"$NPDMTOOL" "$ROOT/npdm/main_npdm.json" "$OUT/main.npdm"
python3 - "$ROOT/dist" <<'PY'
import os, sys, zipfile
dist = sys.argv[1]
with zipfile.ZipFile(os.path.join(dist, 'letssing2026_saltar_intro.zip'), 'w', zipfile.ZIP_DEFLATED) as z:
    for folder, _, files in os.walk(os.path.join(dist, 'atmosphere')):
        z.write(folder, os.path.relpath(folder, dist))
        for name in files:
            z.write(os.path.join(folder, name), os.path.relpath(os.path.join(folder, name), dist))
PY
echo "Done: $ROOT/dist/letssing2026_saltar_intro.zip"
