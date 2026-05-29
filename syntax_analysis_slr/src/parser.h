#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "grammar.h"
#include "input_token.h"
#include "slr_table.h"
#include "terminal.h"
#include "token_reader.h"

// Parser 发现的错误类型
enum class ParseDiagnosticKind {
	TokenInputError, // 在词法分析阶段发现 token 级错误
	EmptyInput,      // 表达式 token 序列为空
	InputExhausted,  // 没达到 acc，但输入 token 已空
	EmptyAction,     // ACTION 表项为空
	MissingGoto,     // Reduce 后找不到 GOTO 表项
	StackUnderflow,  // Reduce 时栈内符号数量不足
};

// 单条 Parser 错误信息
struct ParseDiagnostic {
	ParseDiagnosticKind kind = ParseDiagnosticKind::EmptyAction;

	std::size_t expression_index = 0;
	std::size_t token_index = 0;       //
	std::size_t token_source_line = 0; // 在词法分析 result.txt 中的行号

	std::string lexeme;
	Terminal terminal = Terminal::End;

	int state = -1;
	std::string message;
};

// 单步 Parser 分析记录
struct ParseStep {
	std::size_t step_index = 0;

	std::string state_stack;
	std::string symbol_stack;
	std::string combined_stack;
	std::string remaining_input;

	std::string action;
};

// 单条表达式的分析结果
struct ParseResult {
	std::size_t expression_index = 0;
	bool accepted = false;

	std::vector<ParseStep> steps;
	std::optional<ParseDiagnostic> diagnostic;
};

// SLR(1) 语法分析器
class Parser {
  public:
	Parser(const Grammar &grammar, const SLRTable &table);

	// 对单条归一化并追加 # 的表达式进行语法分析
	ParseResult parse(const ExpressionInput &expression) const;

  private:
	const Grammar &grammar_;
	const SLRTable &table_;

	static std::string stateStackToString(const std::vector<int> &stack);

	static std::string symbolStackToString(const std::vector<Symbol> &stack);

	static std::string
	combinedStackToString(const std::vector<Symbol> &symbol_stack,
	                      const std::vector<int> &state_stack);

	static std::string
	remainingInputToString(const std::vector<InputToken> &tokens,
	                       std::size_t input_index);

	static std::string productionRuleToString(const Production &production);

	static ParseStep makeStep(std::size_t step_index,
	                          const std::vector<int> &state_stack,
	                          const std::vector<Symbol> &symbol_stack,
	                          const std::vector<InputToken> &tokens,
	                          std::size_t input_index,
	                          const std::string &action);
};