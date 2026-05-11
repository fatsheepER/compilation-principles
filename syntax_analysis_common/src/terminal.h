#pragma once

#include <string>

// 终结符
enum class Terminal {
	Id,     // i
	Plus,   // +
	Mul,    // *
	LParen, // (
	RParen, // )
	End     // #
};

std::string toString(Terminal terminal);
