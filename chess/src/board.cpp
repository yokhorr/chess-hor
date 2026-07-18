#include <iostream>
#include <ranges>
#include <stdexcept>
#include <cassert>
#include <unordered_set>
#include <algorithm>

#include "board.hpp"
#include "cell.hpp"
#include "king.hpp"
#include "move.hpp"
#include "outcome.hpp"
#include "pawn.hpp"
#include "piece.hpp"
#include "pieceFactory.hpp"
#include "pieceType.hpp"
#include "rook.hpp"

bool Board::operator==(const Board& other) const {
	for (int32_t y = 0; y < BOARD_SIZE; ++y) {
		for (int32_t x = 0; x < BOARD_SIZE; ++x) {
			if (Cell cell(x, y); isFreeCell(cell)) {
				if (!other.isFreeCell(cell)) {
					return false;
				}
			} else {
				if (other.isFreeCell(cell)) {
					return false;
				}
				if (*getPiece(cell) != *other.getPiece(cell)) {
					return false;
				}
			}
		}
	}

	return castleRights == other.castleRights
		&& getEnPassantCell() == other.getEnPassantCell()
		&& getColorToMove() == other.getColorToMove();
		// NOTE: the following data is not passed through FEN, so we ignore it
		// && halfMoveClock_ == other.halfMoveClock_
		// && positionsCounter_ == other.positionsCounter_
		// && tripleRepeat_ == other.tripleRepeat_;

	// NOTE: data about the last move is not compared
}

Board::Board(Board const& other) : Board() {
	for (int32_t y = 0; y < BOARD_SIZE; ++y) {
		for (int32_t x = 0; x < BOARD_SIZE; ++x) {
			const Cell cell(x, y);
			if (!other.isFreeCell(cell)) {
				const auto piece = other.getPiece(cell);
				placePiece(createPiece(piece->getType(), piece->getColor()), cell);
			}
		}
	}

	enPassantCell_ = other.enPassantCell_;
	colorToMove_ = other.colorToMove_;
	considerColorToMove_ = other.considerColorToMove_;
	castleRights = other.castleRights;
	halfMoveClock_ = other.halfMoveClock_;
	positionsCounter_ = other.positionsCounter_;
	tripleRepeat_ = other.tripleRepeat_;

	// NOTE: data about the last move is not copied;
	// we don't plan to undoMove on just copied board.
}

Board& Board::operator=(Board const& other) {
	if (this != &other) {
		Board tmp(other);
		swap(tmp);
	}
	return *this;
}

void Board::swap(Board& other) noexcept {
	std::swap(enPassantCell_, other.enPassantCell_);
	std::swap(colorToMove_, other.colorToMove_);
	std::swap(considerColorToMove_, other.considerColorToMove_);
	std::swap(castleRights, other.castleRights);
	std::swap(board_, other.board_);
	std::swap(pieces_, other.pieces_);
	std::swap(kings_, other.kings_);

	// NOTE: data about the last move is not swapped;
	// we don't need it.
}

Board::Board(std::string const& str) : Board() {
	importFromFEN(str);
}

void Board::placePiece(Piece* piece, Cell const& cell) {
	assert(isValidCell(cell) && isFreeCell(cell));

	piece->setCell(cell);
	board_[cell.y][cell.x] = piece;
	pieces_[piece->getColor()].insert(piece);

	if (piece->getType() == PieceType::king) {
		kings_[piece->getColor()] = static_cast<King*>(piece); // NOLINT(*-pro-type-static-cast-downcast)
	}
}

void Board::placePiece(Piece* const piece) {
	placePiece(piece, piece->getCell());
}

void Board::removePieceImpl(Piece* piece, const Cell cell) {
	board_[cell.y][cell.x] = nullptr;
	pieces_[piece->getColor()].erase(piece);
}

// never remove king
// doesn't delete the piece
// can not actually define one from the other
void Board::removePiece(Piece* piece) {
	const auto cell = piece->getCell();

	removePieceImpl(piece, cell);
}

void Board::removePiece(Cell const& cell) {
	const auto piece = getPiece(cell);

	removePieceImpl(piece, cell);
}

void Board::revokeKingsideCastleRight(Color const& color) {
	castleRights[color].kingside = false;
}

void Board::revokeQueensideCastleRight(Color const& color) {
	castleRights[color].queenside = false;
}

void Board::triggerParamsUpdate(Move const& move, const moveInvolvedPieces& p) {
	if (move.from == kingCell(move.color)) {
		revokeKingsideCastleRight(move.color);
		revokeQueensideCastleRight(move.color);
	} else if (move.from == kingsideRookCell(move.color)) {
		revokeKingsideCastleRight(move.color);
	} else if (move.from == queensideRookCell(move.color)) {
		revokeQueensideCastleRight(move.color);
	} else if (move.to == kingsideRookCell(oppositeColor(move.color))) {
		revokeKingsideCastleRight(oppositeColor(move.color));
	} else if (move.to == queensideRookCell(oppositeColor(move.color))) {
		revokeQueensideCastleRight(oppositeColor(move.color));
	}

	if
	(
		p.movedPiece->getType() == PieceType::pawn
		&& move.from.y == pawnRank(move.color)
		&& abs(move.from.y - move.to.y) == 2
	) {
		enPassantCell_ = move.from.forward(move.color);
	} else {
		enPassantCell_ = NCELL;
	}

	if (p.movedPiece->getType() != PieceType::pawn && p.capturedPiece == nullptr) {
		++halfMoveClock_;
	} else {
		halfMoveClock_ = 0;
	}
}

void Board::doRookCastleMove(Move const& move) {
	// maybe one day I'll rewrite it so that
	// we don't create a new piece and delete the old one all the time,
	// and just move the original one

	Cell oldRookCell;
	Cell newRookCell;
	const auto newRook = new Rook(move.color);

	if (move.to == kingsideKnightCell(move.color)) {
		// kingside
		oldRookCell = kingsideRookCell(move.color);
		newRookCell = kingsideBishopCell(move.color);
	} else {
		// queenside
		oldRookCell = queensideRookCell(move.color);
		newRookCell = queenCell(move.color);
	}

	placePiece(newRook, newRookCell);
	const auto oldRook = getPiece(oldRookCell);
	removePiece(oldRookCell);
	delete oldRook;
}

void Board::undoRookCastleMove(Move const& move) {
	Cell oldRookCell;
	Cell newRookCell;
	const auto newRook = new Rook(move.color);

	if (move.to == kingsideKnightCell(move.color)) {
		// kingside
		oldRookCell = kingsideBishopCell(move.color);
		newRookCell = kingsideRookCell(move.color);
	} else {
		// queenside
		oldRookCell = queenCell(move.color);
		newRookCell = queensideRookCell(move.color);
	}

	placePiece(newRook, newRookCell);
	const auto oldRook = getPiece(oldRookCell);
	removePiece(oldRookCell);
	delete oldRook;
}

// assuming the move is legal, or we know what we do
void Board::makeMove(Move const& move) {
	if (!lastMoveIsCastle_) {
		delete moveInvolvedPiecesBackup_.movedPiece;
		delete moveInvolvedPiecesBackup_.capturedPiece;
		moveInvolvedPiecesBackup_.movedPiece = nullptr;
		moveInvolvedPiecesBackup_.capturedPiece = nullptr;
	}
	auto p = getMoveInvolvedPieces(move);
	moveInvolvedPiecesBackup_ = p;
	enPassantCellBackup_ = enPassantCell_;
	castleRightsBackup_ = castleRights;
	halfMoveClockBackup_ = halfMoveClock_;

	simulateMove(p);
	triggerParamsUpdate(move, p);

	if (isCastleMove(move, p.movedPiece)) {
		lastMoveIsCastle_ = true;
		doRookCastleMove(move);
	} else {
		lastMoveIsCastle_ = false;
	}

	colorToMove_ = oppositeColor(colorToMove_);
	if (++positionsCounter_[exportToFEN()] >= 3) {
		tripleRepeat_ = true;
	}
}

// Can only undo the very last move, otherwise UB
void Board::undoMove(Move const& move) {
	if (positionsCounter_[exportToFEN()]-- == 3) {
		tripleRepeat_ = false;
	}

	auto p = moveInvolvedPiecesBackup_;
	moveInvolvedPiecesBackup_ = {nullptr, nullptr, nullptr};

	simulateMove(p, true);
	enPassantCell_ = enPassantCellBackup_;
	castleRights = castleRightsBackup_;
	halfMoveClock_ = halfMoveClockBackup_;

	if (isCastleMove(move, p.movedPiece)) {
		undoRookCastleMove(move);
	}

	// nothing to delete, backward simulateMoves does the work

	colorToMove_ = oppositeColor(colorToMove_);
}

// assuming the move is legal, or we know what we do
void Board::makeMove(Cell const& from, Cell const& to) {
	makeMove(Move(from, to, PieceType::none, getPiece(from)->getColor()));
}

// assuming the move is legal enough
void Board::simulateMove(moveInvolvedPieces& p, const bool backward) {
	// duplicate code?
	if (!backward) {
		removePiece(p.movedPiece);
		// ReSharper disable once CppDFAConstantConditions // absolutely not correct
		if (p.capturedPiece) {
			removePiece(p.capturedPiece);
		}
		placePiece(p.arrivedPiece);
	} else {
		removePiece(p.arrivedPiece);
		placePiece(p.movedPiece);
		if (p.capturedPiece) {
			placePiece(p.capturedPiece);
		}

		delete p.arrivedPiece;
		p.arrivedPiece = nullptr;
	}
}

bool Board::isCastleMove(Move const& move, const Piece* movedPiece) {
	return movedPiece->getType() == PieceType::king && abs(move.from.x - move.to.x) == 2;
}

void Board::setCastleRights(const Color& color) {
	castleRights[color] = CastleRight(true, true);
}

void Board::setCastleRights() {
	setCastleRights(Color::white);
	setCastleRights(Color::black);
}

void Board::setKing(const Color& color) {
	placePiece(new King(color), kingCell(color));
}

void Board::setKingsideRook(const Color& color) {
	placePiece(new Rook(color), kingsideRookCell(color));
}

void Board::setQueensideRook(const Color& color) {
	placePiece(new Rook(color), queensideRookCell(color));
}

// assuming board is empty
void Board::setStart() {
	importFromFEN(START_FEN);
}

void Board::print() const {
	for (int32_t y = BOARD_SIZE - 1; y >= 0; --y) {
		for (int32_t x = 0; x < BOARD_SIZE; ++x) {
			if (isFreeCell(Cell(x, y))) {
				std::cout << ((x + y) % 2 == 0 ? "." : ".");
			} else {
				std::cout << getPiece(Cell(x, y))->getSign();
			}
			std::cout << ' ';
		}
		std::cout << "  " << y + 1 << std::endl;
	}
	std::cout << std::endl;
	for (char c = 'a'; c <= 'h'; ++c) {
		std::cout << c << ' ';
	}
	std::cout << std::endl;
}

Cell nextCellFEN(Cell const& cell, const int32_t delta) {
	return {cell.x + delta, cell.y};
}

Cell nextCellFEN(Cell const& cell) {
	return nextCellFEN(cell, 1);
}

std::string Board::exportToFEN() const {
	std::string ans;
	int32_t emptyRow = 0;
	for (int32_t y = BOARD_SIZE - 1; y >= 0; --y) {
		for (int32_t x = 0; x < BOARD_SIZE; ++x) {
			if (isFreeCell(Cell(x, y))) {
				++emptyRow;
				continue;
			}

			if (emptyRow > 0) {
				ans.push_back(static_cast<char>('0' + emptyRow));
				emptyRow = 0;
			}

			const auto piece = getPiece(Cell(x, y));
			const char tmpCh = static_cast<char>(piece->getType());
			const char ch = piece->getColor() == Color::white ? static_cast<char>(std::toupper(tmpCh)) : tmpCh;
			ans.push_back(ch);
		}

		// duplicate...
		if (emptyRow > 0) {
			ans.push_back(static_cast<char>('0' + emptyRow));
			emptyRow = 0;
		}

		ans.push_back('/');
	}

	ans.pop_back(); // last `/`

	ans.push_back(' ');
	ans.push_back(colorToMove_ == Color::white ? 'w' : 'b');
	ans.push_back(' ');

	std::string castlingString;
	if (castleRights.at(Color::white).kingside) {
		castlingString.push_back('K');
	}
	if (castleRights.at(Color::white).queenside) {
		castlingString.push_back('Q');
	}
	if (castleRights.at(Color::black).kingside) {
		castlingString.push_back('k');
	}
	if (castleRights.at(Color::black).queenside) {
		castlingString.push_back('q');
	}

	if (!castlingString.empty()) {
		ans += castlingString;
	} else {
		ans.push_back('-');
	}

	ans.push_back(' ');

	if (enPassantCell_ != NCELL) {
		ans += enPassantCell_.toString();
	} else {
		ans.push_back('-');
	}

	ans += " 0 1"; // we don't care now

	return ans;
}

std::size_t Board::parseMaterialFEN(std::string const& str, std::size_t i) {
	Cell currCell(0, 7);

	for (i = 0; str[i] != ' '; ++i) {
		if (isdigit(str[i])) {
			currCell = nextCellFEN(currCell, str[i] - '0');
		} else if (str[i] == '/') {
			currCell = {0, currCell.y - 1};
		} else {
			Color color = std::isupper(str[i]) ? Color::white : Color::black;
			Piece* piece = createPiece(static_cast<PieceType>(std::tolower(str[i])), color);
			placePiece(piece, currCell);
			currCell = nextCellFEN(currCell);
		}
	}

	return i;
}

std::size_t Board::parseCastlingRightsFEN(std::string const& str, std::size_t i) {
	for (; str[i] != ' '; ++i) {
		if (str[i] == '-') return i + 1;

		const Color color = std::isupper(str[i]) ? Color::white : Color::black;
		switch (std::tolower(str[i])) {
		case 'k':
			castleRights.at(color).kingside = true;
			break;
		case 'q':
			castleRights.at(color).queenside = true;
			break;
		default:
			throw std::invalid_argument("Invalid FEN");
		}
	}

	return i;
}

void Board::importFromFEN(std::string const& str) {
	std::size_t i = 0;
	i = parseMaterialFEN(str, i) + 1;
	colorToMove_ = str[i] == 'w' ? Color::white : Color::black;
	// ReSharper disable once CppDFAUnusedValue
	i = parseCastlingRightsFEN(str, i + 2);

	if (str[i + 1] != '-') {
		std::string s;
		s.push_back(str[i + 1]);
		s.push_back(str[i + 2]);
		enPassantCell_ = Cell(s);
	}

	// well, now we should parse the last two numbers, but I don't care
}

std::unordered_set<Cell> Board::pieceCellsToMove(Cell const& from) const {
	if (isFreeCell(from)) {
		throw std::invalid_argument("No piece at the given cell");
	}

	return pieceCellsToMove(getPiece(from));
}

std::unordered_set<Cell> Board::pieceCellsToMove(const Piece* piece) const {
	return piece->cellsToMove(*this);
}

std::vector<std::pair<Piece*, std::unordered_set<Cell>>> Board::allCellsToMove(Color const& color) const {
	return collectCells(color, [](const Piece* p, const Board& b) {
		return p->cellsToMove(b);
	});
}

std::vector<std::pair<Piece*, std::unordered_set<Cell>>> Board::allCellsToAttack(Color const& color) const {
	return collectCells(color, [](const Piece* p, const Board& b) {
		return p->cellsToAttack(b);
	});
}

std::vector<Move> Board::getAllLegalMoves() const {
	return getAllLegalMoves(getColorToMove());
}

std::vector<Move> Board::getAllLegalMoves(Color const& color) const {
	return getAllLegalActions(color, [this] (Color const& _color) {
		return allCellsToMove(_color);
	}, false);
}

std::vector<Move> Board::getAllLegalAttacks(Color const& color) const {
	return getAllLegalActions(color, [this] (Color const& _color) {
		return allCellsToAttack(_color);
	}, true);
}

bool Board::isCellAttacked
(
	Cell const& targetCell, std::vector<std::pair<Piece*, std::unordered_set<Cell>>> const& attackedCells
) {
	return std::ranges::any_of(attackedCells | std::views::values,
	                           [&](const auto& cells) { return cells.contains(targetCell); });
}

bool Board::isCellAttacked(Color const& color, Cell const& targetCell) const {
	return isCellAttacked(targetCell, allCellsToAttack(oppositeColor(color)));
}

bool Board::isKingChecked(Color const& color) const {
	if (!kings_.contains(color)) {
		throw std::invalid_argument("The king is not on the board");
	}

	return isCellAttacked(color, kings_.at(color)->getCell());
}

bool Board::isMoveLegal(Move const& move) const {
	if
	(
		isOkColor(move.color) && getPiece(move.from)->getColor() == move.color && !isFreeCell(move.from)
		&& pieceCellsToMove(move.from).contains(move.to)
	) {
		auto p = getMoveInvolvedPieces(move);

		if (isCastleMove(move, p.movedPiece)) {
			return true; // castling legality is already properly checked
		}

		auto& mutableThis = const_cast<Board&>(*this); // we roll back the state after check

		mutableThis.simulateMove(p);
		const auto kingIsChecked = isKingChecked(move.color);
		mutableThis.simulateMove(p, true); // roll back

		return !kingIsChecked;
	}

	return false;
}

Outcome Board::getGameOutcome() const {
	if (tripleRepeat_) {
		return Outcome::draw;
	}

	if (halfMoveClock_ >= 50) {
		// the fifty-move rule
		return Outcome::draw;
	}

	if (pieces_.at(Color::white).size() == 1 && pieces_.at(Color::black).size() == 1) {
		// just two kings left
		return Outcome::draw;
	}

	const bool hasMoves = !getAllLegalMoves(getColorToMove()).empty();
	const bool inCheck = isKingChecked(getColorToMove());

	if (hasMoves) {
		return Outcome::ongoing;
	}

	// no moves
	if (inCheck) {
		return Outcome::mate;
	}

	// no moves, no check
	return Outcome::draw; // stalemate
}

moveInvolvedPieces Board::getMoveInvolvedPieces(Move const& move) const {
	moveInvolvedPieces ans;

	ans.movedPiece = getPiece(move.from);

	if (move.promotion == PieceType::none) {
		ans.arrivedPiece = createPiece(ans.movedPiece->getType(), move.color);
	} else {
		ans.arrivedPiece = createPiece(move.promotion, move.color);
	}
	ans.arrivedPiece->setCell(move.to);

	ans.capturedPiece = nullptr;
	// if a pawn goes to an en passant field, we sure it does an en passant move
	if (move.to == enPassantCell_) {
		ans.capturedPiece = getPiece(enPassantCell_.forward(oppositeColor(move.color)));
	} else if (!isFreeCell(move.to)) {
		ans.capturedPiece = getPiece(move.to);
	}

	return ans;
}

CastleRight Board::checkCastleRight(Color const& color) const {
	auto ans = CastleRight(false, false);

	if (!castleRights.at(color).kingside && !castleRights.at(color).queenside) {
		return ans;
	}

	const auto attackedCells = allCellsToAttack(oppositeColor(color));

	// duplicate code?
	if (
		castleRights.at(color).kingside
		&& isFreeCell(kingsideBishopCell(color))
		&& isFreeCell(kingsideKnightCell(color))
		&& !isCellAttacked(kingCell(color), attackedCells)
		&& !isCellAttacked(kingsideBishopCell(color), attackedCells)
		&& !isCellAttacked(kingsideKnightCell(color), attackedCells)
	) {
		ans.kingside = true;
	}

	if (
		castleRights.at(color).queenside
		&& isFreeCell(queensideBishopCell(color))
		&& isFreeCell(queensideKnightCell(color))
		&& isFreeCell(queenCell(color))
		&& !isCellAttacked(kingCell(color), attackedCells)
		&& !isCellAttacked(queensideBishopCell(color), attackedCells)
		&& !isCellAttacked(queensideKnightCell(color), attackedCells)
		&& !isCellAttacked(queenCell(color), attackedCells)
	) {
		ans.queenside = true;
	}

	return ans;
}

Board::~Board() {
	for (const auto& color : {Color::white, Color::black}) {
		for (const auto& piece : pieces_[color]) {
			delete piece;
		}
	}

	// ReSharper disable once CppDFAConstantConditions // not actually always false
	if (!lastMoveIsCastle_) {
		// ReSharper disable once CppDFAUnreachableCode
		delete moveInvolvedPiecesBackup_.movedPiece;
		delete moveInvolvedPiecesBackup_.capturedPiece;
	}
}
