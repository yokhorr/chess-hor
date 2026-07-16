#pragma once

class Board;
enum class Color;
class Cell;
class Piece;
struct Move;

class Player {
public:
	explicit Player(const Color color) : color_(color) {}

	[[nodiscard]] Color getColor() const { return color_; }

	[[nodiscard]] virtual Move makeMove(Board const& board) const = 0;

	virtual ~Player() = default;
private:
	Color color_;
};
