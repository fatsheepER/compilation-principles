#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "input_token.h"
#include "precedence_table.h"
#include "terminal.h"

enum class StackSymbolKind {
	Terminal,
	NonTerminalPlaceholder,
};

struct StackSymbol {
	StackSymbolKind kind = StackSymbolKind::Terminal;
	Terminal terminal = Terminal::End;

	static StackSymbol terminalSymbol(Terminal terminal);
	static StackSymbol nonTerminalPlaceholder();
};

struct OperatorPrecedenceStep {
	int index = 0;
	std::string stack;
	std::string remaining_input;
	std::string relation;
	std::string action;
};

struct OperatorPrecedenceParseResult {
	bool accepted = false;
	std::vector<OperatorPrecedenceStep> steps;

	std::string error_message;
	std::size_t error_token_index = 0;
};

class OperatorPrecedenceParser {
  public:
	OperatorPrecedenceParseResult
	parse(const std::vector<InputToken> &raw_input,
	      const PrecedenceTable &precedence_table) const;

  private:
	struct TerminalPosition {
		bool found = false;
		std::size_t index = 0;
		Terminal terminal = Terminal::End;
	};

	struct HandleSearchResult {
		bool success = false;
		std::size_t begin = 0;
		std::string error_message;
	};

	static TerminalPosition
	topMostTerminal(const std::vector<StackSymbol> &stack);

	static TerminalPosition
	previousTerminalBefore(const std::vector<StackSymbol> &stack,
	                       std::size_t before_index);

	static HandleSearchResult
	findHandle(const std::vector<StackSymbol> &stack,
	           const PrecedenceTable &precedence_table);

	static bool canReduceToN(const std::vector<StackSymbol> &phrase);

	// toString()s

	static std::string stackToString(const std::vector<StackSymbol> &stack);

	static std::string
	remainingInputToString(const std::vector<InputToken> &input,
	                       std::size_t position);

	static std::string phraseToString(const std::vector<StackSymbol> &phrase);

	static std::string toString(const StackSymbol &symbol);

	static bool isAcceptStack(const std::vector<StackSymbol> &stack);
};