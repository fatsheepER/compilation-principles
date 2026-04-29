#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "tables.h"
#include "token.h"

class Lexer {
  public:
	explicit Lexer(std::string source);

	void tokenize();
	Token nextToken();

	const std::vector<Token> &getTokens() const;
	const std::vector<LexError> &getErrors() const;

	const IdentifierTable &getIdentifierTable() const;
	const ConstantTable &getConstantTable() const;

  private:
	// 输入状态
	std::string source_;
	std::size_t pos_ = 0;
	int line_ = 1;
	int column_ = 1;

	// 扫描结果
	std::vector<Token> tokens_;
	std::vector<LexError> errors_;

	// 表
	IdentifierTable identifierTable_;
	ConstantTable constantTable_;

  private:
	// 基础字符流操作
	bool isAtEnd() const;
	char peek() const;     // 查看当前字符
	char peekNext() const; // 查看下一个字符
	char advance();        // 光标前进，维护行列号，返回上一个字符

	// 分类辅助
	bool isIdentifierStart(char ch) const;
	bool isIdentifierPart(char ch) const;
	bool isOctDigit(char ch) const;
	bool isHexDigit(char ch) const;

	// 跳过类操作
	void skipWhitespace();
	bool skipComment();
	void skipUntilSeparatorOrWhitespace(); // 跳过非法字符
	void skipUntilTokenBoundary();         // 跳过到下一个 token 边界
	void skipUntilCharLiteralEnd();        // 跳过到字符常量结束
	void skipUntilStringLiteralEnd();      // 跳过到字符串常量结束

	// 各类扫描函数
	Token scanIdentifierOrKeyword();
	Token scanNumber();
	Token scanCharLiteral();
	Token scanStringLiteral();
	Token scanOperatorOrDelimiter();

	// 构造 token / error
	Token makeToken(TokenType type, const std::string &lexeme, int startLine,
	                int startColumn, int attr = -1) const;

	Token makeErrorToken(const std::string &lexeme, int startLine,
	                     int startColumn) const;

	void reportError(ErrorCode code, const std::string &message,
	                 const std::string &fragment, int errorLine,
	                 int errorColumn);
};
