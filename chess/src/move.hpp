#pragma once
#include "cell.hpp"

enum class PieceType : char;

struct Move {
	Cell from{};
	Cell to{};
	PieceType promotion{};
	Color color{};

	friend bool operator==(const Move& lhs, const Move& rhs) {
		return lhs.from == rhs.from
			&& lhs.to == rhs.to
			&& lhs.promotion == rhs.promotion
			&& lhs.color == rhs.color;
	}
};
