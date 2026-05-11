#include "token_reader.h"

#include <cctype>
#include <cstddef>
#include <exception>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
constexpr int TK_IDENTIFIER = 0;
constexpr int TK_INT_LITERAL = 1;

constexpr int OP_PLUS = 200;
constexpr int OP_MUL = 202;

constexpr int SEP_SEMICOLON = 301;
constexpr int SEP_LPAREN = 302;
constexpr int SEP_RPAREN = 303;
} // namespace

TokenReaderResult
TokenReader::read(const std::string &token_path,
                  const std::string &lexical_error_path) const {
	TokenReaderResult result;

	// 读取词法分析报错文件
	const std::string lexical_errors = trim(readWholeFile(lexical_error_path));

	// 如果词法分析有报错
	if (!lexical_errors.empty()) {
		result.lexical_passed = false;
		result.lexical_error_text = lexical_errors;
		return result;
	}

	std::ifstream ifs(token_path);
	if (!ifs) {
		throw std::runtime_error("无法打开路径下的 token 文件: " + token_path);
	}

	ExpressionInput current;
	current.index = 1;

	bool has_current_expression_content = false;

	std::string line;
	std::size_t line_number = 0;

	// 逐行读取 token
	while (std::getline(ifs, line)) {
		++line_number;

		// 跳过空行
		if (trim(line).empty()) {
			continue;
		}

		RawToken raw;

		try {
			raw = parseTokenLine(line, line_number);
		} catch (const std::exception &ex) {
			// 解析 token 行出错
			current.errors.push_back({
			    current.index,
			    line_number,
			    ex.what(),
			});
			has_current_expression_content = true;
			continue;
		}

		// 读到 ';'，该表达式读取完毕
		if (raw.code == SEP_SEMICOLON) {
			// 什么都没读到 -> 空表达式
			if (!has_current_expression_content && current.tokens.empty() &&
			    current.errors.empty()) {
				current.errors.push_back({
				    current.index,
				    raw.line,
				    "空表达式",
				});
			}

			current.tokens.push_back({
			    Terminal::End,
			    "#",
			    raw.line,
			});

			result.expressions.push_back(current);

			current = ExpressionInput{}; // 重置表达式
			current.index = result.expressions.size() + 1;
			has_current_expression_content = false;
			continue;
		}

		// 读到内容 token
		has_current_expression_content = true;

		Terminal terminal = Terminal::End;
		std::string error_message;

		// 将 token 映射为终结符
		if (tryMapTerminal(raw, terminal, error_message)) {
			current.tokens.push_back({
			    terminal,
			    raw.lexeme,
			    raw.line,
			});
		}
		else {
			current.errors.push_back({
			    current.index,
			    raw.line,
			    error_message,
			});
		}
	}

	// 读取完毕后仍有未归纳到表达式的 token 或报错
	if (has_current_expression_content || !current.tokens.empty() ||
	    !current.errors.empty()) {
		current.errors.push_back({
		    current.index,
		    line_number,
		    "表达式缺少结束分号 ';'",
		});

		result.expressions.push_back(current);
	}

	// 啥都没有读到
	if (result.expressions.empty() && result.file_errors.empty()) {
		result.file_errors.push_back({
		    0,
		    0,
		    "token 文件中没有可分析的表达式",
		});
	}

	return result;
};

std::string TokenReader::readWholeFile(const std::string &path) {
	std::ifstream ifs(path);
	if (!ifs) {
		throw std::runtime_error("无法打开路径下的文件: " + path);
	}

	std::ostringstream oss;
	oss << ifs.rdbuf();
	return oss.str();
}

std::string TokenReader::trim(const std::string &text) {
	std::size_t begin = 0;
	while (begin < text.size() &&
	       std::isspace(static_cast<unsigned char>(text[begin]))) {
		++begin;
	}

	std::size_t end = text.size();
	while (end > begin &&
	       std::isspace(static_cast<unsigned char>(text[end - 1]))) {
		--end;
	}

	return text.substr(begin, end - begin);
}

TokenReader::RawToken TokenReader::parseTokenLine(const std::string &line,
                                                  std::size_t line_number) {
	const std::string text = trim(line);

	// 标准 token 行格式: (=, 210)

	// 检查 token 行格式是否正确
	if (text.size() < 5 || text.front() != '(' || text.back() != ')') {
		throw std::runtime_error("token 行格式错误: " + line);
	}

	// 检查中间是否有逗号分隔符
	const std::string body = text.substr(1, text.size() - 2);
	const std::size_t comma_pos = body.rfind(',');

	if (comma_pos == std::string::npos) {
		throw std::runtime_error("token 行缺少逗号分隔符: " + line);
	}

	// 拆分前项和后项
	const std::string lexeme = trim(body.substr(0, comma_pos));
	const std::string code_text = trim(body.substr(comma_pos + 1));

	if (lexeme.empty() || code_text.empty()) {
		throw std::runtime_error("token 行内容不完整: " + line);
	}

	// 解析种别码字符串为整数
	int code = -1;

	try {
		std::size_t consumed = 0;
		code = std::stoi(code_text, &consumed);

		if (consumed != code_text.size()) {
			throw std::runtime_error("invalid token code");
		}
	} catch (...) {
		throw std::runtime_error("token 种别码不是合法整数: " + line);
	}

	return {
	    lexeme,
	    code,
	    line_number,
	};
}

bool TokenReader::tryMapTerminal(const RawToken &raw, Terminal &terminal,
                                 std::string &error_message) {
	switch (raw.code) {
	case TK_IDENTIFIER:
		if (raw.lexeme == "i") {
			terminal = Terminal::Id;
			return true;
		}

		error_message = "表达式文法只允许标识符 i，不允许标识符 " + raw.lexeme;
		return false;

	case TK_INT_LITERAL:
		terminal = Terminal::Id;
		return true;

	case OP_PLUS:
		terminal = Terminal::Plus;
		return true;

	case OP_MUL:
		terminal = Terminal::Mul;
		return true;

	case SEP_LPAREN:
		terminal = Terminal::LParen;
		return true;

	case SEP_RPAREN:
		terminal = Terminal::RParen;
		return true;

	default:
		std::ostringstream oss;
		oss << "token 不在本实验文法范围内: ";
		oss << raw.lexeme << ", 种别码 " << raw.code;

		error_message = oss.str();
		return false;
	}
}