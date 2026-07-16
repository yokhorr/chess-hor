#pragma once

struct CastleRight {
	bool kingside = false;
	bool queenside = false;

	bool operator==(const CastleRight&) const = default;
};
