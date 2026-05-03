#pragma once

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "evaluator.h"
#include "parser.h"
#include "token_reader.h"

struct ExpressionReport {
	std::size_t index = 0;
	std::vector<InputToken> tokens;
	std::vector<TokenReadError> input_errors;

	bool parser_ran = false;
	ParseResult parse_result;

	bool evaluation_ran = false;
	EvaluationResult evaluation_result;
};

struct ResultWriteSummary {
	std::size_t total = 0;
	std::size_t accpeted = 0;
	std::size_t rejected = 0;
};

class ResultWriter {
  public:
	ResultWriteSummary
	write(const std::string &output_dir, const TokenReaderResult &read_result,
	      const std::vector<ExpressionReport> &reports) const;

  private:
	static std::string
	tokenSequenceToString(const std::vector<InputToken> &tokens);
	static void writeReadError(std::ostream &os, const TokenReadError &error);
};