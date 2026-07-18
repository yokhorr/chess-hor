#include "api.hpp"

#include <array>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "board.hpp"
#include "MinmaxedPlayer.hpp"
#include "cell.hpp"
#include "color.hpp"
#include "config.hpp"
#include "move.hpp"
#include "outcome.hpp"
#include "pieceType.hpp"

namespace chessapi {

namespace {

struct FenValidationError final : std::runtime_error {
	using std::runtime_error::runtime_error;
};

PieceType promotionFromChar(const std::string& s) {
	if (s.size() != 1) return PieceType::none;
	switch (s[0]) {
	case 'q': case 'Q': return PieceType::queen;
	case 'r': case 'R': return PieceType::rook;
	case 'b': case 'B': return PieceType::bishop;
	case 'n': case 'N': return PieceType::knight;
	default: return PieceType::none;
	}
}

std::string pieceTypeToStr(const PieceType t) {
	if (t == PieceType::none) return "";
	return std::string(1, static_cast<char>(t));
}

std::string colorToStr(const Color c) {
	return c == Color::white ? "w" : "b";
}

std::string outcomeToStr(const Outcome o) {
	switch (o) {
	case Outcome::mate: return "mate";
	case Outcome::draw: return "draw";
	default: return "ongoing";
	}
}

std::string cellToStr(const Cell& c) {
	if (!isValidCell(c)) return "";
	return c.toString();
}

std::vector<std::string> splitWs(const std::string& s) {
	std::vector<std::string> out;
	std::istringstream iss(s);
	std::string tok;
	while (iss >> tok) out.push_back(tok);
	return out;
}

void validatePlacement(const std::string& placement) {
	std::vector<std::string> ranks;
	{
		std::string cur;
		for (const char c : placement) {
			if (c == '/') {
				ranks.push_back(cur);
				cur.clear();
			} else {
				cur.push_back(c);
			}
		}
		ranks.push_back(cur);
	}

	if (ranks.size() != 8) {
		throw FenValidationError("Некорректная позиция: должно быть 8 горизонталей");
	}

	int whiteKings = 0;
	int blackKings = 0;
	static const std::string validPieces = "pnbrqkPNBRQK";

	for (const auto& rank : ranks) {
		int count = 0;
		for (const char c : rank) {
			if (std::isdigit(static_cast<unsigned char>(c))) {
				const int d = c - '0';
				if (d < 1 || d > 8) {
					throw FenValidationError("Некорректная позиция: неверное число пустых клеток");
				}
				count += d;
			} else {
				if (validPieces.find(c) == std::string::npos) {
					throw FenValidationError("Некорректная позиция: недопустимый символ фигуры");
				}
				if (c == 'K') ++whiteKings;
				if (c == 'k') ++blackKings;
				++count;
			}
			if (count > BOARD_SIZE) {
				throw FenValidationError("Некорректная позиция: переполнение горизонтали");
			}
		}
		if (count != BOARD_SIZE) {
			throw FenValidationError("Некорректная позиция: горизонталь должна содержать 8 клеток");
		}
	}

	if (whiteKings != 1 || blackKings != 1) {
		throw FenValidationError("Некорректная позиция: должно быть ровно по одному королю каждого цвета");
	}
}

void validateActiveColor(const std::string& s) {
	if (s != "w" && s != "b") {
		throw FenValidationError("Некорректный цвет хода в FEN");
	}
}

void validateCastling(const std::string& s) {
	if (s == "-") return;
	if (s.empty() || s.size() > 4) {
		throw FenValidationError("Некорректные права на рокировку в FEN");
	}
	static const std::string allowed = "KQkq";
	std::array<bool, 4> seen{false, false, false, false};
	for (const char c : s) {
		const auto pos = allowed.find(c);
		if (pos == std::string::npos || seen[pos]) {
			throw FenValidationError("Некорректные права на рокировку в FEN");
		}
		seen[pos] = true;
	}
}

void validateEnPassant(const std::string& s) {
	if (s == "-") return;
	if (s.size() != 2 || s[0] < 'a' || s[0] > 'h' || (s[1] != '3' && s[1] != '6')) {
		throw FenValidationError("Некорректное поле взятия на проходе в FEN");
	}
}

// Строго проверяет структуру FEN (чтобы движок не получил данные, на которых
// его посимвольный разбор мог бы выйти за границы строки) и дополняет
// отсутствующие поля счётчиков полуходов, к которым движок безразличен.
std::string normalizeAndValidateFEN(const std::string& fenRaw) {
	auto tokens = splitWs(fenRaw);

	if (tokens.size() < 4) {
		throw FenValidationError("Некорректный FEN: недостаточно полей");
	}
	if (tokens.size() > 6) {
		throw FenValidationError("Некорректный FEN: слишком много полей");
	}

	validatePlacement(tokens[0]);
	validateActiveColor(tokens[1]);
	validateCastling(tokens[2]);
	validateEnPassant(tokens[3]);

	while (tokens.size() < 6) {
		tokens.emplace_back(tokens.size() == 4 ? "0" : "1");
	}

	std::string result;
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (i) result.push_back(' ');
		result += tokens[i];
	}
	return result;
}

std::string buildLegalMovesJSON(const Board& board, const Color color) {
	const auto moves = board.getAllLegalMoves();

	// Группируем по (from, to), собирая варианты превращения в один список.
	std::vector<std::pair<std::pair<Cell, Cell>, std::vector<PieceType>>> grouped;
	for (const auto& m : moves) {
		bool found = false;
		for (auto& g : grouped) {
			if (g.first.first == m.from && g.first.second == m.to) {
				g.second.push_back(m.promotion);
				found = true;
				break;
			}
		}
		if (!found) {
			grouped.push_back({{m.from, m.to}, {m.promotion}});
		}
	}

	std::ostringstream out;
	out << "[";
	bool firstItem = true;
	for (const auto& g : grouped) {
		if (!firstItem) out << ",";
		firstItem = false;

		out << "{\"from\":\"" << cellToStr(g.first.first) << "\","
			<< "\"to\":\"" << cellToStr(g.first.second) << "\","
			<< "\"promotions\":[";

		bool firstP = true;
		for (const auto& p : g.second) {
			if (p == PieceType::none) continue;
			if (!firstP) out << ",";
			firstP = false;
			out << "\"" << pieceTypeToStr(p) << "\"";
		}
		out << "]}";
	}
	out << "]";
	return out.str();
}

std::string errorJSON(const std::string& message) {
	return "{\"ok\":false,\"error\":\"" + message + "\"}";
}

} // namespace

std::string getStartFEN() {
	Board board;
	board.setStart();
	return board.exportToFEN();
}

std::string getStatus(const std::string& fenRaw) {
	try {
		const std::string fen = normalizeAndValidateFEN(fenRaw);
		const Board board(fen);
		const Color color = board.getColorToMove();
		const bool inCheck = board.isKingChecked(color);
		const Outcome outcome = board.getGameOutcome();
		const std::string movesJson = buildLegalMovesJSON(board, color);

		std::ostringstream out;
		out << "{\"ok\":true,"
			<< "\"colorToMove\":\"" << colorToStr(color) << "\","
			<< "\"inCheck\":" << (inCheck ? "true" : "false") << ","
			<< "\"outcome\":\"" << outcomeToStr(outcome) << "\","
			<< "\"legalMoves\":" << movesJson
			<< "}";
		return out.str();
	} catch (const std::exception&) {
		return errorJSON("Некорректная позиция (FEN)");
	}
}

std::string makeMove(
	const std::string& fenRaw,
	const std::string& fromStr,
	const std::string& toStr,
	const std::string& promotionStr
) {
	try {
		const std::string fen = normalizeAndValidateFEN(fenRaw);
		Board board(fen);
		const Color color = board.getColorToMove();

		if (fromStr.size() != 2 || toStr.size() != 2) {
			return errorJSON("Некорректные координаты клетки");
		}

		const Cell from(fromStr);
		const Cell to(toStr);
		if (!isValidCell(from) || !isValidCell(to)) {
			return errorJSON("Некорректные координаты клетки");
		}
		if (board.isFreeCell(from)) {
			return errorJSON("На исходной клетке нет фигуры");
		}

		const PieceType promotion = promotionFromChar(promotionStr);
		const Move move{from, to, promotion, color};

		if (!board.isMoveLegal(move)) {
			return errorJSON("Недопустимый ход");
		}

		const auto involved = board.getMoveInvolvedPieces(move);
		const bool isCapture = involved.capturedPiece != nullptr;
		const bool isCastle = Board::isCastleMove(move, involved.movedPiece);
		const bool isEnPassant =
			isCapture
			&& involved.movedPiece->getType() == PieceType::pawn
			&& to == board.getEnPassantCell();
		const std::string movedPieceStr = pieceTypeToStr(involved.movedPiece->getType());
		const std::string capturedPieceStr = isCapture ? pieceTypeToStr(involved.capturedPiece->getType()) : "";

		board.makeMove(move);

		const std::string newFen = board.exportToFEN();
		const Color opponent = oppositeColor(color);
		const bool inCheck = board.isKingChecked(opponent);
		const Outcome outcome = board.getGameOutcome();

		std::ostringstream out;
		out << "{\"ok\":true,"
			<< "\"fen\":\"" << newFen << "\","
			<< "\"from\":\"" << cellToStr(from) << "\","
			<< "\"to\":\"" << cellToStr(to) << "\","
			<< "\"promotion\":\"" << pieceTypeToStr(promotion) << "\","
			<< "\"color\":\"" << colorToStr(color) << "\","
			<< "\"movedPiece\":\"" << movedPieceStr << "\","
			<< "\"capturedPiece\":\"" << capturedPieceStr << "\","
			<< "\"isCapture\":" << (isCapture ? "true" : "false") << ","
			<< "\"isCastle\":" << (isCastle ? "true" : "false") << ","
			<< "\"isEnPassant\":" << (isEnPassant ? "true" : "false") << ","
			<< "\"colorToMove\":\"" << colorToStr(opponent) << "\","
			<< "\"inCheck\":" << (inCheck ? "true" : "false") << ","
			<< "\"outcome\":\"" << outcomeToStr(outcome) << "\""
			<< "}";
		return out.str();
	} catch (const std::exception&) {
		return errorJSON("Некорректный ход или позиция");
	}
}

std::string getBotMove(const std::string& fenRaw) {
	try {
		const std::string fen = normalizeAndValidateFEN(fenRaw);
		const Board board(fen);
		const Color color = board.getColorToMove();
		MinmaxedPlayer bot(color);

		const auto moves = board.getAllLegalMoves();
		if (moves.empty()) {
			return errorJSON("Нет доступных ходов");
		}

		const Move m = bot.makeMove(board);

		std::ostringstream out;
		out << "{\"ok\":true,"
			<< "\"from\":\"" << cellToStr(m.from) << "\","
			<< "\"to\":\"" << cellToStr(m.to) << "\","
			<< "\"promotion\":\"" << pieceTypeToStr(m.promotion) << "\""
			<< "}";
		return out.str();
	} catch (const std::exception&) {
		return errorJSON("Некорректная позиция (FEN)");
	}
}

} // namespace chessapi
