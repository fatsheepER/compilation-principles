#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class SymbolKind { Terminal, NonTerminal };

// 终结符
enum class Terminal {
	Id,     // i
	Plus,   // +
	Mul,    // *
	LParen, // (
	RParen, // )
	End     // #
};

// 非终结符
enum class NonTerminal {
	E,
	EPrime, // E'
	T,
	TPrime, // T'
	F
};

// 统一符号结构体
struct Symbol {
	SymbolKind kind;
	Terminal terminal = Terminal::End;
	NonTerminal non_terminal = NonTerminal::E;

	static Symbol terminalSymbol(Terminal terminal);
	static Symbol nonTerminalSymbol(NonTerminal non_terminal);
};

// 产生式
struct Production {
	int id = 0;
	NonTerminal lhs = NonTerminal::E;

	// 空 rhs 表示 epsilon 产生式
	std::vector<Symbol> rhs;
};

// 文法
class Grammar {
  public:
	Grammar();

	// 查找预测分析表
	const Production *lookup(NonTerminal non_terminal,
	                         Terminal lookahead) const;

	const std::vector<Production> &productions() const;

  private:
	// 产生式
	std::vector<Production> productions_;
	// 预测分析表: (NT, T) -> PID
	std::map<std::pair<NonTerminal, Terminal>, int> table_;

	void addProduction(Production production);
	void addTableEntry(NonTerminal non_terminal, Terminal terminal,
	                   int production_id);
};

std::string toString(Terminal terminal);
std::string toString(NonTerminal nonTerminal);
std::string toString(const Symbol &symbol);
std::string toString(const Production &production);