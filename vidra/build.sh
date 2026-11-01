#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
PREFIX="${PREFIX:-/usr}"
INSTALL=1
UNINSTALL=0
for a in "$@"; do
  case "$a" in --no-install) INSTALL=0 ;; --uninstall) UNINSTALL=1 ;; esac
done
if [ "$UNINSTALL" = 1 ]; then
  S=""; [ "$(id -u)" -ne 0 ] && S=sudo
  $S rm -f "$PREFIX/bin/vidra" "$PREFIX/share/applications/ai.vidra.Vidra.desktop"
  echo removido; exit 0
fi
for c in cmake ninja clang++ python3 pkg-config; do
  command -v "$c" >/dev/null || { echo "[x] falta $c"; exit 1; }
done
python3 tools/tokens_gen.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j "$(nproc)"
ctest --test-dir build --output-on-failure
if [ "$INSTALL" = 1 ]; then
  S=""; [ "$(id -u)" -ne 0 ] && S=sudo
  $S install -Dm755 build/vidra "$PREFIX/bin/vidra"
  $S install -Dm644 packaging/vidra.desktop "$PREFIX/share/applications/ai.vidra.Vidra.desktop"
  $S install -Dm644 packaging/vidra.svg "$PREFIX/share/icons/hicolor/scalable/apps/ai.vidra.Vidra.svg"
  $S update-desktop-database "$PREFIX/share/applications" 2>/dev/null || true
  echo "instalado: $PREFIX/bin/vidra"
else
  echo "binario: ./build/vidra"
fi
