#include "tables.h"
#include "token.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

// 关键字 - 种别
const std::unordered_map<std::string, TokenType> kKeywords = {
    {"int", TokenType::KW_INT},           {"char", TokenType::KW_CHAR},
    {"void", TokenType::KW_VOID},         {"if", TokenType::KW_IF},
    {"else", TokenType::KW_ELSE},         {"while", TokenType::KW_WHILE},
    {"for", TokenType::KW_FOR},           {"do", TokenType::KW_DO},
    {"return", TokenType::KW_RETURN},     {"break", TokenType::KW_BREAK},
    {"continue", TokenType::KW_CONTINUE}, {"main", TokenType::KW_MAIN}};

// 运算符 - 种别
const std::unordered_map<std::string, TokenType> kOperators = {
    {"+", TokenType::OP_PLUS},        {"-", TokenType::OP_MINUS},
    {"*", TokenType::OP_MUL},         {"/", TokenType::OP_DIV},
    {"%", TokenType::OP_MOD},         {"++", TokenType::OP_INC},
    {"--", TokenType::OP_DEC},

    {"=", TokenType::OP_ASSIGN},      {"+=", TokenType::OP_ADD_ASSIGN},
    {"-=", TokenType::OP_SUB_ASSIGN}, {"*=", TokenType::OP_MUL_ASSIGN},
    {"/=", TokenType::OP_DIV_ASSIGN}, {"%=", TokenType::OP_MOD_ASSIGN},

    {"<", TokenType::OP_LT},          {"<=", TokenType::OP_LE},
    {">", TokenType::OP_GT},          {">=", TokenType::OP_GE},
    {"==", TokenType::OP_EQ},         {"!=", TokenType::OP_NE},

    {"&&", TokenType::OP_AND},        {"||", TokenType::OP_OR},
    {"!", TokenType::OP_NOT},

    {"&", TokenType::OP_BIT_AND},     {"|", TokenType::OP_BIT_OR},
    {"^", TokenType::OP_BIT_XOR},     {"~", TokenType::OP_BIT_NOT},
    {"<<", TokenType::OP_SHL},        {">>", TokenType::OP_SHR},

    {".", TokenType::OP_DOT},         {"->", TokenType::OP_ARROW},
    {"?", TokenType::OP_QUESTION},    {":", TokenType::OP_COLON}};

// 界限符 - 种别
const std::unordered_map<std::string, TokenType> kDelimiters = {
    {",", TokenType::SEP_COMMA},    {";", TokenType::SEP_SEMICOLON},
    {"(", TokenType::SEP_LPAREN},   {")", TokenType::SEP_RPAREN},
    {"[", TokenType::SEP_LBRACKET}, {"]", TokenType::SEP_RBRACKET},
    {"{", TokenType::SEP_LBRACE},   {"}", TokenType::SEP_RBRACE}};

// 可作为操作符起始的字符
const std::unordered_set<char> kOperatorStartChars = {
    '+', '-', '*', '/', '%', '=', '!', '<',
    '>', '&', '|', '^', '~', '.', '?', ':'};

// 界限符
const std::unordered_set<char> kDelimiterChars = {',', ';', '(', ')',
                                                  '[', ']', '{', '}'};

// 合法的转义字符
const std::unordered_set<char> kEscapeChars = {'n',  't', 'r', '\\',
                                               '\'', '"', '0'};

} // namespace

// IdentifierTable
int IdentifierTable::find(const std::string &name) const {
	for (const auto &entry : entries_) {
		if (entry.name == name) { return entry.index; }
	}
	return -1;
}

int IdentifierTable::addOrGet(const std::string &name) {
	int idx = find(name);
	if (idx != -1) { return idx; }

	int newIndex = static_cast<int>(entries_.size());
	entries_.push_back({newIndex, name});
	return newIndex;
}

void IdentifierTable::clear() { entries_.clear(); }

const std::vector<IdentifierEntry> &IdentifierTable::entries() const {
	return entries_;
}

// ConstantTable
int ConstantTable::find(const std::string &literal, TokenType type) const {
	for (const auto &entry : entries_) {
		if (entry.literal == literal && entry.type == type) {
			return entry.index;
		}
	}
	return -1;
}

int ConstantTable::addOrGet(const std::string &literal, TokenType type) {
	int idx = find(literal, type);
	if (idx != -1) { return idx; }

	int newIndex = static_cast<int>(entries_.size());
	entries_.push_back({newIndex, literal, type});
	return newIndex;
}

void ConstantTable::clear() { entries_.clear(); }

const std::vector<ConstantEntry> &ConstantTable::entries() const {
	return entries_;
}

// Utils

bool isKeyword(const std::string &lexeme) {
	return kKeywords.find(lexeme) != kKeywords.end();
}

TokenType keywordTypeOf(const std::string &lexeme) {
	auto it = kKeywords.find(lexeme);
	if (it != kKeywords.end()) { return it->second; }
	return TokenType::TK_ERROR;
}

bool matchOperator(const std::string &lexeme, TokenType &outType) {
	auto it = kOperators.find(lexeme);
	if (it == kOperators.end()) { return false; }
	outType = it->second;
	return true;
}

bool matchDelimiter(const std::string &lexeme, TokenType &outType) {
	auto it = kDelimiters.find(lexeme);
	if (it == kDelimiters.end()) { return false; }
	outType = it->second;
	return true;
}

bool isOperatorStart(char ch) {
	return kOperatorStartChars.find(ch) != kOperatorStartChars.end();
}

bool isDelimiterStart(char ch) {
	return kDelimiterChars.find(ch) != kDelimiterChars.end();
}

bool isEscapeChar(char ch) {
	return kEscapeChars.find(ch) != kEscapeChars.end();
}