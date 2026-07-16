#pragma once

#include "cell.hpp"

inline constexpr int32_t BOARD_SIZE = 8;
inline const Cell NCELL(-1, -1);

inline bool isValidCell(Cell const& cell) {
	return 0 <= cell.x && cell.x < BOARD_SIZE && 0 <= cell.y && cell.y < BOARD_SIZE;
}

inline int32_t pawnRank(Color const& color) {
	return color == Color::white ? 1 : 6;
}

inline int32_t lastRank(Color const& color) {
	return color == Color::white ? 7 : 0;
}

inline Cell kingCell(Color const& color) {
	return color == Color::white ? Cell("e1") : Cell("e8");
}

inline Cell queenCell(Color const& color) {
	return color == Color::white ? Cell("d1") : Cell("d8");
}

inline Cell kingsideBishopCell(Color const& color) {
	return color == Color::white ? Cell("f1") : Cell("f8");
}

inline Cell kingsideKnightCell(Color const& color) {
	return color == Color::white ? Cell("g1") : Cell("g8");
}

inline Cell kingsideRookCell(Color const& color) {
	return color == Color::white ? Cell("h1") : Cell("h8");
}

inline Cell queensideBishopCell(Color const& color) {
	return color == Color::white ? Cell("c1") : Cell("c8");
}

inline Cell queensideKnightCell(Color const& color) {
	return color == Color::white ? Cell("b1") : Cell("b8");
}

inline Cell queensideRookCell(Color const& color) {
	return color == Color::white ? Cell("a1") : Cell("a8");
}
