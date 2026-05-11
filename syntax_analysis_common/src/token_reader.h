#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "input_token.h"
#include "terminal.h"

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

// TokenReader 读取完整个 token 文件后的输出
// 如果 token 文件有词法分析错误，直接输出错误文本
// 如果 token 文件通过了词法分析，会整理所有表达式以及 token 级错误
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
		std::string lexeme;   // token 字符串量
		int code = -1;        // token 对应种别码
		std::size_t line = 0; // token 所在行号
	};

	// 用于读取错误文件判断是否为空
	static std::string readWholeFile(const std::string &path);

	static std::string trim(const std::string &text);

	static RawToken parseTokenLine(const std::string &line,
	                               std::size_t line_number);

	static bool tryMapTerminal(const RawToken &raw, Terminal &terminal,
	                           std::string &error_message);
};