#pragma once

#include <string>
#include <vector>

#include "terminal.h"

// NonTerminal

enum class NonTerminal {
	AugmentedStart, // 增广文法开始符号 S'
	S,
	E,
	T,
	F
};

std::string toString(NonTerminal non_terminal);

// Symbol

enum class SymbolKind { Terminal, NonTerminal };

struct Symbol {
	SymbolKind kind = SymbolKind::Terminal;
	Terminal terminal = Terminal::End;
	NonTerminal non_terminal = NonTerminal::E;
};

Symbol makeTerminal(Terminal terminal);
Symbol makeNonTerminal(NonTerminal non_terminal);

bool isTerminal(const Symbol &symbol);
bool isNonTerminal(const Symbol &symbol);

bool operator==(const Symbol &lhs, const Symbol &rhs);
bool operator!=(const Symbol &lhs, const Symbol &rhs);
bool operator<(const Symbol &lhs, const Symbol &rhs);

std::string toString(const Symbol &symbol);

// Production

struct Production {
	int id = 0;
	NonTerminal lhs = NonTerminal::S;
	std::vector<Symbol> rhs;
};

std::string toString(const Production &production);

// Grammar

class Grammar {
  public:
	Grammar();

	const std::vector<Production> &productions() const;

	const Production &production(int id) const;

	NonTerminal augmentedStartSymbol() const;
	NonTerminal startSymbol() const;

	std::vector<Terminal> terminals() const;
	std::vector<NonTerminal> nonTerminals() const;

	std::vector<Symbol> symbolsForAutomatonExpansion() const;

	std::vector<const Production *> productionsFor(NonTerminal lhs) const;

  private:
	std::vector<Production> productions_;
};
