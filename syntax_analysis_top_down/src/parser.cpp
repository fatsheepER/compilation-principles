#include "parser.h"
#include "grammar.h"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

PredictiveParser::PredictiveParser() = default;

ParseResult
PredictiveParser::parse(const std::vector<InputToken> &raw_input) const {
	ParseResult result;

	// 输入符号序列
	std::vector<InputToken> input = raw_input;
	// 如需，手动在序列结尾添加终结符 #
	if (input.empty() || input.back().terminal != Terminal::End) {
		input.push_back({
		    Terminal::End,
		    "#",
		    input.size(),
		});
	}

	// 分析栈
	std::vector<Symbol> stack;
	// 终止符号和开始符号入栈
	stack.push_back(Symbol::terminalSymbol(Terminal::End));
	stack.push_back(Symbol::nonTerminalSymbol(NonTerminal::E));

	std::size_t position = 0; // 输入符号序列序号
	int step_index = 1;       // 语法解析步骤序号

	while (!stack.empty()) {
		const Symbol top = stack.back();
		const std::string stack_before_pop = stackToString(stack);
		stack.pop_back();

		const InputToken &current = input[position];
		const Terminal lookahead = current.terminal;

		ParseStep step;
		step.index = step_index++;
		step.stack = stack_before_pop;
		step.remaining_input = remainingInputToString(input, position);

		// 如果 X 是终结符
		if (top.kind == SymbolKind::Terminal) {
			// 如果 X == a，则成功匹配，指针前进
			if (isSameTerminal(top, lookahead)) {
				std::ostringstream action;
				action << "匹配终结符 " << toString(lookahead);
				step.action = action.str();
				result.steps.push_back(step);

				// 如果 X == a == #，分析成功
				if (lookahead == Terminal::End) {
					result.accepted = true;
					return result;
				}

				++position;
				continue;
			}

			// 匹配失败，报错
			std::ostringstream message;
			message << "终结符匹配失败：栈顶为 ";
			message << toString(top);
			message << "，当前输入为 ";
			message << toString(lookahead);

			step.action = "出错";
			result.steps.push_back(step);

			result.accepted = false;
			result.error_token_index = current.source_index;
			result.error_message = message.str();
			return result;
		}

		// 如果 X 是非终结符

		// 查询预测分析表
		const Production *production =
		    grammer_.lookup(top.non_terminal, lookahead);

		// 如果表项为空，则报语法错误
		if (production == nullptr) {
			std::ostringstream message;
			message << "预测分析表表项为空：M[";
			message << toString(top.non_terminal) << ", ";
			message << toString(lookahead) << "] 无产生式";

			step.action = "出错";
			result.steps.push_back(step);

			result.accepted = false;
			result.error_token_index = current.source_index;
			result.error_message = message.str();
			return result;
		}

		// 如果表项不为空
		step.action = "使用产生式" + toString(*production);
		result.steps.push_back(step);

		// rhs 符号逆序压栈 (epsilon 本就为空)
		for (auto it = production->rhs.rbegin(); it != production->rhs.rend();
		     ++it) {
			stack.push_back(*it);
		}
	}

	// 非正常退出循环
	result.accepted = false;

	if (position < input.size()) {
		result.error_token_index = input[position].source_index;
		result.error_message = "分析栈已空，但输入尚未结束";
	}
	else {
		result.error_token_index =
		    input.empty() ? 0 : input.back().source_index;
		result.error_message = "分析栈异常为空";
	}
	return result;
}

std::string PredictiveParser::stackToString(const std::vector<Symbol> &stack) {
	if (stack.empty()) {
		return "空";
	}

	std::ostringstream oss;

	for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
		if (it != stack.rbegin()) {
			oss << ' ';
		}
		oss << toString(*it);
	}

	return oss.str();
}

std::string
PredictiveParser::remainingInputToString(const std::vector<InputToken> &input,
                                         std::size_t position) {
	if (position >= input.size()) {
		return "空";
	}

	std::ostringstream oss;

	for (std::size_t i = position; i < input.size(); ++i) {
		if (i > position) {
			oss << ' ';
		}

		oss << toString(input[i].terminal);
	}

	return oss.str();
}

bool PredictiveParser::isSameTerminal(const Symbol &symbol, Terminal terminal) {
	return symbol.kind == SymbolKind::Terminal && symbol.terminal == terminal;
}
