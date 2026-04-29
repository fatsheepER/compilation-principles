#pragma once

#include <string>
#include <vector>

#include "token.h"

struct KeywordEntry {
	std::string lexeme; // 关键字原文
	TokenType type;     // 对应种别码
};

struct OperatorEntry {
	std::string lexeme; // 运算符原文
	TokenType type;     // 对应种别码
};

struct DelimiterEntry {
	std::string lexeme; // 界限符原文
	TokenType type;     // 对应种别码
};

struct IdentifierEntry {
	int index = -1;   // 标识符表项序号
	std::string name; // 标识符原文
};

struct ConstantEntry {
	int index = -1;                             // 常量表项序号
	std::string literal;                        // 常量字面量文本
	TokenType type = TokenType::TK_INT_LITERAL; // 常量类型
};

class IdentifierTable {
  public:
	int find(const std::string &name) const;
	int addOrGet(const std::string &name);
	void clear();

	const std::vector<IdentifierEntry> &entries() const;

  private:
	std::vector<IdentifierEntry> entries_;
};

class ConstantTable {
  public:
	int find(const std::string &literal, TokenType type) const;
	int addOrGet(const std::string &literal, TokenType type);
	void clear();

	const std::vector<ConstantEntry> &entries() const;

  private:
	std::vector<ConstantEntry> entries_;
};

// 关键字 / 运算符 / 界限符 查询工具
bool isKeyword(const std::string &lexeme);
TokenType keywordTypeOf(const std::string &lexeme);

bool matchOperator(const std::string &lexeme, TokenType &outType);
bool matchDelimiter(const std::string &lexeme, TokenType &outType);

bool isOperatorStart(char ch);
bool isDelimiterStart(char ch);
bool isEscapeChar(char ch);