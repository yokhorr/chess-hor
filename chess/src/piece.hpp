#pragma once
#include <string>
#include <unordered_set>
#include "cell.hpp"
#include "color.hpp"
#include "config.hpp"
#include "pieceType.hpp"

class Board;

class Piece {
public:
	explicit Piece(const Color color) : color_(color) {
	}

	[[nodiscard]] Color getColor() const { return color_; }

	[[nodiscard]] Cell getCell() const { return cell_; }

	void setCell(Cell const& cell) {
		cell_ = cell;
	}

	[[nodiscard]] virtual std::unordered_set<Cell> cellsToMove(Board const& board) const = 0;

	[[nodiscard]] virtual std::unordered_set<Cell> cellsToAttack(Board const& board) const {
		return cellsToMove(board);
	}

	[[nodiscard]] virtual std::string getSign() const = 0;

	[[nodiscard]] virtual PieceType getType() const = 0;

	virtual ~Piece() = default;

	bool operator==(const Piece& other) const = default;

protected:
	[[nodiscard]] virtual std::unordered_set<Cell>
	slidingMoves(Board const& board, std::vector<Cell> vecs) const;

private:
	Color color_;
	Cell cell_ = NCELL;
};
