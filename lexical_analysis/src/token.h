#pragma once

#include <string>

enum class TokenType : int {
	// 标识符与常量
	TK_IDENTIFIER = 0,
	TK_INT_LITERAL = 1,
	TK_CHAR_LITERAL = 2,
	TK_STRING_LITERAL = 3,

	// 关键字
	KW_INT = 100,
	KW_CHAR = 101,
	KW_VOID = 102,
	KW_IF = 103,
	KW_ELSE = 104,
	KW_WHILE = 105,
	KW_FOR = 106,
	KW_DO = 107,
	KW_RETURN = 108,
	KW_BREAK = 109,
	KW_CONTINUE = 110,
	KW_MAIN = 111,

	// 运算符
	OP_PLUS = 200,
	OP_MINUS = 201,
	OP_MUL = 202,
	OP_DIV = 203,
	OP_MOD = 204,
	OP_INC = 205,
	OP_DEC = 206,

	OP_ASSIGN = 210,
	OP_ADD_ASSIGN = 211,
	OP_SUB_ASSIGN = 212,
	OP_MUL_ASSIGN = 213,
	OP_DIV_ASSIGN = 214,
	OP_MOD_ASSIGN = 215,

	OP_LT = 220,
	OP_LE = 221,
	OP_GT = 222,
	OP_GE = 223,
	OP_EQ = 224,
	OP_NE = 225,

	OP_AND = 230,
	OP_OR = 231,
	OP_NOT = 232,

	OP_BIT_AND = 240,
	OP_BIT_OR = 241,
	OP_BIT_XOR = 242,
	OP_BIT_NOT = 243,
	OP_SHL = 244,
	OP_SHR = 245,

	OP_DOT = 250,
	OP_ARROW = 251,
	OP_QUESTION = 252,
	OP_COLON = 253,

	// 界限符/分隔符
	SEP_COMMA = 300,
	SEP_SEMICOLON = 301,
	SEP_LPAREN = 302,
	SEP_RPAREN = 303,
	SEP_LBRACKET = 304,
	SEP_RBRACKET = 305,
	SEP_LBRACE = 306,
	SEP_RBRACE = 307,

	// 特殊记号
	TK_EOF = 900,
	TK_ERROR = 901,
};

enum class ErrorCode {
	INVALID_CHAR,
	INVALID_IDENTIFIER,
	INVALID_NUMBER,
	UNCLOSED_CHAR,
	UNCLOSED_STRING,
	UNCLOSED_COMMENT,
	UNKNOWN_OPERATOR,
	INVALID_ESCAPE,
};

struct Token {
	TokenType type = TokenType::TK_ERROR;
	std::string lexeme;
	int line = 1;
	int column = 1;
	int attr = -1;
};

struct LexError {
	ErrorCode code = ErrorCode::INVALID_CHAR;
	std::string message;
	int line = 1;
	int column = 1;
	std::string fragment;
};

inline int tokenCode(TokenType type) { return static_cast<int>(type); }