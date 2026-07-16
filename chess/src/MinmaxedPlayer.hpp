#pragma once
#include <cstdint>

#include "player.hpp"

class MinmaxedPlayer final : public Player {
public:
	explicit MinmaxedPlayer(const Color color) : Player(color) {}

	[[nodiscard]] Move makeMove(const Board& board) const override;

	static int32_t countMaterialValue(Board const& board, Color const& color);

	static int32_t countAttackValue(Board const& board, Color const& color);

	static int32_t heuristicValue(Board const& board, Color const& color);

	static int32_t evaluatePosition(const Board& board, const Color& color, int32_t depth);

	inline static int32_t maxDepth = 3;
};
