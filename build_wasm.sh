#!/usr/bin/env bash
# Локальная сборка WebAssembly-модуля движка (требует установленного и
# активированного Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html
#
# Использование:
#   source /путь/к/emsdk/emsdk_env.sh
#   ./build_wasm.sh
#
# После сборки откройте web/ через любой локальный статический HTTP-сервер
# (например: `python3 -m http.server --directory web 8080`) и перейдите на
# http://localhost:8080 — открывать index.html напрямую через file:// нельзя,
# так как ES-модули и WebAssembly требуют загрузки по http(s).

set -euo pipefail

if ! command -v emcmake >/dev/null 2>&1; then
	echo "Не найдена команда emcmake. Установите и активируйте Emscripten SDK:" >&2
	echo "  git clone https://github.com/emscripten-core/emsdk.git" >&2
	echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest" >&2
	echo "  source ./emsdk_env.sh" >&2
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

emcmake cmake -S chess -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm --target chess_wasm -j

mkdir -p web/engine
cp build-wasm/chess.js web/engine/chess.js
cp build-wasm/chess.wasm web/engine/chess.wasm

echo
echo "Готово. Модуль скопирован в web/engine/."
echo "Запустите локальный сервер, например:"
echo "  python3 -m http.server --directory web 8080"
echo "и откройте http://localhost:8080"
