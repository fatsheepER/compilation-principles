#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "grammar.h"
#include "input_token.h"

/*
    输入符号序列示意:
    std::vector<InputToken> input = {
        {Terminal::Id, "1", 0},
        {Terminal::Plus, "+", 1},
        {Terminal::Id, "2", 2},
        {Terminal::End, "#", 3},
    };
*/

struct ParseStep {
	int index = 0;
	std::string stack;
	std::string remaining_input;
	std::string action; // 使用产生式 or 匹配终结符
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
