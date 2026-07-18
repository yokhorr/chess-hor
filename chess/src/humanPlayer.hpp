#pragma once
#include <iostream>

#include "move.hpp"
#include "player.hpp"

class HumanPlayer final : public Player {
public:
	explicit HumanPlayer(const Color color) : Player(color) {}

	[[nodiscard]] Move makeMove(Board board) override {
		std::cout << "Введите ход в формате: `e2 e4 ?`" << std::endl;
		char x1c, x2c, y1c, y2c, pc;
		std::cin >> x1c >> y1c >> x2c >> y2c >> pc;
		const int32_t x1 = x1c - 'a';
		const int32_t x2 = x2c - 'a';
		const int32_t y1 = y1c - '0' - 1;
		const int32_t y2 = y2c - '0' - 1;

		PieceType promotion;
		switch (pc) {
		case 'Q':
			promotion = PieceType::queen;
			break;
		case 'R':
			promotion = PieceType::rook;
			break;
		case 'B':
			promotion = PieceType::bishop;
			break;
		case 'N':
			promotion = PieceType::knight;
			break;
		default:
			promotion = PieceType::none;
		}

		return Move(Cell(x1, y1), Cell(x2, y2), promotion,  getColor());
	}
};
