#pragma once

#include <string>
#include <vector>

#include "terminal.h"

enum class SymbolKind { Terminal, NonTerminal };

enum class NonTerminal {
	E,
	T,
	F,
};

struct Symbol {
	SymbolKind kind = SymbolKind::Terminal;
	Terminal terminal = Terminal::End;
	NonTerminal non_terminal = NonTerminal::E;

	static Symbol terminalSymbol(Terminal);
	static Symbol nonTerminalSymbol(NonTerminal);
};

struct Production {
	int id = 0;
	NonTerminal lhs = NonTerminal::E;
	std::vector<Symbol> rhs;
};

class Grammar {
  public:
	Grammar();

	NonTerminal startSymbol() const;

	const std::vector<Terminal> &terminals() const;
	const std::vector<NonTerminal> &nonTerminals() const;
	const std::vector<Production> &productions() const;

  private:
	std::vector<Terminal> terminals_;
	std::vector<NonTerminal> non_terminals_;
	std::vector<Production> productions_;

	void addProduction(Production);
};

std::string toString(NonTerminal);
std::string toString(const Symbol &);
std::string toString(const Production &);