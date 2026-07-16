#pragma once

#include <string>

#include "piece.hpp"
#include "pieceType.hpp"

class King final : public Piece {
public:
	explicit King(const Color color) : Piece(color) {
	}

	[[nodiscard]] std::string getSign() const override {
		return getColor() == Color::white ? "K" : "k";
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	[[nodiscard]] PieceType getType() const override { return PieceType::king; } // NOLINT(*-convert-member-functions-to-static)

	[[nodiscard]] std::unordered_set<Cell> cellsToMove(Board const& board) const override;

	[[nodiscard]] std::unordered_set<Cell> cellsToAttack(Board const& board) const override;

	static Cell kingsideCastleCell(Color const& color);

	static Cell queensideCastleCell(Color const& color);

	[[nodiscard]] int32_t getValue() const override {
		return 0;
	}
};
