#pragma once

#include <string>
#include "piece.hpp"
#include "pieceType.hpp"

class Knight final : public Piece {
public:
	explicit Knight(const Color color) : Piece(color) {
	}

	[[nodiscard]] std::string getSign() const override {
		return getColor() == Color::white ? "N" : "n";
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	[[nodiscard]] PieceType getType() const override { return PieceType::knight; } // NOLINT(*-convert-member-functions-to-static)

	[[nodiscard]] std::unordered_set<Cell> cellsToMove(Board const& board) const override;

	[[nodiscard]] int32_t getValue() const override {
		return 3;
	}
};
