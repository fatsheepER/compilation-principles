#include "grammar.h"

#include <cstddef>
#include <sstream>
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
	using NT = NonTerminal;
	using T = Terminal;

	// STEP 1. 添加产生式

	// 1. E -> T E'
	addProduction({
	    1,
	    NT::E,
	    {
	        Symbol::nonTerminalSymbol(NT::T),
	        Symbol::nonTerminalSymbol(NT::EPrime),
	    },
	});

	// 2. E' -> + T E'
	addProduction({
	    2,
	    NT::EPrime,
	    {
	        Symbol::terminalSymbol(T::Plus),
	        Symbol::nonTerminalSymbol(NT::T),
	        Symbol::nonTerminalSymbol(NT::EPrime),
	    },
	});

	// 3. E' -> epsilon
	addProduction({
	    3,
	    NT::EPrime,
	    {},
	});

	// 4. T -> F T'
	addProduction({
	    4,
	    NT::T,
	    {
	        Symbol::nonTerminalSymbol(NT::F),
	        Symbol::nonTerminalSymbol(NT::TPrime),
	    },
	});

	// 5. T' -> * F T'
	addProduction({
	    5,
	    NT::TPrime,
	    {
	        Symbol::terminalSymbol(T::Mul),
	        Symbol::nonTerminalSymbol(NT::F),
	        Symbol::nonTerminalSymbol(NT::TPrime),
	    },
	});

	// 6. T' -> epsilon
	addProduction({
	    6,
	    NT::TPrime,
	    {},
	});

	// 7. F -> ( E )
	addProduction({
	    7,
	    NT::F,
	    {
	        Symbol::terminalSymbol(T::LParen),
	        Symbol::nonTerminalSymbol(NT::E),
	        Symbol::terminalSymbol(T::RParen),
	    },
	});

	// 8. F -> i
	addProduction({
	    8,
	    NT::F,
	    {
	        Symbol::terminalSymbol(T::Id),
	    },
	});

	// STEP 2. 添加表项

	addTableEntry(NT::E, T::Id, 1);
	addTableEntry(NT::E, T::LParen, 1);

	addTableEntry(NT::EPrime, T::Plus, 2);
	addTableEntry(NT::EPrime, T::RParen, 3);
	addTableEntry(NT::EPrime, T::End, 3);

	addTableEntry(NT::T, T::Id, 4);
	addTableEntry(NT::T, T::LParen, 4);

	addTableEntry(NT::TPrime, T::Plus, 6);
	addTableEntry(NT::TPrime, T::Mul, 5);
	addTableEntry(NT::TPrime, T::RParen, 6);
	addTableEntry(NT::TPrime, T::End, 6);

	addTableEntry(NT::F, T::Id, 8);
	addTableEntry(NT::F, T::LParen, 7);
}

const Production *Grammar::lookup(NonTerminal non_terminal,
                                  Terminal lookahead) const {
	const auto key = std::make_pair(non_terminal, lookahead);
	const auto it = table_.find(key);

	if (it == table_.end()) {
		return nullptr;
	}

	const int production_id = it->second;
	for (const auto &production : productions_) {
		if (production.id == production_id) {
			return &production;
		}
	}

	return nullptr;
}

const std::vector<Production> &Grammar::productions() const {
	return productions_;
}

void Grammar::addProduction(Production production) {
	productions_.push_back(std::move(production));
}

void Grammar::addTableEntry(NonTerminal non_terminal, Terminal terminal,
                            int production_id) {
	table_[std::make_pair(non_terminal, terminal)] = production_id;
}

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

std::string toString(NonTerminal non_terminal) {
	switch (non_terminal) {
	case NonTerminal::E:
		return "E";
	case NonTerminal::EPrime:
		return "E'";
	case NonTerminal::T:
		return "T";
	case NonTerminal::TPrime:
		return "T'";
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

	// PID: lhs -> r h s
	oss << production.id << ": ";
	oss << toString(production.lhs) << "->";

	if (production.rhs.empty()) {
		oss << "epsilon";
		return oss.str();
	}

	for (std::size_t i = 0; i < production.rhs.size(); ++i) {
		if (i > 0) {
			oss << ' ';
		}
		oss << toString(production.rhs[i]);
	}

	return oss.str();
}