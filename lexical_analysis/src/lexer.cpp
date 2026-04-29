#include "lexer.h"
#include "tables.h"
#include "token.h"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

void Lexer::tokenize() {
	pos_ = 0;
	line_ = 1;
	column_ = 1;

	tokens_.clear();
	errors_.clear();
	identifierTable_.clear();
	constantTable_.clear();

	while (true) {
		Token tk = nextToken();
		tokens_.push_back(tk);
		if (tk.type == TokenType::TK_EOF) { break; }
	}
}

Token Lexer::nextToken() {
	skipWhitespace();

	if (isAtEnd()) {
		return makeToken(TokenType::TK_EOF, "EOF", line_, column_);
	}

	// 取字符
	char ch = peek();

	// 进入标识符 / 关键字识别流程
	if (isIdentifierStart(ch)) { return scanIdentifierOrKeyword(); }

	// 进入整数常量识别流程
	if (std::isdigit(static_cast<unsigned char>(ch))) { return scanNumber(); }

	// 进入字符常量识别流程
	if (ch == '\'') { return scanCharLiteral(); }

	// 进入字符串常量识别流程
	if (ch == '"') { return scanStringLiteral(); }

	// 进入操作符 / 界限符 / 注释识别流程
	if (isOperatorStart(ch) || isDelimiterStart(ch)) {
		return scanOperatorOrDelimiter();
	}

	int startLine = line_;
	int startColumn = column_;
	std::string bad(1, advance());
	reportError(ErrorCode::INVALID_CHAR, "invalid char", bad, startLine,
	            startColumn);
	return makeErrorToken(bad, startLine, startColumn);
}

const std::vector<Token> &Lexer::getTokens() const { return tokens_; }

const std::vector<LexError> &Lexer::getErrors() const { return errors_; }

const IdentifierTable &Lexer::getIdentifierTable() const {
	return identifierTable_;
}

const ConstantTable &Lexer::getConstantTable() const { return constantTable_; }

bool Lexer::isAtEnd() const { return pos_ >= source_.size(); }

char Lexer::peek() const {
	if (isAtEnd()) { return '\0'; }
	return source_[pos_];
}

char Lexer::peekNext() const {
	if (pos_ + 1 >= source_.size()) { return '\0'; }
	return source_[pos_ + 1];
}

char Lexer::advance() {
	if (isAtEnd()) { return '\0'; }

	char ch = source_[pos_++];
	if (ch == '\n') {
		++line_;
		column_ = 1;
	}
	else { ++column_; }
	return ch;
}

bool Lexer::isIdentifierStart(char ch) const {
	return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool Lexer::isIdentifierPart(char ch) const {
	return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

bool Lexer::isOctDigit(char ch) const { return ch >= '0' && ch <= '7'; }

bool Lexer::isHexDigit(char ch) const {
	return std::isxdigit(static_cast<unsigned char>(ch));
}

void Lexer::skipWhitespace() {
	while (!isAtEnd()) {
		char ch = peek();
		if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') { advance(); }
		else { break; }
	}
}

bool Lexer::skipComment() {
	// 首字符应当为斜杠
	if (peek() != '/') { return false; }

	// 单行注释
	if (peekNext() == '/') {
		advance(); // #1 '/'
		advance(); // #2 '/'

		while (!isAtEnd() && peek() != '\n') { advance(); }
		return true;
	}

	// 块注释
	if (peekNext() == '*') {
		int startLine = line_;
		int startColumn = column_;

		advance(); // '/'
		advance(); // '*'

		while (!isAtEnd()) {
			char ch = advance();

			// 跳出逻辑
			if (ch == '*' && peek() == '/') {
				advance(); // '/'
				return true;
			}
		}

		// 注释未闭合
		reportError(ErrorCode::UNCLOSED_COMMENT, "unclosed comment", "/*",
		            startLine, startColumn);
		return true;
	}

	// 非注释
	return false;
}

void Lexer::skipUntilSeparatorOrWhitespace() {
	while (!isAtEnd()) {
		char ch = peek();
		if (std::isspace(static_cast<unsigned char>(ch)) ||
		    isDelimiterStart(ch)) {
			return;
		}
		advance();
	}
}

void Lexer::skipUntilTokenBoundary() {
	while (!isAtEnd()) {
		char ch = peek();
		if (std::isspace(static_cast<unsigned char>(ch)) ||
		    isDelimiterStart(ch) || isOperatorStart(ch)) {
			return;
		}
		advance();
	}
}

void Lexer::skipUntilCharLiteralEnd() {
	while (!isAtEnd()) {
		char ch = peek();
		if (ch == '\n') { return; }

		ch = advance();
		if (ch == '\\' && !isAtEnd() && peek() != '\n') {
			advance();
			continue;
		}

		if (ch == '\'') { return; }
	}
}

void Lexer::skipUntilStringLiteralEnd() {
	while (!isAtEnd()) {
		char ch = peek();
		if (ch == '\n') { return; }

		ch = advance();
		if (ch == '\\' && !isAtEnd() && peek() != '\n') {
			advance();
			continue;
		}

		if (ch == '"') { return; }
	}
}

Token Lexer::scanIdentifierOrKeyword() {
	int startLine = line_;
	int startColumn = column_;
	std::string lexeme;

	while (!isAtEnd() && isIdentifierPart(peek())) {
		lexeme.push_back(advance());
	}

	// Token 为关键字
	if (isKeyword(lexeme)) {
		return makeToken(keywordTypeOf(lexeme), lexeme, startLine, startColumn);
	}

	// Token 为标识符
	int attr = identifierTable_.addOrGet(lexeme);
	return makeToken(TokenType::TK_IDENTIFIER, lexeme, startLine, startColumn,
	                 attr);
}

Token Lexer::scanNumber() {
	int startLine = line_;
	int startColumn = column_;
	std::string lexeme;

	// 首数字为 0
	if (peek() == '0') {
		lexeme.push_back(advance());

		// HEX
		if (peek() == 'x' || peek() == 'X') {
			lexeme.push_back(advance());

			if (!isHexDigit(peek())) {
				reportError(ErrorCode::INVALID_NUMBER,
				            "invalid hexadecimal literal", lexeme, startLine,
				            startColumn);
				skipUntilTokenBoundary();
				return makeErrorToken(lexeme, startLine, startColumn);
			}

			while (!isAtEnd() && isHexDigit(peek())) {
				lexeme.push_back(advance());
			}

			// 数字后缀出现意外字母 / 下划线
			if (isIdentifierStart(peek())) {
				reportError(ErrorCode::INVALID_NUMBER,
				            "invalid suffix after number", lexeme, startLine,
				            startColumn);
				skipUntilTokenBoundary();
				return makeErrorToken(lexeme, startLine, startColumn);
			}

			// 出现其他符号：数字后面的运算符，不用管

			// 储存整数常量信息
			int attr =
			    constantTable_.addOrGet(lexeme, TokenType::TK_INT_LITERAL);
			return makeToken(TokenType::TK_INT_LITERAL, lexeme, startLine,
			                 startColumn, attr);
		} // HEX

		// OCT
		if (isOctDigit(peek())) {
			while (!isAtEnd() && isOctDigit(peek())) {
				lexeme.push_back(advance());
			}

			// 数字后缀出现不属于 OCT 的数字
			if (std::isdigit(static_cast<unsigned char>(peek()))) {
				reportError(ErrorCode::INVALID_NUMBER, "invalid octal literal",
				            lexeme, startLine, startColumn);
				skipUntilTokenBoundary();
				return makeErrorToken(lexeme, startLine, startColumn);
			}

			// 数字后缀出现字母 / 下划线
			if (isIdentifierStart(peek())) {
				reportError(ErrorCode::INVALID_NUMBER,
				            "invalid suffix after number", lexeme, startLine,
				            startColumn);
				skipUntilTokenBoundary();
				return makeErrorToken(lexeme, startLine, startColumn);
			}

			// 储存整数常量信息
			int attr =
			    constantTable_.addOrGet(lexeme, TokenType::TK_INT_LITERAL);
			return makeToken(TokenType::TK_INT_LITERAL, lexeme, startLine,
			                 startColumn, attr);
		} // OCT

		// 后面跟着非 OCT 数字：非法 OCT
		if (std::isdigit(static_cast<unsigned char>(peek()))) {
			reportError(ErrorCode::INVALID_NUMBER, "invalid octal literal",
			            lexeme, startLine, startColumn);
			skipUntilTokenBoundary();
			return makeErrorToken(lexeme, startLine, startColumn);
		}

		// 单独的 0
		int attr = constantTable_.addOrGet(lexeme, TokenType::TK_INT_LITERAL);
		return makeToken(TokenType::TK_INT_LITERAL, lexeme, startLine,
		                 startColumn, attr);
	} // 首数字为 0

	// DEC
	while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
		lexeme.push_back(advance());
	}

	if (isIdentifierStart(peek())) {
		reportError(ErrorCode::INVALID_NUMBER, "invalid suffix after number",
		            lexeme, startLine, startColumn);
		skipUntilTokenBoundary();
		return makeErrorToken(lexeme, startLine, startColumn);
	}

	// 储存整数常量信息
	int attr = constantTable_.addOrGet(lexeme, TokenType::TK_INT_LITERAL);
	return makeToken(TokenType::TK_INT_LITERAL, lexeme, startLine, startColumn,
	                 attr);
}

Token Lexer::scanCharLiteral() {
	int startLine = line_;
	int startColumn = column_;
	std::string lexeme;

	lexeme.push_back(advance()); // starting '\''

	// 字符无内容 单引号未闭合
	if (isAtEnd() || peek() == '\n') {
		reportError(ErrorCode::UNCLOSED_CHAR, "unclosed char literal", lexeme,
		            startLine, startColumn);
		return makeErrorToken(lexeme, startLine, startColumn);
	}

	// 判断是否为合法转义字符
	if (peek() == '\\') {
		lexeme.push_back(advance()); // '\\'

		// 非法转义字符
		if (!isEscapeChar(peek())) {
			reportError(ErrorCode::INVALID_ESCAPE,
			            "invalid escape sequence in char literal", lexeme,
			            startLine, startColumn);
			skipUntilCharLiteralEnd();
			return makeErrorToken(lexeme, startLine, startColumn);
		}

		// 合法转义字符
		lexeme.push_back(advance());
	}
	else { // 普通字符内容
		lexeme.push_back(advance());
	}

	// 字符内容后单引号未闭合
	if (isAtEnd() || peek() != '\'') {
		reportError(ErrorCode::UNCLOSED_CHAR, "unclosed char literal", lexeme,
		            startLine, startColumn);
		skipUntilCharLiteralEnd();
		return makeErrorToken(lexeme, startLine, startColumn);
	}

	lexeme.push_back(advance()); // closing '\''

	// 储存字符常量信息
	int attr = constantTable_.addOrGet(lexeme, TokenType::TK_CHAR_LITERAL);
	return makeToken(TokenType::TK_CHAR_LITERAL, lexeme, startLine, startColumn,
	                 attr);
}

Token Lexer::scanStringLiteral() {
	int startLine = line_;
	int startColumn = column_;
	std::string lexeme;

	lexeme.push_back(advance()); // starting '"'

	while (!isAtEnd()) {
		if (peek() == '"') { // closing '"'
			lexeme.push_back(advance());
			int attr =
			    constantTable_.addOrGet(lexeme, TokenType::TK_STRING_LITERAL);
			return makeToken(TokenType::TK_STRING_LITERAL, lexeme, startLine,
			                 startColumn, attr);
		}

		// 遇到换行 字符串未闭合
		if (peek() == '\n') {
			reportError(ErrorCode::UNCLOSED_STRING, "unclosed string literal",
			            lexeme, startLine, startColumn);
			return makeErrorToken(lexeme, startLine, startColumn);
		}

		// 遇到反斜杠 判断是否为转义字符
		if (peek() == '\\') {
			lexeme.push_back(advance()); // '\\'

			if (!isEscapeChar(peek())) {
				reportError(ErrorCode::INVALID_ESCAPE,
				            "invalid escape sequence in string literal", lexeme,
				            startLine, startColumn);
				skipUntilStringLiteralEnd();
				return makeErrorToken(lexeme, startLine, startColumn);
			}

			lexeme.push_back(advance()); // 转义字符
			continue;
		}

		lexeme.push_back(advance()); // 读入一个字符
	}

	// EOF 字符串未闭合
	reportError(ErrorCode::UNCLOSED_STRING, "unclosed string literal", lexeme,
	            startLine, startColumn);
	return makeErrorToken(lexeme, startLine, startColumn);
}

Token Lexer::scanOperatorOrDelimiter() {
	int startLine = line_;
	int startColumn = column_;

	// 优先处理注释
	if (peek() == '/' && skipComment()) { return nextToken(); }

	TokenType type = TokenType::TK_ERROR;

	// 操作符最长匹配 双字符优先
	if (!isAtEnd()) {
		std::string two;
		two.push_back(peek());
		if (peekNext() != '\0') {
			two.push_back(peekNext());
			if (matchOperator(two, type)) {
				advance();
				advance();
				return makeToken(type, two, startLine, startColumn);
			}
		}
	}

	std::string one(1, peek());

	// 单字符界限符
	if (matchDelimiter(one, type)) {
		advance();
		return makeToken(type, one, startLine, startColumn);
	}

	// 单字符运算符
	if (matchOperator(one, type)) {
		advance();
		return makeToken(type, one, startLine, startColumn);
	}

	// 未知符号
	std::string bad(1, advance());
	reportError(ErrorCode::UNKNOWN_OPERATOR, "unknown operator or delimiter",
	            bad, startLine, startColumn);
	return makeErrorToken(bad, startLine, startColumn);
}

Token Lexer::makeToken(TokenType type, const std::string &lexeme, int startLine,
                       int startColumn, int attr) const {
	Token tk;
	tk.type = type;
	tk.lexeme = lexeme;
	tk.line = startLine;
	tk.column = startColumn;
	tk.attr = attr;
	return tk;
}

Token Lexer::makeErrorToken(const std::string &lexeme, int startLine,
                            int startColumn) const {
	return makeToken(TokenType::TK_ERROR, lexeme, startLine, startColumn);
}

void Lexer::reportError(ErrorCode code, const std::string &message,
                        const std::string &fragment, int errorLine,
                        int errorColumn) {
	errors_.push_back({code, message, errorLine, errorColumn, fragment});
}
