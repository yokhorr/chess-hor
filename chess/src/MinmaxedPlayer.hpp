#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "board.hpp"
#include "player.hpp"
#include "move.hpp"

enum class Outcome;

class MinmaxedPlayer final : public Player {
public:
	explicit MinmaxedPlayer(const Color color) : Player(color) {}

	[[nodiscard]] Move makeMove(Board board) override;

	static int32_t countMaterialValue(Board const& board, Color const& color);

	static int32_t countMobilityValue(Board const& board, Color const& color);

	static int32_t heuristicValue(Board const& board, Color const& color);

	static int32_t evaluatePosition(Board board, const Color& color, int32_t depth, int32_t alpha, int32_t beta);

	inline static int32_t maxDepth = 4; // TODO: bigNumber which then get divided by numberOfMoves?
};
