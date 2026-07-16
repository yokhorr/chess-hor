#pragma once

#include <random>

#include "board.hpp"
#include "move.hpp"
#include "piece.hpp"
#include "player.hpp"

class BotPlayer final : public Player {
public:
	explicit BotPlayer(Color const color) : Player(color) {
		std::random_device rd;
		gen.seed(rd());
	}

	Move makeMove(Board const& board) const override {
		const auto okMoves = board.getAllLegalMoves(getColor());

		std::uniform_int_distribution<std::size_t> distrib(0, okMoves.size() - 1);

		return okMoves[distrib(gen)];
	}

private:
	mutable std::mt19937 gen{std::random_device{}()};
};
