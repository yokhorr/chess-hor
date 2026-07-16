#pragma once

enum class Color {
	white,
	black
};

inline Color oppositeColor(Color const& color) {
	return color == Color::white ? Color::black : Color::white;
}

inline std::size_t colorIndex(Color const& color) {
	return color == Color::white ? 0 : 1;
}

inline std::string colorName(Color const& color) {
	return color == Color::white ? "белые" : "чёрные";
}
