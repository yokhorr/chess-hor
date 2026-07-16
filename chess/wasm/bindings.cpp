// Embind-обвязка для WebAssembly. Сам движок (папка src/) не изменяется —
// здесь мы лишь публикуем функции из api.hpp в JavaScript.

#include <emscripten/bind.h>

#include "api.hpp"

EMSCRIPTEN_BINDINGS(chess_module) {
	emscripten::function("getStartFEN", &chessapi::getStartFEN);
	emscripten::function("getStatus", &chessapi::getStatus);
	emscripten::function("makeMove", &chessapi::makeMove);
	emscripten::function("getBotMove", &chessapi::getBotMove);
}
