#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
python3 tools/tokens_gen.py
OUT="vidra-source.zip"
rm -f "$OUT"
zip -r -9 "$OUT" . -x '.git/*' 'build/*' '*.zip' '*/__pycache__/*' '*/__pycache__/' '*.pyc' 'include/vidra/tokens.hpp' >/dev/null
echo "gerado: $OUT"
