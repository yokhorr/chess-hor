#pragma once
#include <functional>
#include <ostream>
#include <stdexcept>
#include "color.hpp"

class Cell {
public:
	int32_t x = -1;
	int32_t y = -1;

	Cell() = default;

	explicit Cell(std::string const& str) {
		if (str.size() != 2) {
			throw std::invalid_argument("Cell constructor from string takes a string of exactly 2 characters");
		}

		x = str[0] - 'a';
		y = str[1] - '1';
	}

	Cell(const int32_t x, const int32_t y) : x(x), y(y) {
	}

	bool operator==(const Cell& other) const {
		return x == other.x && y == other.y;
	}

	bool operator!=(const Cell& other) const {
		return !(*this == other);
	}

	[[nodiscard]] Cell forward(const Color color) const {
		if (color == Color::white) {
			return {x, y + 1};
		}
		return {x, y - 1};
	}

	[[nodiscard]] std::string toString() const {
		std::string ans;

		ans.push_back(static_cast<char>('a' + x));
		ans.push_back(static_cast<char>('0' + y + 1));

		return ans;
	}
};

inline Cell operator+(const Cell& lhs, const Cell& rhs) {
	return {lhs.x + rhs.x, lhs.y + rhs.y};
}

template <>
struct std::hash<Cell> {
	std::size_t operator()(const Cell& s) const noexcept {
		// ReSharper disable once CppRedundantParentheses
		return std::hash<int32_t>{}(s.x) ^ (std::hash<int32_t>{}(s.y) << 1);
	}
};
