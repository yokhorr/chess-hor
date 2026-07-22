#include "MinmaxedPlayer.hpp"

#include <cassert>
#include <iostream>
#include <ranges>

#include "board.hpp"
#include "outcome.hpp"
#include "piece.hpp"
#include "move.hpp"

constexpr int32_t INF = std::numeric_limits<int32_t>::max();
constexpr int32_t WIN = INF - 1;

Move MinmaxedPlayer::makeMove(Board board) {
	int32_t minMaxValue = INF;
	Move bestMove;

	for (const auto& move : board.getAllLegalMoves(getColor())) {
		board.makeMove(move);
		if (
			const int32_t maxEval = evaluatePosition(board, oppositeColor(getColor()), 1, -INF, +INF);
			maxEval < minMaxValue
		) {
			minMaxValue = maxEval;
			bestMove = move;
		}
		board.undoMove(move);
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

int32_t MinmaxedPlayer::countMobilityValue(Board const& board, Color const& color) {
	return static_cast<int32_t>(board.getAllLegalMoves(color).size());
}

int32_t MinmaxedPlayer::heuristicValue(Board const& board, Color const& color) {
	const auto gameOutcome = board.getGameOutcome();

	if (gameOutcome == Outcome::mate) {
		return -WIN;
	}

	if (gameOutcome == Outcome::draw) {
		// NOTE: aggressive: >=, non-aggressive: >
		if (countMaterialValue(board, color) > countMaterialValue(board, oppositeColor(color))) {
			return -WIN;
		}
		return WIN;
	}

	const int32_t materialValue = 1000 *
		(countMaterialValue(board, color) - countMaterialValue(board, oppositeColor(color)));
	const int32_t mobilityValue = countMobilityValue(board, color) - countMobilityValue(board, oppositeColor(color));
	return materialValue + mobilityValue;
}

int32_t MinmaxedPlayer::evaluatePosition(Board board, const Color& color, const int32_t depth, int32_t alpha, int32_t beta) {
	if (depth == maxDepth || board.getGameOutcome() != Outcome::ongoing) {
		return heuristicValue(board, color);
	}

	int32_t bestValue = -INF;

	for (const auto& move : board.getAllLegalMoves(color)) {
		board.makeMove(move);
		const int32_t currValue = -evaluatePosition(board, oppositeColor(color), depth + 1, -beta, -alpha);
		board.undoMove(move);

		bestValue = std::max(bestValue, currValue);
		alpha = std::max(alpha, currValue);
		if (alpha >= beta) {
			break;
		}
	}

	return bestValue;
}
