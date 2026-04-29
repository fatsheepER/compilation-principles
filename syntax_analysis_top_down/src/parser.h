#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "grammar.h"

/*
    输入符号序列示意:
    std::vector<InputToken> input = {
        {Terminal::Id, "1", 0},
        {Terminal::Plus, "+", 1},
        {Terminal::Id, "2", 2},
        {Terminal::End, "#", 3},
    };
*/

struct InputToken {
	Terminal terminal = Terminal::End;
	std::string lexeme;
	std::size_t source_index = 0;
};

struct ParseStep {
	int index = 0;
	std::string stack;
	std::string remaining_input;
	std::string action;
};

struct ParseResult {
	bool accepted = false;
	std::vector<ParseStep> steps;

	std::string error_message;
	std::size_t error_token_index = 0;
};

class PredictiveParser {
  public:
	PredictiveParser();

	ParseResult parse(const std::vector<InputToken> &input) const;

  private:
	Grammar grammer_;

	static std::string stackToString(const std::vector<Symbol> &stack);
	static std::string
	remainingInputToString(const std::vector<InputToken> &input,
	                       std::size_t position);
	static bool isSameTerminal(const Symbol &symbol, Terminal terminal);
};