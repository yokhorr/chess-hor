#pragma once

#include <string>
#include "piece.hpp"
#include "pieceType.hpp"

class Bishop final : public Piece {
public:
	explicit Bishop(const Color color) : Piece(color) {
	}

	[[nodiscard]] std::string getSign() const override {
		return getColor() == Color::white ? "B" : "b";
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	[[nodiscard]] PieceType getType() const override { return PieceType::bishop; } // NOLINT(*-convert-member-functions-to-static)

	[[nodiscard]] std::unordered_set<Cell> cellsToMove(Board const& board) const override {
		return slidingMoves(board, directionVecs);
	}

	inline static std::vector<Cell> directionVecs = {
		{-1, -1},
		{-1, +1},
		{+1, -1},
		{+1, +1}
	};

	[[nodiscard]] int32_t getValue() const override {
		return 3;
	}
};
