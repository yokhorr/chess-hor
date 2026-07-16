#include <unordered_set>

#include "king.hpp"
#include "board.hpp"
#include "config.hpp"
#include "piece.hpp"

std::unordered_set<Cell> King::cellsToMove(Board const& board) const {
	auto ans = cellsToAttack(board);

	const auto [kingsideCastleRight, queensideCastleRight] = board.checkCastleRight(getColor());

	if (kingsideCastleRight) {
		ans.insert(kingsideCastleCell(getColor()));
	}
	if (queensideCastleRight) {
		ans.insert(queensideCastleCell(getColor()));
	}

	return ans;
}

std::unordered_set<Cell> King::cellsToAttack(Board const& board) const {
	std::unordered_set<Cell> ans;

	for (int32_t dy = -1; dy <= 1; ++dy) {
		for (int32_t dx = -1; dx <= 1; ++dx) {
			if (dx == 0 && dy == 0) continue;

			const auto cell = Cell(getCell().x + dx, getCell().y + dy);
			if (!isValidCell(cell)) continue;

			if (board.isFreeCell(cell) || board.getPiece(cell)->getColor() != getColor()) {
				ans.insert(cell);
			}
		}
	}

	return ans;
}

// to config.hpp? though only king needs it
Cell King::kingsideCastleCell(Color const& color) {
	return kingsideKnightCell(color);
}

Cell King::queensideCastleCell(Color const& color) {
	return queensideBishopCell(color);
}
