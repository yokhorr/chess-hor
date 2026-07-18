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

	if (const auto currFen = board.exportToFEN(); !table_.contains(currFen)) {
		for (const auto& move : board.getAllLegalMoves(getColor())) {
			board.makeMove(move);
			table_[currFen].emplace_back(board.exportToFEN(), move);
			if (
				const int32_t maxEval = evaluatePosition(board, oppositeColor(getColor()), 1);
				maxEval < minMaxValue
			) {
				minMaxValue = maxEval;
				bestMove = move;
			}
			board.undoMove(move);
		}
	} else {
		for (const auto& [fen, move] : table_.at(currFen)) {
			if (
				const int32_t maxEval = evaluatePosition(Board(fen), oppositeColor(getColor()), 1);
				maxEval < minMaxValue
			) {
				minMaxValue = maxEval;
				bestMove = move;
			}
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

int32_t MinmaxedPlayer::countMobilityValue(Board const& board, Color const& color) {
	return static_cast<int32_t>(board.getAllLegalMoves(color).size());
}

int32_t MinmaxedPlayer::heuristicValue(Board const& board, Color const& color) {
	const auto fen = board.exportToFEN();
	if (positionValuesCache_.contains(fen)) {
		// std::cout << positionValuesCache_.size() << std::endl;
		return positionValuesCache_.at(fen);
	}

	const auto gameOutcome = board.getGameOutcome();

	if (gameOutcome == Outcome::mate) {
		positionValuesCache_[fen] = -WIN;
		return -WIN;
	}

	if (gameOutcome == Outcome::draw) {
		// NOTE: aggressive: >=, non-aggressive: >
		if (countMaterialValue(board, color) >= countMaterialValue(board, oppositeColor(color))) {
			positionValuesCache_[fen] = -WIN;
			return -WIN;
		}
		positionValuesCache_[fen] = WIN;
		return WIN;
	}

	const int32_t materialValue = 1000 *
		(countMaterialValue(board, color) - countMaterialValue(board, oppositeColor(color)));
	const int32_t mobilityValue = countMobilityValue(board, color) - countMobilityValue(board, oppositeColor(color));
	positionValuesCache_[fen] = materialValue + mobilityValue;
	return materialValue + mobilityValue;
}

int32_t MinmaxedPlayer::evaluatePosition(Board board, const Color& color, const int32_t depth) {
	if (depth == maxDepth || board.getGameOutcome() != Outcome::ongoing) {
		return heuristicValue(board, color);
	}

	int32_t minMaxValue = INF;

	if (const auto currFen = board.exportToFEN(); !table_.contains(currFen)) {
		for (const auto& move : board.getAllLegalMoves(color)) {
			board.makeMove(move);
			table_[currFen].emplace_back(board.exportToFEN(), move);
			if (
				const int32_t maxEval = evaluatePosition(board, oppositeColor(color), depth + 1);
				maxEval < minMaxValue
			) {
				minMaxValue = maxEval;
			}
			board.undoMove(move);
		}
	} else {
		for (const auto& fen : table_.at(currFen) | std::views::keys) {
			if (
				const int32_t maxEval = evaluatePosition(Board(fen), oppositeColor(color), depth + 1);
				maxEval < minMaxValue
			) {
				minMaxValue = maxEval;
			}
		}
	}

	return -minMaxValue;
}
