#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "player.hpp"

enum class Outcome;

class MinmaxedPlayer final : public Player {
public:
	explicit MinmaxedPlayer(const Color color) : Player(color) {
		table_.reserve(100000);
		positionValuesCache_.reserve(100000);
	}

	[[nodiscard]] Move makeMove(Board board) override;

	static int32_t countMaterialValue(Board const& board, Color const& color);

	static int32_t countMobilityValue(Board const& board, Color const& color);

	int32_t heuristicValue(Board const& board, Color const& color);

	int32_t evaluatePosition(Board board, const Color& color, int32_t depth);

	inline static int32_t maxDepth = 3;

	size_t getCacheSize() const {
		return positionValuesCache_.size();
	}

private:
	std::unordered_map<std::string, std::vector<std::pair<std::string, Move>>> table_; // TODO: save board?
	std::unordered_map<std::string, int32_t> positionValuesCache_;
};
