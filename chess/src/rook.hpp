#pragma once

#include <string>
#include "piece.hpp"
#include "pieceType.hpp"

class Rook final : public Piece {
public:
	explicit Rook(const Color color) : Piece(color) {
	}

	[[nodiscard]] std::string getSign() const override {
		return getColor() == Color::white ? "R" : "r";
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	[[nodiscard]] PieceType getType() const override { return PieceType::rook; } // NOLINT(*-convert-member-functions-to-static)

	[[nodiscard]] std::unordered_set<Cell> cellsToMove(Board const& board) const override {
		return slidingMoves(board, directionVecs);
	}

	inline static std::vector<Cell> directionVecs = {
		{-1, 0},
		{+1, 0},
		{0, -1},
		{0, +1}
	};
};