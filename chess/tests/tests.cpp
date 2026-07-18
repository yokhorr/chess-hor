#define CATCH_CONFIG_MAIN
#include <iostream>
#include <catch2/catch_all.hpp>

#include "bishop.hpp"
#include "knight.hpp"
#include "move.hpp"
#include "outcome.hpp"
#include "queen.hpp"
#include "rook.hpp"
#include "../src/board.hpp"
#include "../src/pawn.hpp"
#include "../src/king.hpp"
#include "../src/config.hpp"

// TODO: неплохо бы бенчмарки написать

constexpr auto WHITE = Color::white;
constexpr auto BLACK = Color::black;


TEST_CASE("All correct cells") {
	for (int32_t x = 0; x < 8; ++x) {
		for (int32_t y = 0; y < 8; ++y) {
			REQUIRE(isValidCell(Cell(x, y)) == true);
		}
	}
}

TEST_CASE("Some incorrect cells") {
	REQUIRE_FALSE(isValidCell(NCELL));
	REQUIRE_FALSE(isValidCell(Cell(-1, 0)));
	REQUIRE_FALSE(isValidCell(Cell(0, 8)));
	REQUIRE_FALSE(isValidCell(Cell(8, 0)));
}

TEST_CASE("Cell::forward moves correctly for both colors") {
	const Cell start("e5");

	REQUIRE(start.forward(WHITE) == Cell("e6"));
	REQUIRE(start.forward(BLACK) == Cell("e4"));
}

TEST_CASE("Pawn has 4 moves") {
	Board board;
	const auto whitePawn = new Pawn(WHITE);
	board.placePiece(whitePawn, Cell("e2"));
	board.placePiece(new Pawn(BLACK), Cell("d3"));
	board.placePiece(new Pawn(BLACK), Cell("f3"));

	REQUIRE(board.pieceCellsToMove(whitePawn).size() == 4);
}

TEST_CASE("King has 8 moves") {
	Board board;
	const auto king = new King(WHITE);
	board.placePiece(king, Cell("d4"));

	REQUIRE(board.pieceCellsToMove(king).size() == 8);
}

TEST_CASE("King has 2 moves") {
	Board board;
	const auto king = new King(WHITE);
	board.placePiece(king, Cell("a1"));
	board.placePiece(new Pawn(WHITE), Cell("a2"));
	board.placePiece(new Pawn(BLACK), Cell("b2"));

	REQUIRE(board.pieceCellsToMove(king).size() == 2);
}

TEST_CASE("White have 8 moves with 3 pieces") {
	Board board;
	const auto whitePawn = new Pawn(WHITE);
	board.placePiece(whitePawn, Cell("e2"));
	board.placePiece(new Pawn(BLACK), Cell("d3"));
	board.placePiece(new Pawn(BLACK), Cell("f3"));

	const auto king = new King(WHITE);
	board.placePiece(king, Cell("a1"));
	board.placePiece(new Pawn(WHITE), Cell("a2"));

	const auto moves = board.allCellsToMove(WHITE);
	REQUIRE(moves.size() == 3);

	std::size_t movesCnt = 0;
	for (const auto& val : moves | std::views::values) {
		movesCnt += val.size();
	}
	REQUIRE(movesCnt == 8);
}

TEST_CASE("King is checked by pawn") {
	Board board;
	board.placePiece(new King(WHITE), Cell("a1"));
	board.placePiece(new Pawn(BLACK), Cell("b2"));

	REQUIRE(board.isKingChecked(WHITE));
}

TEST_CASE("King is 'checked' by other king") {
	Board board;
	board.placePiece(new King(WHITE), Cell("a1"));
	board.placePiece(new King(BLACK), Cell("b2"));

	REQUIRE(board.isKingChecked(WHITE));
}

TEST_CASE("King makes illegal move to pawn") {
	Board board;
	board.placePiece(new King(WHITE), Cell("a1"));
	board.placePiece(new Pawn(BLACK), Cell("b3"));

	const Move move = {Cell("a1"), Cell("a2"), PieceType::none, WHITE};

	REQUIRE_FALSE(board.isMoveLegal(move));
}

TEST_CASE("King makes legal move to pawn") {
	Board board;
	board.placePiece(new King(WHITE), Cell("a1"));
	board.placePiece(new Pawn(BLACK), Cell("b3"));

	const Move move = {Cell("a1"), Cell("b2"), PieceType::none, WHITE};

	REQUIRE(board.isMoveLegal(move));
}

TEST_CASE("King makes illegal move to other king") {
	Board board;
	board.placePiece(new King(WHITE), Cell("a1"));
	board.placePiece(new King(BLACK), Cell("b3"));

	const Move move = {Cell("a1"), Cell("a2"), PieceType::none, WHITE};

	REQUIRE_FALSE(board.isMoveLegal(move));
}

Move kingsideCastle(Color const& color) {
	return Move(kingCell(color), kingsideKnightCell(color), PieceType::none, color);
}

Move queensideCastle(Color const& color) {
	return Move(kingCell(color), queensideBishopCell(color), PieceType::none, color);
}

Board minimalCastleSetup() {
	Board board;

	board.setKing(BLACK);
	board.setKing(WHITE);
	board.setKingsideRook(BLACK);
	board.setQueensideRook(BLACK);
	board.setKingsideRook(WHITE);
	board.setQueensideRook(WHITE);
	board.setCastleRights();

	return board;
}

TEST_CASE("Kings has rights to castle") {
	Board board = minimalCastleSetup();
	board.setConsiderColorToMove(false);

	for (const auto& color : {WHITE, BLACK}) {
		REQUIRE(board.isMoveLegal(kingsideCastle(color)));
		REQUIRE(board.isMoveLegal(queensideCastle(color)));
	}
}

TEST_CASE("Can not castle in check") {
	Board board = minimalCastleSetup();

	board.placePiece(new Pawn(WHITE), Cell("d7"));

	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("d7"), Cell("f7"));

	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));
}

TEST_CASE("Can not castle through attacked cells") {
	Board board = minimalCastleSetup();
	board.setConsiderColorToMove(false);

	board.placePiece(new Pawn(WHITE), Cell("e7"));

	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("e7"), Cell("c7"));

	REQUIRE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("c7"), Cell("g7"));

	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE(board.isMoveLegal(queensideCastle(BLACK)));
}

TEST_CASE("Can not castle to check") {
	Board board = minimalCastleSetup();
	board.setConsiderColorToMove(false);

	board.placePiece(new Pawn(WHITE), Cell("b7"));

	REQUIRE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("b7"), Cell("g7"));

	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE(board.isMoveLegal(queensideCastle(BLACK)));
}

TEST_CASE("Can not castle if king walked") {
	Board board = minimalCastleSetup();

	board.makeMove(kingCell(BLACK), kingCell(BLACK).forward(BLACK));
	board.makeMove(kingCell(BLACK).forward(BLACK), kingCell(BLACK));

	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));
}

TEST_CASE("Can not castle if rook walked") {
	Board board = minimalCastleSetup();
	board.setConsiderColorToMove(false);

	// back and forth
	board.makeMove(kingsideRookCell(BLACK), kingsideRookCell(BLACK).forward(BLACK));
	board.makeMove(kingsideRookCell(BLACK).forward(BLACK), kingsideRookCell(BLACK));

	// queenside castle is yet legal
	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE(board.isMoveLegal(queensideCastle(BLACK)));

	// just one move
	board.makeMove(queensideRookCell(BLACK), queensideRookCell(BLACK).forward(BLACK));

	// both illegal
	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));
}

TEST_CASE("Can not castle if rook not present") {
	Board board = minimalCastleSetup();

	board.placePiece(new Pawn(WHITE), Cell("b7"));
	board.placePiece(new Pawn(WHITE), Cell("g7"));

	board.makeMove(Move(Cell("b7"), Cell("a8"), PieceType::knight, WHITE));
	board.makeMove(Move(Cell("g7"), Cell("h8"), PieceType::knight, WHITE));

	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));
}

TEST_CASE("Can not castle when pieces are on the way") {
	Board board = minimalCastleSetup();
	board.setConsiderColorToMove(false);

	board.placePiece(new Queen(BLACK), Cell("b8"));
	REQUIRE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("b8"), Cell("c8"));
	REQUIRE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("c8"), Cell("d8"));
	REQUIRE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE_FALSE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("d8"), Cell("f8"));
	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("f8"), Cell("g8"));
	REQUIRE_FALSE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE(board.isMoveLegal(queensideCastle(BLACK)));

	board.makeMove(Cell("g8"), Cell("b7"));
	REQUIRE(board.isMoveLegal(kingsideCastle(BLACK)));
	REQUIRE(board.isMoveLegal(queensideCastle(BLACK)));
}

TEST_CASE("Legal and illegal en passant moves for white") {
	Board board;
	board.setConsiderColorToMove(false);
	board.setStart();

	board.placePiece(new Pawn(WHITE), Cell("e5"));

	// pawns go for two cells
	const auto dxd5 = Move(Cell("d7"), Cell("d5"), PieceType::none, BLACK);
	const auto fxf5 = Move(Cell("f7"), Cell("f5"), PieceType::none, BLACK);

	REQUIRE(board.isMoveLegal(dxd5));
	REQUIRE(board.isMoveLegal(fxf5));

	board.makeMove(dxd5); // first
	board.makeMove(fxf5); // second

	// cells to attack
	const auto exd5 = Move(Cell("e5"), Cell("d6"), PieceType::none, WHITE);
	const auto exf5 = Move(Cell("e5"), Cell("f6"), PieceType::none, WHITE);

	REQUIRE_FALSE(board.isMoveLegal(exd5)); // not last
	REQUIRE(board.isMoveLegal(exf5)); // is last
}

TEST_CASE("Legal and illegal en passant moves for black") {
	Board board;
	board.setStart();
	board.setConsiderColorToMove(false);

	board.placePiece(new Pawn(BLACK), Cell("e4"));

	// pawns go for two cells
	const auto dxd4 = Move(Cell("d2"), Cell("d4"), PieceType::none, WHITE);
	const auto fxf4 = Move(Cell("f2"), Cell("f4"), PieceType::none, WHITE);

	REQUIRE(board.isMoveLegal(dxd4));
	REQUIRE(board.isMoveLegal(fxf4));

	board.makeMove(dxd4); // first
	board.makeMove(fxf4); // second

	// cells to attack
	const auto exd3 = Move(Cell("e4"), Cell("d3"), PieceType::none, BLACK);
	const auto exf3 = Move(Cell("e4"), Cell("f3"), PieceType::none, BLACK);

	REQUIRE_FALSE(board.isMoveLegal(exd3)); // not last
	REQUIRE(board.isMoveLegal(exf3)); // is last
}

TEST_CASE("Stalemate and mate by pawns to king in the corner") {
	Board board;
	board.placePiece(new King(BLACK), Cell("a8"));
	board.placePiece(new Pawn(BLACK), Cell("a2"));
	board.placePiece(new Pawn(BLACK), Cell("a3"));
	board.placePiece(new Pawn(BLACK), Cell("b3"));
	board.placePiece(new King(WHITE), Cell("a1"));

	REQUIRE(board.getGameOutcome() == Outcome::draw);

	board.placePiece(new Pawn(BLACK), Cell("b2"));

	REQUIRE(board.getGameOutcome() == Outcome::mate);
}

TEST_CASE("Default boards are equal") {
	Board board1, board2;
	board1.setStart();
	board2.setStart();

	REQUIRE(board1 == board2);
}

TEST_CASE("Minimal castle boards are equal") {
	Board board1 = minimalCastleSetup();
	Board board2 = minimalCastleSetup();

	REQUIRE(board1 == board2);
}

TEST_CASE("Empty boards are equal") {
	Board board1, board2;

	REQUIRE(board1 == board2);
}

// damn, duplicate code
TEST_CASE("Boards with same en passant moves are equal") {
	Board board1, board2;

	for (auto* board : {&board1, &board2}) {
		board->setConsiderColorToMove(false);
		board->setStart();

		board->placePiece(new Pawn(WHITE), Cell("e5"));

		// pawns go for two cells
		const auto dxd5 = Move(Cell("d7"), Cell("d5"), PieceType::none, BLACK);
		const auto fxf5 = Move(Cell("f7"), Cell("f5"), PieceType::none, BLACK);

		board->makeMove(dxd5); // first
		board->makeMove(fxf5); // second
	}

	REQUIRE(board1 == board2);
}

TEST_CASE("Not equal boards") {
	Board board1, board2;

	board2.setStart();

	REQUIRE(board1 != board2);
}

TEST_CASE("Different castling rights in different boards") {
	Board board1, board2;

	board1.setStart();
	board2.setStart();

	board2.castleRights.at(WHITE) = CastleRight(); // default false

	REQUIRE(board1 != board2);
}

TEST_CASE("Basic Board-FEN-Board trips") {
	Board boardEmpty;
	Board boardMinCas = minimalCastleSetup();
	Board boardStart;
	boardStart.setStart();

	REQUIRE(Board(boardStart.exportToFEN()) == boardStart);
	REQUIRE(Board(boardMinCas.exportToFEN()) == boardMinCas);
	REQUIRE(Board(boardEmpty.exportToFEN()) == boardEmpty);
}

const std::string emptyFEN = "8/8/8/8/8/8/8/8 w - - 0 1";
const std::string minCasFEN = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
const std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

TEST_CASE("Basic boards FENs") {
	Board boardEmpty;
	Board boardMinCas = minimalCastleSetup();
	Board boardStart;
	boardStart.setStart();

	REQUIRE(boardEmpty.exportToFEN() == emptyFEN);
	REQUIRE(boardMinCas.exportToFEN() == minCasFEN);
	REQUIRE(boardStart.exportToFEN() == startFEN);
}

TEST_CASE("Basic FEN-Board-FEN trips") {
	REQUIRE(Board(emptyFEN).exportToFEN() == emptyFEN);
	REQUIRE(Board(minCasFEN).exportToFEN() == minCasFEN);
	REQUIRE(Board(startFEN).exportToFEN() == startFEN);
}

const std::vector<std::string> someFens = {
	"rnbqkb1r/ppp1pppp/3p4/3nP3/3P4/5N2/PPP2PPP/RNBQKB1R b KQkq - 0 1",
	"rnbqkb1r/pp2pppp/5n2/3p4/2PP4/2N5/PP3PPP/R1BQKBNR b KQkq - 0 1",
	"r1bqkbnr/pp1npppp/2p5/8/3PN3/8/PPP2PPP/R1BQKBNR w KQkq - 0 1",
	"rnbqkb1r/ppp2ppp/4pn2/3p4/3PP3/2N5/PPP2PPP/R1BQKBNR w KQkq - 0 1",
	"rnbqk1nr/pppp1ppp/8/2b1p3/4PP2/8/PPPP2PP/RNBQKBNR w KQkq - 0 1",
	"rnbqk2r/pppp1ppp/4pn2/8/1bPP4/2N5/PP2PPPP/R1BQKBNR w KQkq - 0 1",
	"rqqqkqqr/qqqqqqqq/qqqqqqqq/qqqqqqqq/QQQQQQQQ/QQQQQQQQ/QQQQQQQQ/RQQQKQQR w KQkq - 0 1",
	"r2q1rk1/pb2ppbp/1pn1n1p1/2p1P3/2P2P2/P1NBBN2/1P4PP/R2Q1RK1 w - - 0 1",
	"rnb1kbnr/ppp1pppp/4q3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1",
	"rnb1kbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1",
	"7k/1P6/8/8/8/8/8/7K w - - 0 1",
	"8/1P6/8/8/8/2k5/2p5/2K5 w - - 0 1",
	"8/q2P3K/8/7k/8/8/8/8 w - - 0 1",
	"r7/r2P3K/r7/7k/8/8/8/6r1 w - - 0 1",
	"6r1/5P2/8/8/8/8/r7/7K w - - 0 1",
	"rnb1k2r/pppp1ppp/4p1qn/2b4P/Q1P3P1/N4P2/PP1PP3/R1B1KBNR b KQkq - 0 1"
};

TEST_CASE("Extra FEN-Board-FEN trips") {
	for (auto const& fen : someFens) {
		REQUIRE(Board(fen).exportToFEN() == fen);
	}
}

// daaamn, so much duplicate code
TEST_CASE("FEN with en passant cell") {
	Board board;
	board.setConsiderColorToMove(false);
	board.setStart();

	board.placePiece(new Pawn(WHITE), Cell("e5"));

	// pawns go for two cells
	const auto dxd5 = Move(Cell("d7"), Cell("d5"), PieceType::none, BLACK);
	const auto fxf5 = Move(Cell("f7"), Cell("f5"), PieceType::none, BLACK);

	board.makeMove(dxd5); // first
	board.makeMove(fxf5); // second

	const std::string fen = "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPPPPP/RNBQKBNR w KQkq f6 0 1";

	REQUIRE(board.exportToFEN() == fen);
	REQUIRE(board == Board(fen));
}

TEST_CASE("FEN black to move") {
	Board board;
	board.setStart();
	board.makeMove(Cell("e2"), Cell("e4"));

	const std::string e4 = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";

	REQUIRE(board.exportToFEN() == e4);
	REQUIRE(board == Board(e4));
}

TEST_CASE("Rook in corner has 14 moves") {
	Board board;
	const auto rook = new Rook(WHITE);
	board.placePiece(rook, Cell("a1"));

	std::vector expectedVec = {
		Cell("a2"), Cell("a3"), Cell("a4"), Cell("a5"), Cell("a6"), Cell("a7"), Cell("a8"),
		Cell("b1"), Cell("c1"), Cell("d1"), Cell("e1"), Cell("f1"), Cell("g1"), Cell("h1"),
	};
	std::unordered_set expectedSet(expectedVec.begin(), expectedVec.end());

	REQUIRE(rook->cellsToMove(board) == expectedSet);
}

TEST_CASE("Rook in center has 14 moves") {
	Board board;
	const auto rook = new Rook(WHITE);
	board.placePiece(rook, Cell("e5"));

	std::vector expectedVec = {
		Cell("e1"), Cell("e2"), Cell("e3"), Cell("e4"), Cell("e6"), Cell("e7"), Cell("e8"),
		Cell("a5"), Cell("b5"), Cell("c5"), Cell("d5"), Cell("f5"), Cell("g5"), Cell("h5"),
	};
	std::unordered_set expectedSet(expectedVec.begin(), expectedVec.end());

	REQUIRE(rook->cellsToMove(board) == expectedSet);
}

TEST_CASE("Bishop in corner has 7 moves") {
	Board board;
	const auto bishop = new Bishop(WHITE);
	board.placePiece(bishop, Cell("a1"));

	std::vector expectedVec = {
		Cell("b2"), Cell("c3"), Cell("d4"), Cell("e5"), Cell("f6"), Cell("g7"), Cell("h8")
	};
	std::unordered_set expectedSet(expectedVec.begin(), expectedVec.end());

	REQUIRE(bishop->cellsToMove(board) == expectedSet);
}

TEST_CASE("Bishop in center has 13 moves") {
	Board board;
	const auto bishop = new Bishop(WHITE);
	board.placePiece(bishop, Cell("e5"));

	std::vector expectedVec = {
		Cell("b8"), Cell("c7"), Cell("d6"),
		Cell("f4"), Cell("g3"), Cell("h2"),
		Cell("a1"), Cell("b2"), Cell("c3"), Cell("d4"),
		Cell("f6"), Cell("g7"), Cell("h8")
	};
	std::unordered_set expectedSet(expectedVec.begin(), expectedVec.end());

	REQUIRE(bishop->cellsToMove(board) == expectedSet);
}

TEST_CASE("Queen in corner has 21 moves") {
	Board board;
	const auto queen = new Queen(WHITE);
	board.placePiece(queen, Cell("a1"));

	std::vector expectedVec = {
		Cell("a2"), Cell("a3"), Cell("a4"), Cell("a5"), Cell("a6"), Cell("a7"), Cell("a8"),
		Cell("b1"), Cell("c1"), Cell("d1"), Cell("e1"), Cell("f1"), Cell("g1"), Cell("h1"),

		Cell("b2"), Cell("c3"), Cell("d4"), Cell("e5"), Cell("f6"), Cell("g7"), Cell("h8")
	};
	std::unordered_set expectedSet(expectedVec.begin(), expectedVec.end());

	REQUIRE(queen->cellsToMove(board) == expectedSet);
}

TEST_CASE("Bishop in center has 27 moves") {
	Board board;
	const auto queen = new Queen(WHITE);
	board.placePiece(queen, Cell("e5"));

	std::vector expectedVec = {
		Cell("e1"), Cell("e2"), Cell("e3"), Cell("e4"), Cell("e6"), Cell("e7"), Cell("e8"),
		Cell("a5"), Cell("b5"), Cell("c5"), Cell("d5"), Cell("f5"), Cell("g5"), Cell("h5"),

		Cell("b8"), Cell("c7"), Cell("d6"),
		Cell("f4"), Cell("g3"), Cell("h2"),
		Cell("a1"), Cell("b2"), Cell("c3"), Cell("d4"),
		Cell("f6"), Cell("g7"), Cell("h8")
	};
	std::unordered_set expectedSet(expectedVec.begin(), expectedVec.end());

	REQUIRE(queen->cellsToMove(board) == expectedSet);
}

TEST_CASE("Knight in corner has 2 moves") {
	Board board;
	const auto knight = new Knight(WHITE);
	board.placePiece(knight, Cell("a1"));

	std::vector expectedVec = {
		Cell("b3"), Cell("c2")
	};
	std::unordered_set expectedSet(expectedVec.begin(), expectedVec.end());

	REQUIRE(knight->cellsToMove(board) == expectedSet);
}

TEST_CASE("Knight in center has 8 moves") {
	Board board;
	const auto knight = new Knight(WHITE);
	board.placePiece(knight, Cell("e5"));

	std::vector expectedVec = {
		Cell("f7"), Cell("d7"), Cell("c6"), Cell("c4"),
		Cell("d3"), Cell("f3"), Cell("g4"), Cell("g6")
	};
	std::unordered_set expectedSet(expectedVec.begin(), expectedVec.end());

	REQUIRE(knight->cellsToMove(board) == expectedSet);
}

TEST_CASE("White has 43 moves") {
	const Board board("r4bk1/1p3pp1/1qp2n1p/p2p1Q2/P7/2P1P2P/BP4P1/R2R2K1 w - - 0 1");

	REQUIRE(board.getAllLegalMoves(WHITE).size() == 43);
	REQUIRE(board.getAllLegalMoves(BLACK).empty()); // not black to move
}

TEST_CASE("Black has 34 moves") {
	const Board board("r4bk1/1p3pp1/1qp2n1p/p2p1Q2/P7/2P1P2P/BP4P1/R2R2K1 b - - 0 1");

	REQUIRE(board.getAllLegalMoves(WHITE).empty()); // not white to move
	REQUIRE(board.getAllLegalMoves(BLACK).size() == 34);
}

TEST_CASE("Board is not modified after any checks") {
	for (auto const& fen : someFens) {
		const Board board(fen);
		const Board boardCopy(fen);
		const auto tmp = board.getAllLegalMoves(WHITE);
		REQUIRE(board == boardCopy);
	}
}

TEST_CASE("Pinned pawn can't capture en passant") {
	const std::string enPassantFen = "rnb1kbnr/ppp1pppp/4q3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1";
	const Board board(enPassantFen);
	const Move enPassantMove = {Cell("e5"), Cell("d6"), PieceType::none, WHITE};

	REQUIRE_FALSE(board.isMoveLegal(enPassantMove));
}

TEST_CASE("Pawn capture en passant") {
	const std::string enPassantFen = "rnb1kbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1"; // not pinned
	const Board board(enPassantFen);
	const Move enPassantMove = {Cell("e5"), Cell("d6"), PieceType::none, WHITE};

	REQUIRE(board.isMoveLegal(enPassantMove));
}

TEST_CASE("Pawn promotion legal and works correctly") {
	const std::string promotionFen = "8/1P6/8/8/8/2k5/2p5/2K5 w - - 0 1";
	const Board board(promotionFen);

	const auto moves = board.getAllLegalMoves(WHITE);

	REQUIRE(moves.size() == 4);

	for (const auto& move : moves) {
		Board boardCopy(promotionFen);
		boardCopy.makeMove(move);

		REQUIRE(boardCopy.getPiece(Cell("b8"))->getType() == move.promotion);
	}
}

TEST_CASE("Pinned pawn can't promote") {
	const std::string pinnedPromotionFen = "r7/r2P3K/r7/7k/8/8/8/6r1 w - - 0 1";
	const Board board(pinnedPromotionFen);

	REQUIRE(board.getAllLegalMoves(WHITE).empty());

	REQUIRE(board.getGameOutcome() == Outcome::draw);
}

TEST_CASE("Get all legal attacks") {
	const std::string fen = "6r1/5P2/8/8/8/8/r7/7K w - - 0 1";
	const Board board(fen);

	REQUIRE(board.getAllLegalAttacks(WHITE).size() == 2);
}

TEST_CASE("undoMove takes everything back correctly") {
	for (const auto& fen : someFens) {
		Board board(fen);
		Board boardBackup(fen);
		for (const auto& move : board.getAllLegalMoves()) {
			board.makeMove(move);
			board.undoMove(move);

			if (board != boardBackup) {
				break;
			}

			REQUIRE(board == boardBackup);
		}
	}
}

TEST_CASE("swap works correctly") {
	Board board1(someFens[0]);
	Board board1Copy(someFens[0]);
	Board board2(someFens[1]);
	Board board2Copy(someFens[1]);

	board1.swap(board2);

	REQUIRE(board1 == board2Copy);
	REQUIRE(board2 == board1Copy);
}

TEST_CASE("Copy constructor works correctly") {
	for (const auto& fen : someFens) {
		const Board board(fen);
		const Board boardBac(fen);
		Board boardCopy(board);

		REQUIRE(boardCopy == boardBac);

		const auto moves = boardCopy.getAllLegalMoves();
		if (!moves.empty()) {
			boardCopy.makeMove(moves[0]);

			REQUIRE(board == boardBac); // original board is not modified
		}
	}
}

TEST_CASE("Copy assignment works correctly") {
	for (const auto& fen : someFens) {
		const Board board(fen);
		const Board boardBac(fen);
		Board boardCopy = board;

		REQUIRE(boardCopy == boardBac);

		const auto moves = boardCopy.getAllLegalMoves();
		if (!moves.empty()) {
			boardCopy.makeMove(moves[0]);

			REQUIRE(board == boardBac); // original board is not modified
		}
	}
}

TEST_CASE("Move constructor works correctly") {
	for (const auto& fen : someFens) {
		const Board boardBac(fen);
		auto boardCopy = Board(Board(fen));

		REQUIRE(boardCopy == boardBac);
	}
}

TEST_CASE("Move assignment works correctly") {
	for (const auto& fen : someFens) {
		const Board boardBac(fen);
		auto boardCopy = Board(fen);

		REQUIRE(boardCopy == boardBac);
	}
}

TEST_CASE("Triple knight repetition is a draw") {
	Board board;
	board.setStart();

	for (int32_t i = 0; i < 2; ++i) {
		board.makeMove(Cell("g1"), Cell("f3"));
		board.makeMove(Cell("g8"), Cell("f6"));
		board.makeMove(Cell("f3"), Cell("g1"));
		REQUIRE(board.getGameOutcome() == Outcome::ongoing);
		board.makeMove(Cell("f6"), Cell("g8"));
	}

	REQUIRE(board.getGameOutcome() == Outcome::draw);
}

TEST_CASE("Triple kings repetition is a draw") {
	Board board("4K3/8/4k3/8/1pp5/8/8/7n w - - 0 1");

	for (int32_t i = 0; i < 2; ++i) {
		board.makeMove(Cell("e8"), Cell("f8"));
		board.makeMove(Cell("e6"), Cell("f6"));
		board.makeMove(Cell("f8"), Cell("e8"));
		REQUIRE(board.getGameOutcome() == Outcome::ongoing);
		board.makeMove(Cell("f6"), Cell("e6"));
	}

	REQUIRE(board.getGameOutcome() == Outcome::draw);
}

void checkRevoking(Board board) {
	auto from = Cell("b1");
	auto to = Cell("b8");
	if (board.getColorToMove() == BLACK) {
		std::swap(from, to);
	}

	board.makeMove(from, to);

	// capture -- halfMovesClock is set to 0
	REQUIRE(board.getGameOutcome() == Outcome::ongoing);
}

bool fiftyMovesHelper(Board& board, Cell& from, int32_t& cnt, Color const& direction) {
	const auto forward = from.forward(direction);
	board.makeMove(from, forward);
	from = forward;

	if (cnt + 1 == 49) {
		checkRevoking(board);
	}

	if (++cnt < 50) {
		REQUIRE(board.getGameOutcome() == Outcome::ongoing);
	} else {
		REQUIRE(board.getGameOutcome() == Outcome::draw);
		return false;
	}
	return true;
}

TEST_CASE("Fifty moves rule") {
	Board board("kr6/8/8/8/8/8/8/KR2RrRr w - - 0 1");

	auto whiteFirstRook = Cell("g1");
	auto blackFirstRook = Cell("h1");
	auto whiteSecondRook = Cell("e1");
	auto blackSecondRook = Cell("f1");

	int32_t cnt = 0;
	bool go = true;
	Color direction = WHITE;

	while (go) {
		const auto targetCell = direction == WHITE ? Cell("h8") : Cell("h1");
		while (go && board.isFreeCell(targetCell)) {
			if (board.getColorToMove() == WHITE) {
				go = fiftyMovesHelper(board, whiteFirstRook, cnt, direction);
			} else {
				go = fiftyMovesHelper(board, blackFirstRook, cnt, direction);
			}
		}
		go = fiftyMovesHelper(board, whiteSecondRook, cnt, WHITE)
			&& fiftyMovesHelper(board, blackSecondRook, cnt, WHITE);
		direction = oppositeColor(direction);
	}
}

