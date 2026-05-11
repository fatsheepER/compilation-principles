#include "terminal.h"

std::string toString(Terminal terminal) {
	switch (terminal) {
	case Terminal::Id:
		return "i";
	case Terminal::Plus:
		return "+";
	case Terminal::Mul:
		return "*";
	case Terminal::LParen:
		return "(";
	case Terminal::RParen:
		return ")";
	case Terminal::End:
		return "#";
	}

	return "?";
}
