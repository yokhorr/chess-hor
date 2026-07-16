#include "pawn.hpp"
#include "board.hpp"
#include "config.hpp"

std::unordered_set<Cell> Pawn::cellsToMove(Board const& board) const {
	std::unordered_set<Cell> ans;
	const Color myColor = getColor();
	const int32_t startPosition = myColor == Color::white ? 1 : 6;
	const auto forwardCell = getCell().forward(myColor);
	if (
		const auto doubleForwardCell = forwardCell.forward(myColor);
		getCell().y == startPosition && board.isFreeCell(forwardCell) && board.isFreeCell(doubleForwardCell)
	) {
		ans.insert(doubleForwardCell);
	}
	if (board.isFreeCell(forwardCell)) {
		ans.insert(forwardCell);
	}

	const auto forwardLeftCell = Cell(forwardCell.x - 1, forwardCell.y);
	const auto forwardRightCell = Cell(forwardCell.x + 1, forwardCell.y);
	const auto enPassantCell = board.getEnPassantCell();
	if (
		isValidCell(forwardLeftCell) &&
		((!board.isFreeCell(forwardLeftCell) && board.getPiece(forwardLeftCell)->getColor() != myColor)
			|| (enPassantCell != NCELL && enPassantCell == forwardLeftCell))
	) {
		ans.insert(forwardLeftCell);
	}
	if (
		isValidCell(forwardRightCell) &&
		((!board.isFreeCell(forwardRightCell) && board.getPiece(forwardRightCell)->getColor() != myColor)
			|| (enPassantCell != NCELL && enPassantCell == forwardRightCell))
	) {
		ans.insert(forwardRightCell);
	}

	return ans;
}

std::unordered_set<Cell> Pawn::cellsToAttack(const Board& board) const {
	const auto forwardCell = getCell().forward(getColor());
	const auto forwardLeftCell = Cell(forwardCell.x - 1, forwardCell.y);
	const auto forwardRightCell = Cell(forwardCell.x + 1, forwardCell.y);

	std::unordered_set<Cell> ans = cellsToMove(board);
	ans.erase(forwardCell);
	ans.insert({forwardLeftCell, forwardRightCell});

	return ans;
}
