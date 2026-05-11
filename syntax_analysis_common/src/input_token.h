#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "terminal.h"

struct InputToken {
	Terminal terminal = Terminal::End;
	std::string lexeme;
	std::size_t source_index = 0;
};