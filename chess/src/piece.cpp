#include "piece.hpp"
#include "board.hpp"

std::unordered_set<Cell> Piece::slidingMoves(Board const& board, const std::vector<Cell> vecs) const {
	std::unordered_set<Cell> ans;

	for (const auto& vec : vecs) {
		Cell from = getCell();
		while (
			isValidCell(from + vec)
			&& (board.isFreeCell(from + vec) || board.getPiece(from + vec)->getColor() != getColor())
		) {
			ans.insert(from + vec);
			if (!board.isFreeCell(from + vec)) {
				break;
			}
			from = from + vec;
		}
	}

	return ans;
}
