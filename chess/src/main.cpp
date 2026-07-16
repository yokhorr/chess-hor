#include <iostream>
#include <thread>
#include <chrono>

#include "board.hpp"
#include "botPlayer.hpp"
#include "humanPlayer.hpp"
#include "MinmaxedPlayer.hpp"
#include "outcome.hpp"
#include "player.hpp"

void clearScreen() {
	// \033[2J — очистить экран, \033[H — перевести курсор в левый верхний угол
	std::cout << "\033[2J\033[H" << std::flush;
}

int main() {
	Board board;
	board.setStart();
	auto outcome = Outcome::ongoing;
	auto currentColor = Color::white;
	const std::vector<Player*> players = {new MinmaxedPlayer(Color::white), new MinmaxedPlayer(Color::black)};

	std::cout << "Текущая позиция:" << std::endl;
	board.print();

	while (outcome == Outcome::ongoing) {
		std::cout << "Ходят " << colorName(currentColor) << std::endl;

		Move move = players[colorIndex(currentColor)]->makeMove(board);
		while (!board.isMoveLegal(move)) {
			std::cout << "Недопустимый ход. Попробуйте другой." << std::endl;
			move = players[colorIndex(currentColor)]->makeMove(board);
		}

		board.makeMove(move);
		outcome = board.getGameOutcome(oppositeColor(currentColor));
		std::cout << colorName(currentColor) << " сделали ход." << std::endl;
		currentColor = oppositeColor(currentColor);

		std::cout << "Текущая позиция:" << std::endl;
		board.print();
		// std::cout << board.exportToFEN() << std::endl;

		// using namespace std::chrono_literals;
		// std::this_thread::sleep_for(1000ms);
	}

	// board.print();
	currentColor = oppositeColor(currentColor);
	if (outcome == Outcome::mate) {
		std::cout << "Победили " << colorName(currentColor) << ".";
	} else if (outcome == Outcome::stalemate) {
		std::cout << "Ничья.";
	}

	std::cout << std::endl;

	std::cout << board.exportToFEN() << std::endl;
}
