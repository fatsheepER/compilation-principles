#include "parser.h"
#include "grammar.h"
#include "input_token.h"
#include "slr_table.h"
#include "terminal.h"
#include "token_reader.h"

#include <cstddef>
#include <optional>
#include <sstream>
#include <vector>

Parser::Parser(const Grammar &grammar, const SLRTable &table)
    : grammar_(grammar), table_(table) {
}

ParseResult Parser::parse(const ExpressionInput &expression) const {
	ParseResult result;
	result.expression_index = expression.index;

	// 如果词法分析已经存在 token 级错误
	// 不执行分析，直接返回错误
	if (!expression.errors.empty()) {
		const TokenReadError &error = expression.errors.front();

		ParseDiagnostic diagnostic;
		diagnostic.kind = ParseDiagnosticKind::TokenInputError;
		diagnostic.expression_index = expression.index;
		diagnostic.token_source_line = error.line;
		diagnostic.message = error.message;

		result.diagnostic = diagnostic;
		return result;
	}

	if (expression.tokens.empty()) {
		ParseDiagnostic diagnostic;
		diagnostic.kind = ParseDiagnosticKind::EmptyInput;
		diagnostic.expression_index = expression.index;
		diagnostic.message = "表达式 token 序列为空";

		result.diagnostic = diagnostic;
		return result;
	}

	std::vector<int> state_stack;
	std::vector<Symbol> symbol_stack;

	// 构造初始栈：     状态栈  符号栈
	//                  0       #
	state_stack.push_back(0);
	symbol_stack.push_back(makeTerminal(Terminal::End));

	std::size_t input_index = 0;
	std::size_t step_index = 1;

	while (true) {
		// 输入序列读完但未达到 acc
		if (input_index >= expression.tokens.size()) {
			ParseDiagnostic diagnostic;
			diagnostic.kind = ParseDiagnosticKind::InputExhausted;
			diagnostic.expression_index = expression.index;
			diagnostic.token_index = input_index;
			diagnostic.state = state_stack.back();
			diagnostic.message = "输入已经耗尽，但未达到 acc";

			result.steps.push_back(makeStep(step_index++, state_stack,
			                                symbol_stack, expression.tokens,
			                                input_index, "错误：输入已耗尽"));

			result.diagnostic = diagnostic;
			return result;
		}

		// 记录当前读取状态 context
		const int state = state_stack.back();
		const InputToken &current_token = expression.tokens[input_index];
		const Terminal current_terminal = current_token.terminal;

		// 根据当前状态获取下个 action
		const Action action = table_.action(state, current_terminal);

		// 移进
		if (action.kind == ActionKind::Shift) {
			std::ostringstream action_text;
			action_text << toString(action) << ": 移进 "
			            << toString(current_terminal);

			result.steps.push_back(makeStep(step_index, state_stack,
			                                symbol_stack, expression.tokens,
			                                input_index, action_text.str()));

			symbol_stack.push_back(makeTerminal(current_terminal));
			state_stack.push_back(action.next_state);

			++step_index;
			++input_index;

			continue;
		}

		// 归约
		if (action.kind == ActionKind::Reduce) {
			const Production &production =
			    grammar_.production(action.production_id);

			std::ostringstream action_text;
			action_text << toString(action) << ": "
			            << productionRuleToString(production);

			result.steps.push_back(makeStep(step_index, state_stack,
			                                symbol_stack, expression.tokens,
			                                input_index, action_text.str()));

			const std::size_t pop_count = production.rhs.size();

			// 栈底状态 0 和符号 # 不能被归约
			// 栈大小必须大于 pop_count
			if (symbol_stack.size() <= pop_count ||
			    state_stack.size() <= pop_count) {
				ParseDiagnostic diagnostic;
				diagnostic.kind = ParseDiagnosticKind::StackUnderflow;
				diagnostic.expression_index = expression.index;
				diagnostic.token_index = input_index;
				diagnostic.token_source_line = current_token.source_index;
				diagnostic.lexeme = current_token.lexeme;
				diagnostic.terminal = current_terminal;
				diagnostic.state = state;
				diagnostic.message = "归约时分析栈内符号或状态数量不足";

				result.diagnostic = diagnostic;
				return result;
			}

			// 归约出栈
			for (std::size_t i = 0; i < pop_count; ++i) {
				symbol_stack.pop_back();
				state_stack.pop_back();
			}

			const int goto_from_state = state_stack.back();
			const auto goto_state =
			    table_.goTo(goto_from_state, production.lhs);

			if (!goto_state.has_value()) {
				ParseDiagnostic diagnostic;
				diagnostic.kind = ParseDiagnosticKind::MissingGoto;
				diagnostic.expression_index = expression.index;
				diagnostic.token_index = input_index;
				diagnostic.token_source_line = current_token.source_index;
				diagnostic.lexeme = current_token.lexeme;
				diagnostic.terminal = current_terminal;
				diagnostic.state = goto_from_state;

				std::ostringstream message;
				message << "归约为 " << toString(production.lhs)
				        << " 后，GOTO[I" << goto_from_state << ", "
				        << toString(production.lhs) << "] 为空";
				diagnostic.message = message.str();

				result.diagnostic = diagnostic;
				return result;
			}

			symbol_stack.push_back(makeNonTerminal(production.lhs));
			state_stack.push_back(*goto_state);

			// Reduce 不消耗输入符号
			++step_index;

			continue;
		}

		// 接收
		if (action.kind == ActionKind::Accept) {
			result.steps.push_back(makeStep(step_index++, state_stack,
			                                symbol_stack, expression.tokens,
			                                input_index, "acc: 分析成功"));

			result.accepted = true;
			return result;
		}

		// ACTION 为空，出错
		{
			std::ostringstream action_text;
			action_text << "错误：ACTION[I" << state << ", "
			            << toString(current_terminal) << "] 为空";

			result.steps.push_back(makeStep(step_index++, state_stack,
			                                symbol_stack, expression.tokens,
			                                input_index, action_text.str()));

			ParseDiagnostic diagnostic;
			diagnostic.kind = ParseDiagnosticKind::EmptyAction;
			diagnostic.expression_index = expression.index;
			diagnostic.token_index = input_index;
			diagnostic.token_source_line = current_token.source_index;
			diagnostic.lexeme = current_token.lexeme;
			diagnostic.terminal = current_terminal;
			diagnostic.state = state;
			diagnostic.message = action_text.str();

			result.diagnostic = diagnostic;
			return result;
		}
	}
}

std::string Parser::stateStackToString(const std::vector<int> &state_stack) {
	std::ostringstream oss;

	for (std::size_t i = 0; i < state_stack.size(); ++i) {
		if (i > 0) {
			oss << " ";
		}

		oss << state_stack[i];
	}

	return oss.str();
}

std::string
Parser::symbolStackToString(const std::vector<Symbol> &symbol_stack) {
	std::ostringstream oss;

	for (std::size_t i = 0; i < symbol_stack.size(); ++i) {
		if (i > 0) {
			oss << " ";
		}

		oss << toString(symbol_stack[i]);
	}

	return oss.str();
}

std::string
Parser::combinedStackToString(const std::vector<Symbol> &symbol_stack,
                              const std::vector<int> &state_stack) {
	std::ostringstream oss;

	const std::size_t count = symbol_stack.size();

	for (std::size_t i = 0; i < count; ++i) {
		if (i > 0) {
			oss << " ";
		}

		oss << toString(symbol_stack[i]);

		if (i < state_stack.size()) {
			oss << " " << state_stack[i];
		}
	}

	return oss.str();
}

std::string
Parser::remainingInputToString(const std::vector<InputToken> &tokens,
                               std::size_t input_index) {
	std::ostringstream oss;

	for (std::size_t i = input_index; i < tokens.size(); ++i) {
		if (i > input_index) {
			oss << " ";
		}

		oss << toString(tokens[i].terminal);
	}

	return oss.str();
}

std::string Parser::productionRuleToString(const Production &production) {
	std::ostringstream oss;

	oss << toString(production.lhs) << " -> ";

	if (production.rhs.empty()) {
		oss << "ε";
		return oss.str();
	}

	for (std::size_t i = 0; i < production.rhs.size(); ++i) {
		if (i > 0) {
			oss << " ";
		}

		oss << toString(production.rhs[i]);
	}

	return oss.str();
}

ParseStep Parser::makeStep(std::size_t step_index,
                           const std::vector<int> &state_stack,
                           const std::vector<Symbol> &symbol_stack,
                           const std::vector<InputToken> &tokens,
                           std::size_t input_index, const std::string &action) {
	ParseStep step;
	step.step_index = step_index;
	step.state_stack = stateStackToString(state_stack);
	step.symbol_stack = symbolStackToString(symbol_stack);
	step.combined_stack = combinedStackToString(symbol_stack, state_stack);
	step.remaining_input = remainingInputToString(tokens, input_index);
	step.action = action;
	return step;
}
