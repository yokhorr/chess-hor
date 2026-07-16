#include "MinmaxedPlayer.hpp"

#include <ranges>

#include "board.hpp"
#include "outcome.hpp"
#include "piece.hpp"
#include "move.hpp"

constexpr int32_t INF = std::numeric_limits<int32_t>::max();

Move MinmaxedPlayer::makeMove(Board const& board) const {
	int32_t minMaxValue = INF;
	Move bestMove;

	for (const auto& move : board.getAllLegalMoves(getColor())) {
		auto boardCopy = Board(board.exportToFEN());
		boardCopy.makeMove(move);
		if (const int32_t maxEval = evaluatePosition(boardCopy, oppositeColor(getColor()), 1); maxEval < minMaxValue) {
			minMaxValue = maxEval;
			bestMove = move;
		}
	}

	return bestMove;
}

int32_t MinmaxedPlayer::countMaterialValue(Board const& board, Color const& color) {
	int32_t ans = 0;

	for (const auto& piece : board.getPieces(color)) {
		ans += piece->getValue();
	}

	return ans;
}

int32_t MinmaxedPlayer::countAttackValue(Board const& board, Color const& color) {
	int32_t ans = 0;

	auto pairs = board.allCellsToAttack(color);
	for (const auto& cells : pairs | std::views::values) {
		ans += cells.size();
	}

	return ans;
}

int32_t MinmaxedPlayer::heuristicValue(Board const& board, Color const& color) {
	if (board.getGameOutcome(color) == Outcome::mate) {
		return -INF;
	}

	if (board.getGameOutcome(color) == Outcome::stalemate) {
		if (countMaterialValue(board, color) > countMaterialValue(board, oppositeColor(color))) {
			return -INF;
		}
		return INF;
	}

	return 1000 * (countMaterialValue(board, color) - countMaterialValue(board, oppositeColor(color))); // +
		//(countAttackValue(board, color) - countAttackValue(board, oppositeColor(color)));
}

int32_t MinmaxedPlayer::evaluatePosition(const Board& board, const Color& color, const int32_t depth) {
	if (depth == maxDepth) {
		return heuristicValue(board, color);
	}

	int32_t minMaxValue = INF;

	for (const auto& move : board.getAllLegalMoves(color)) {
		auto boardCopy = Board(board.exportToFEN());
		boardCopy.makeMove(move);
		minMaxValue = std::min(minMaxValue, evaluatePosition(boardCopy, oppositeColor(color), depth + 1));
	}

	return -minMaxValue;
}
