#pragma once
#include <unordered_set>

#include "cell.hpp"
#include "config.hpp"
#include "castleRight.hpp"
#include "move.hpp"
#include "piece.hpp"
#include "pieceType.hpp"

enum class Outcome;
class King;
class Piece;

struct moveInvolvedPieces {
	Piece* movedPiece = nullptr;
	Piece* capturedPiece = nullptr;
	Piece* arrivedPiece = nullptr;
};

class Board {
public:
	[[nodiscard]] bool operator==(const Board&) const;

	Board() = default;

	explicit Board(std::string const& str);

	bool isFreeCell(Cell const& cell) const {
		return board_[cell.y][cell.x] == nullptr;
	}

	void placePiece(Piece* piece, Cell const& cell);

	void placePiece(Piece* piece);

	[[nodiscard]] Piece* getPiece(Cell const& cell) const {
		return board_[cell.y][cell.x];
	}

	void makeMove(Move const& move);
	void makeMove(Cell const& from, Cell const& to);

	void setStart();

	void print() const;

	std::string exportToFEN() const;

	std::size_t parseMaterialFEN(std::string const& str, std::size_t i);
	std::size_t parseCastlingRightsFEN(std::string const& str, std::size_t i);
	void importFromFEN(std::string const& str);

	[[nodiscard]] std::unordered_set<Cell> pieceCellsToMove(Cell const& from) const;

	[[nodiscard]] std::unordered_set<Cell> pieceCellsToMove(const Piece* piece) const;

	[[nodiscard]] std::vector<std::pair<Piece*, std::unordered_set<Cell>>> allCellsToMove(Color const& color) const;

	[[nodiscard]] std::vector<std::pair<Piece*, std::unordered_set<Cell>>> allCellsToAttack(Color const& color) const;

	[[nodiscard]] static bool isCellAttacked(
		Cell const& targetCell, std::vector<std::pair<Piece*, std::unordered_set<Cell>>> const& attackedCells
	);

	[[nodiscard]] bool isCellAttacked(Color const& color, Cell const& targetCell) const;

	[[nodiscard]] bool isKingChecked(Color const& color) const;

	[[nodiscard]] bool isMoveLegal(Move const& move) const;

	[[nodiscard]] std::vector<Move> getAllLegalMoves(Color const& color) const;
	std::vector<Move> getAllLegalAttacks(Color const& color) const;

	[[nodiscard]] Outcome getGameOutcome(Color const& currentTurn) const;

	[[nodiscard]] Cell getEnPassantCell() const { return enPassantCell_; }

	[[nodiscard]] moveInvolvedPieces getMoveInvolvedPieces(Move const& move) const;

	[[nodiscard]] CastleRight checkCastleRight(Color const& color) const;

	[[nodiscard]] static bool isCastleMove(Move const& move, const Piece* movedPiece);

	[[nodiscard]] Color getColorToMove() const {
		return colorToMove_;
	}

	void setConsiderColorToMove(const bool consider) {
		considerColorToMove_ = consider;
	}

	std::vector<Piece*> getPieces(Color const& color) const {
		std::vector<Piece*> ans;

		for (const auto& piece : pieces_.at(color)) {
			ans.push_back(piece);
		}

		return ans;
	}

	void setCastleRights(const Color& color);
	void setCastleRights();
	void setKing(const Color& color);
	void setKingsideRook(const Color& color);
	void setQueensideRook(const Color& color);

	~Board();

private:
	void removePieceImpl(Piece* piece, Cell cell);
	void removePiece(Cell const& cell);
	void revokeKingsideCastleRight(Color const& color);
	void revokeQueensideCastleRight(Color const& color);
	void removePiece(Piece* piece);
	void triggerParamsUpdate(Move const& move, const Piece*);
	void doRookCastleMove(Move const& move);
	void simulateMove(moveInvolvedPieces const& p, bool backward = false);

	[[nodiscard]] bool isOkColor(Color const& color) const {
		return considerColorToMove_ ? color == getColorToMove() : true;
	}

	template<typename Func>
	std::vector<std::pair<Piece*, std::unordered_set<Cell>>> collectCells(Color const& color, Func&& func) const;
	template <class Func>
	std::vector<Move> getAllLegalActions(Color const& color, Func&& func, bool crutchAttack) const;

	std::vector<std::vector<Piece*>> board_ = std::vector(BOARD_SIZE, std::vector<Piece*>(BOARD_SIZE));
	std::unordered_map<Color, std::unordered_set<Piece*>> pieces_;
	std::unordered_map<Color, King*> kings_;
	Cell enPassantCell_ = NCELL;
	Color colorToMove_ = Color::white;
	bool considerColorToMove_ = true;

public:
	std::unordered_map<Color, CastleRight> castleRights = {
		{Color::white, CastleRight()},
		{Color::black, CastleRight()}
	};
};

template <typename Func>
std::vector<std::pair<Piece*, std::unordered_set<Cell>>> Board::collectCells(Color const& color, Func&& func) const {
	std::vector<std::pair<Piece*, std::unordered_set<Cell>>> ans;

	for (const auto& piece : pieces_.at(color)) {
		if (auto moves = func(piece, *this); !moves.empty()) {
			ans.emplace_back(piece, std::move(moves));
		}
	}

	return ans;
}

// TODO: kill yourself

template <typename Func>
std::vector<Move> Board::getAllLegalActions(Color const& color, Func&& func, bool crutchAttack) const {
	std::vector<Move> ans;

	for (const auto& [piece, tos] : func(color)) {
		for (const auto& to : tos) {
			if (Move move = {piece->getCell(), to, PieceType::none, color}; (getPiece(move.from)->getType() == PieceType::pawn && crutchAttack && isValidCell(move.to)) || isMoveLegal(move)) {
				if (!crutchAttack && piece->getType() == PieceType::pawn && to.y == lastRank(color)) {
					for (
						const auto& pieceType :
						{PieceType::knight, PieceType::bishop, PieceType::rook, PieceType::queen}
					) {
						ans.emplace_back(move.from, move.to, pieceType, color);
					}
				} else {
					ans.emplace_back(move.from, move.to, PieceType::none, color);
				}
			}
		}
	}

	return ans;
}