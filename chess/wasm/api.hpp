#pragma once

// Тонкая обёртка над движком (Board/BotPlayer/...), не изменяющая и не дублирующая
// его логику. Все функции принимают/возвращают простые std::string (FEN и JSON),
// что позволяет использовать их как из Embind (WebAssembly), так и из обычного
// нативного main() для тестирования без Emscripten.
//
// Формат ответа единообразный:
//   {"ok":true, ...поля...}
//   {"ok":false,"error":"текст ошибки"}

#include <string>

namespace chessapi {

// Стартовая позиция (делегирует Board::setStart()).
std::string getStartFEN();

// Возвращает статус позиции: чей ход, шах ли, исход партии, список легальных ходов
// (сгруппированных по from/to с массивом возможных превращений).
std::string getStatus(const std::string& fen);

// Пытается совершить ход. Возвращает новую позицию и метаданные хода
// (взятие, рокировка, взятие на проходе, превращение) для анимации/звуков на фронтенде.
std::string makeMove(
    const std::string& fen,
    const std::string& from,
    const std::string& to,
    const std::string& promotion
);

// Просит движковый BotPlayer выбрать ход для текущей стороны (не совершая его).
std::string getBotMove(const std::string& fen);

} // namespace chessapi
