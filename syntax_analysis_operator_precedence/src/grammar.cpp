#include "grammar.h"
#include "terminal.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

Symbol Symbol::terminalSymbol(Terminal terminal) {
	Symbol symbol;
	symbol.kind = SymbolKind::Terminal;
	symbol.terminal = terminal;
	return symbol;
}

Symbol Symbol::nonTerminalSymbol(NonTerminal non_terminal) {
	Symbol symbol;
	symbol.kind = SymbolKind::NonTerminal;
	symbol.non_terminal = non_terminal;
	return symbol;
}

Grammar::Grammar() {
	terminals_ = {
	    Terminal::Plus,   Terminal::Mul, Terminal::LParen,
	    Terminal::RParen, Terminal::Id,  Terminal::End,
	};

	non_terminals_ = {
	    NonTerminal::E,
	    NonTerminal::T,
	    NonTerminal::F,
	};

	// 1. E -> E + T
	addProduction({
	    1,
	    NonTerminal::E,
	    {
	        Symbol::nonTerminalSymbol(NonTerminal::E),
	        Symbol::terminalSymbol(Terminal::Plus),
	        Symbol::nonTerminalSymbol(NonTerminal::T),
	    },
	});

	// 2. E -> T
	addProduction({
	    2,
	    NonTerminal::E,
	    {
	        Symbol::nonTerminalSymbol(NonTerminal::T),
	    },
	});

	// 3. T -> T * F
	addProduction({
	    3,
	    NonTerminal::T,
	    {
	        Symbol::nonTerminalSymbol(NonTerminal::T),
	        Symbol::terminalSymbol(Terminal::Mul),
	        Symbol::nonTerminalSymbol(NonTerminal::F),
	    },
	});

	// 4. T -> F
	addProduction({
	    4,
	    NonTerminal::T,
	    {
	        Symbol::nonTerminalSymbol(NonTerminal::F),
	    },
	});

	// 5. F -> ( E )
	addProduction({
	    5,
	    NonTerminal::F,
	    {
	        Symbol::terminalSymbol(Terminal::LParen),
	        Symbol::nonTerminalSymbol(NonTerminal::E),
	        Symbol::terminalSymbol(Terminal::RParen),
	    },
	});

	// 6. F -> i
	addProduction({
	    6,
	    NonTerminal::F,
	    {
	        Symbol::terminalSymbol(Terminal::Id),
	    },
	});
}

NonTerminal Grammar::startSymbol() const { return NonTerminal::E; }

const std::vector<Terminal> &Grammar::terminals() const { return terminals_; }

const std::vector<NonTerminal> &Grammar::nonTerminals() const {
	return non_terminals_;
}

const std::vector<Production> &Grammar::productions() const {
	return productions_;
}

void Grammar::addProduction(Production production) {
	productions_.push_back(std::move(production));
}

std::string toString(NonTerminal non_terminal) {
	switch (non_terminal) {
	case NonTerminal::E:
		return "E";
	case NonTerminal::T:
		return "T";
	case NonTerminal::F:
		return "F";
	}

	return "?";
}

std::string toString(const Symbol &symbol) {
	if (symbol.kind == SymbolKind::Terminal) {
		return toString(symbol.terminal);
	}

	return toString(symbol.non_terminal);
}

std::string toString(const Production &production) {
	std::ostringstream oss;

	oss << production.id << ": ";
	oss << toString(production.lhs) << " -> ";

	for (std::size_t i = 0; i < production.rhs.size(); ++i) {
		if (i > 0) {
			oss << ' ';
		}
		oss << toString(production.rhs[i]);
	}

	return oss.str();
}
