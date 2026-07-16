#pragma once
#include "piece.hpp"
#include "pieceType.hpp"

class Pawn final : public Piece {
public:
	explicit Pawn(const Color color) : Piece(color) {
	}

	[[nodiscard]] std::string getSign() const override {
		return getColor() == Color::white ? "P" : "p";
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	[[nodiscard]] PieceType getType() const override { return PieceType::pawn; } // NOLINT(*-convert-member-functions-to-static)

	[[nodiscard]] std::unordered_set<Cell> cellsToMove(Board const& board) const override;

	[[nodiscard]] std::unordered_set<Cell> cellsToAttack(const Board& board) const override;

	[[nodiscard]] int32_t getValue() const override {
		return 1;
	}
};
