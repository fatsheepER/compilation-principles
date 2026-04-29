#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "grammar.h"
#include "parser.h"

struct TokenReadError {
	std::size_t expression_index = 0;
	std::size_t line = 0;
	std::string message;
};

struct ExpressionInput {
	std::size_t index = 0;
	std::vector<InputToken> tokens;
	std::vector<TokenReadError> errors;
};

struct TokenReaderResult {
	bool lexical_passed = true;
	std::string lexical_error_text;

	std::vector<ExpressionInput> expressions;
	std::vector<TokenReadError> file_errors;
};

class TokenReader {
  public:
	TokenReaderResult read(const std::string &token_path,
	                       const std::string &lexical_error_path) const;

  private:
	struct RawToken {
		std::string lexeme;
		int code = -1;
		std::size_t line = 0;
	};

	static std::string readWholeFile(const std::string &path);
	static std::string trim(const std::string &text);

	static RawToken parseTokenLine(const std::string &line,
	                               std::size_t line_number);

	static bool tryMapTerminal(const RawToken &raw, Terminal &terminal,
	                           std::string &error_message);
};