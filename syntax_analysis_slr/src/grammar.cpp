#include "grammar.h"
#include "terminal.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int terminalOrder(Terminal terminal) {
	return static_cast<int>(terminal);
}

int nonTerminalOrder(NonTerminal non_terminal) {
	return static_cast<int>(non_terminal);
}
} // namespace

// NonTerminal

std::string toString(NonTerminal non_terminal) {
	switch (non_terminal) {
	case NonTerminal::AugmentedStart:
		return "S'";
	case NonTerminal::S:
		return "S";
	case NonTerminal::E:
		return "E";
	case NonTerminal::T:
		return "T";
	case NonTerminal::F:
		return "F";
	}

	return "?";
}

// Symbol

Symbol makeTerminal(Terminal terminal) {
	Symbol symbol;
	symbol.kind = SymbolKind::Terminal;
	symbol.terminal = terminal;
	return symbol;
}

Symbol makeNonTerminal(NonTerminal non_terminal) {
	Symbol symbol;
	symbol.kind = SymbolKind::NonTerminal;
	symbol.non_terminal = non_terminal;
	return symbol;
}

bool isTerminal(const Symbol &symbol) {
	return symbol.kind == SymbolKind::Terminal;
}

bool isNonTerminal(const Symbol &symbol) {
	return symbol.kind == SymbolKind::NonTerminal;
}

bool operator==(const Symbol &lhs, const Symbol &rhs) {
	if (lhs.kind != rhs.kind) {
		return false;
	}

	if (lhs.kind == SymbolKind::Terminal) {
		return lhs.terminal == rhs.terminal;
	}

	return lhs.non_terminal == rhs.non_terminal;
}

bool operator!=(const Symbol &lhs, const Symbol &rhs) {
	return !(lhs == rhs);
}

bool operator<(const Symbol &lhs, const Symbol &rhs) {
	if (lhs.kind != rhs.kind) {
		return lhs.kind < rhs.kind;
	}

	if (lhs.kind == SymbolKind::Terminal) {
		return terminalOrder(lhs.terminal) < terminalOrder(rhs.terminal);
	}

	return nonTerminalOrder(lhs.non_terminal) <
	       nonTerminalOrder(rhs.non_terminal);
}

std::string toString(const Symbol &symbol) {
	if (symbol.kind == SymbolKind::Terminal) {
		return toString(symbol.terminal);
	}

	return toString(symbol.non_terminal);
}

// Production

std::string toString(const Production &production) {
	std::ostringstream oss;

	oss << production.id << ": ";
	oss << toString(production.lhs) << " -> ";

	if (production.rhs.empty()) {
		oss << "ε";
		return oss.str();
	}

	for (std::size_t i = 0; i < production.rhs.size(); ++i) {
		if (i > 0) {
			oss << " ";
		}

		oss << toString(production.rhs[i]);
	}

	return oss.str();
}

// Grammar

Grammar::Grammar() {
	productions_ = {
	    {0,
	     NonTerminal::AugmentedStart,
	     {
	         makeNonTerminal(NonTerminal::S),
	     }},
	    {
	        1,
	        NonTerminal::S,
	        {
	            makeNonTerminal(NonTerminal::E),
	        },
	    },
	    {
	        2,
	        NonTerminal::E,
	        {
	            makeNonTerminal(NonTerminal::E),
	            makeTerminal(Terminal::Plus),
	            makeNonTerminal(NonTerminal::T),
	        },
	    },
	    {
	        3,
	        NonTerminal::E,
	        {
	            makeNonTerminal(NonTerminal::T),
	        },
	    },
	    {
	        4,
	        NonTerminal::T,
	        {
	            makeNonTerminal(NonTerminal::T),
	            makeTerminal(Terminal::Mul),
	            makeNonTerminal(NonTerminal::F),
	        },
	    },
	    {
	        5,
	        NonTerminal::T,
	        {
	            makeNonTerminal(NonTerminal::F),
	        },
	    },
	    {
	        6,
	        NonTerminal::F,
	        {
	            makeTerminal(Terminal::LParen),
	            makeNonTerminal(NonTerminal::E),
	            makeTerminal(Terminal::RParen),
	        },
	    },
	    {
	        7,
	        NonTerminal::F,
	        {
	            makeTerminal(Terminal::Id),
	        },
	    },
	};
}

const std::vector<Production> &Grammar::productions() const {
	return productions_;
}

const Production &Grammar::production(int id) const {
	for (const Production &production : productions_) {
		if (production.id == id) {
			return production;
		}
	}

	throw std::out_of_range("unknown production id: " + std::to_string(id));
}

NonTerminal Grammar::augmentedStartSymbol() const {
	return NonTerminal::AugmentedStart;
}

NonTerminal Grammar::startSymbol() const {
	return NonTerminal::S;
}

std::vector<Terminal> Grammar::terminals() const {
	return {
	    Terminal::Id,     Terminal::Plus,   Terminal::Mul,
	    Terminal::LParen, Terminal::RParen, Terminal::End,
	};
}

std::vector<NonTerminal> Grammar::nonTerminals() const {
	return {
	    NonTerminal::S,
	    NonTerminal::E,
	    NonTerminal::T,
	    NonTerminal::F,
	};
}

std::vector<Symbol> Grammar::symbolsForAutomatonExpansion() const {
	return {
	    makeNonTerminal(NonTerminal::S), makeNonTerminal(NonTerminal::E),
	    makeNonTerminal(NonTerminal::T), makeNonTerminal(NonTerminal::F),
	    makeTerminal(Terminal::Plus),    makeTerminal(Terminal::Mul),
	    makeTerminal(Terminal::LParen),  makeTerminal(Terminal::RParen),
	    makeTerminal(Terminal::Id),
	};
}

std::vector<const Production *> Grammar::productionsFor(NonTerminal lhs) const {
	std::vector<const Production *> result;

	for (const Production &production : productions_) {
		if (production.lhs == lhs) {
			result.push_back(&production);
		}
	}

	return result;
}
