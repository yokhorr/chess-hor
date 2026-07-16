#pragma once

#include <stdexcept>

#include "bishop.hpp"
#include "king.hpp"
#include "knight.hpp"
#include "pawn.hpp"
#include "piece.hpp"
#include "queen.hpp"
#include "rook.hpp"

inline Piece* createPiece(PieceType const& type, Color const& color) {
	switch (type) {
	case PieceType::pawn:
		return new Pawn(color);
	case PieceType::king:
		return new King(color);
	case PieceType::knight:
		return new Knight(color);
	case PieceType::bishop:
		return new Bishop(color);
	case PieceType::rook:
		return new Rook(color);
	case PieceType::queen:
		return new Queen(color);
	case PieceType::none:
		throw std::invalid_argument("Can not create none piece");
	default:
		throw std::invalid_argument("Invalid PieceType received");
	}
}
