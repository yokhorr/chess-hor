#include "knight.hpp"

#include "board.hpp"

std::unordered_set<Cell> Knight::cellsToMove(Board const& board) const {
	std::unordered_set<Cell> ans;

	const std::vector<Cell> vecs = {
		{+1, +2}, {-1, +2}, {-2, +1}, {-2, -1},
		{-1, -2}, {+1, -2}, {+2, -1}, {+2, +1}
	};
	const auto from = getCell();

	for (const auto& vec : vecs) {
		const Cell to = from + vec;
		if (isValidCell(to) && (board.isFreeCell(to) || board.getPiece(to)->getColor() != getColor())) {
			ans.insert(to);
		}
	}

	return ans;
}
