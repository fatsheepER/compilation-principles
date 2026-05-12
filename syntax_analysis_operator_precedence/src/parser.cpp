#include "parser.h"
#include "grammar.h"
#include "input_token.h"
#include "precedence_table.h"
#include "terminal.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

StackSymbol StackSymbol::terminalSymbol(Terminal terminal) {
	StackSymbol symbol;
	symbol.kind = StackSymbolKind::Terminal;
	symbol.terminal = terminal;
	return symbol;
}

StackSymbol StackSymbol::nonTerminalPlaceholder() {
	StackSymbol symbol;
	symbol.kind = StackSymbolKind::NonTerminalPlaceholder;
	return symbol;
}

OperatorPrecedenceParseResult
OperatorPrecedenceParser::parse(const std::vector<InputToken> &raw_input,
                                const PrecedenceTable &precedence_table) const {
	OperatorPrecedenceParseResult result;

	std::vector<InputToken> input = raw_input;
	if (input.empty() || input.back().terminal != Terminal::End) {
		input.push_back({
		    Terminal::End,
		    "#",
		    input.size(),
		});
	}

	std::vector<StackSymbol> stack;
	stack.push_back(StackSymbol::terminalSymbol(Terminal::End));

	std::size_t position = 0; // for remaining_input
	int step_index = 1;

	while (position < input.size()) {
		const InputToken &current = input[position];

		OperatorPrecedenceStep step;

		// save step contexts
		step.index = step_index++;
		step.stack = stackToString(stack);
		step.remaining_input = remainingInputToString(input, position);

		const TerminalPosition top_terminal = topMostTerminal(stack);
		if (!top_terminal.found) {
			step.action = "出错";
			result.steps.push_back(step);

			result.accepted = false;
			result.error_token_index = current.source_index;
			result.error_message = "分析栈中找不到终结符";
			return result;
		}

		// check if is done
		if (current.terminal == Terminal::End && isAcceptStack(stack)) {
			step.relation = "# = #";
			step.action = "分析成功";
			result.steps.push_back(step);

			result.accepted = true;
			return result;
		}

		const auto relation =
		    precedence_table.lookup(top_terminal.terminal, current.terminal);

		if (!relation.has_value()) {
			std::ostringstream oss;
			oss << "终结符 " << ::toString(top_terminal.terminal)
			    << " 与当前输入符号 " << ::toString(current.terminal)
			    << " 之间不存在算符优先关系";

			step.relation = ::toString(top_terminal.terminal) + " ? " +
			                ::toString(current.terminal);
			step.action = "出错";
			result.steps.push_back(step);

			result.accepted = false;
			result.error_token_index = current.source_index;
			result.error_message = oss.str();
			return result;
		}

		step.relation = ::toString(top_terminal.terminal) + " " +
		                ::toString(relation.value()) + " " +
		                ::toString(current.terminal);

		// stack_top_terminal <= current_input_terminal
		if (relation.value() == PrecedenceRelation::Less ||
		    relation.value() == PrecedenceRelation::Equal) {
			stack.push_back(StackSymbol::terminalSymbol(current.terminal));

			step.action = "移进 " + ::toString(current.terminal);
			result.steps.push_back(step);

			++position;
			continue;
		}

		// reduce leftmost prime phrase
		const HandleSearchResult handle = findHandle(stack, precedence_table);

		if (!handle.success) {
			step.action = "出错";
			result.steps.push_back(step);

			result.accepted = false;
			result.error_token_index = current.source_index;
			result.error_message = handle.error_message;
			return result;
		}

		std::vector<StackSymbol> phrase(stack.begin() + handle.begin,
		                                stack.end());

		if (!canReduceToN(phrase)) {
			step.action = "出错";
			result.steps.push_back(step);

			result.accepted = false;
			result.error_token_index = current.source_index;
			result.error_message =
			    "短语 " + phraseToString(phrase) + " 不能匹配任何规约模式";
			return result;
		}

		stack.erase(stack.begin() + handle.begin, stack.end());
		stack.push_back(StackSymbol::nonTerminalPlaceholder());

		step.action = "归约 " + phraseToString(phrase) + " -> N";
		result.steps.push_back(step);
	}

	result.accepted = false;
	result.error_token_index = input.empty() ? 0 : input.back().source_index;
	result.error_message = "输入已经结束，但分析过程未达到成功状态";
	return result;
}

OperatorPrecedenceParser::TerminalPosition
OperatorPrecedenceParser::topMostTerminal(
    const std::vector<StackSymbol> &stack) {
	for (std::size_t i = stack.size(); i > 0; --i) {
		const std::size_t index = i - 1;

		if (stack[index].kind == StackSymbolKind::Terminal) {
			return {
			    true,
			    index,
			    stack[index].terminal,
			};
		}
	}

	return {}; // found = false
}

OperatorPrecedenceParser::TerminalPosition
OperatorPrecedenceParser::previousTerminalBefore(
    const std::vector<StackSymbol> &stack, std::size_t before_index) {
	for (std::size_t i = before_index; i > 0; --i) {
		const std::size_t index = i - 1;

		if (stack[index].kind == StackSymbolKind::Terminal) {
			return {
			    true,
			    index,
			    stack[index].terminal,
			};
		}
	}

	return {}; // found = false
}

OperatorPrecedenceParser::HandleSearchResult
OperatorPrecedenceParser::findHandle(const std::vector<StackSymbol> &stack,
                                     const PrecedenceTable &precedence_table) {
	const TerminalPosition first_terminal = topMostTerminal(stack);

	if (!first_terminal.found) {
		return {
		    false,
		    0,
		    "归约时分析栈找不到终结符",
		};
	}

	std::size_t current_terminal_index = first_terminal.index;

	// 向前查找第一个更小的终结符，确定边界
	while (true) {
		const TerminalPosition previous =
		    previousTerminalBefore(stack, current_terminal_index);

		if (!previous.found) {
			return {
			    false,
			    0,
			    "归约时找不到最左素短语左边界",
			};
		}

		const auto relation = precedence_table.lookup(
		    previous.terminal, stack[current_terminal_index].terminal);

		if (!relation.has_value()) {
			return {
			    false,
			    0,
			    "归约时，边界查找遇到不存在的优先关系: " +
			        ::toString(previous.terminal) + " ? " +
			        ::toString(stack[current_terminal_index].terminal),
			};
		}

		if (relation.value() == PrecedenceRelation::Less) {
			return {
			    true,
			    previous.index + 1,
			    "",
			};
		}

		current_terminal_index = previous.index;
	}
}

bool OperatorPrecedenceParser::canReduceToN(
    const std::vector<StackSymbol> &phrase) {
	// i -> N
	if (phrase.size() == 1 && phrase[0].kind == StackSymbolKind::Terminal &&
	    phrase[0].terminal == Terminal::Id) {
		return true;
	}

	// N + N -> N
	if (phrase.size() == 3 &&
	    phrase[0].kind == StackSymbolKind::NonTerminalPlaceholder &&
	    phrase[1].kind == StackSymbolKind::Terminal &&
	    phrase[1].terminal == Terminal::Plus &&
	    phrase[2].kind == StackSymbolKind::NonTerminalPlaceholder) {
		return true;
	}

	// N * N -> N
	if (phrase.size() == 3 &&
	    phrase[0].kind == StackSymbolKind::NonTerminalPlaceholder &&
	    phrase[1].kind == StackSymbolKind::Terminal &&
	    phrase[1].terminal == Terminal::Mul &&
	    phrase[2].kind == StackSymbolKind::NonTerminalPlaceholder) {
		return true;
	}

	// ( N ) -> N
	if (phrase.size() == 3 && phrase[0].kind == StackSymbolKind::Terminal &&
	    phrase[0].terminal == Terminal::LParen &&
	    phrase[1].kind == StackSymbolKind::NonTerminalPlaceholder &&
	    phrase[2].kind == StackSymbolKind::Terminal &&
	    phrase[2].terminal == Terminal::RParen) {
		return true;
	}

	return false;
}

std::string
OperatorPrecedenceParser::stackToString(const std::vector<StackSymbol> &stack) {
	if (stack.empty()) {
		return "空";
	}

	std::ostringstream oss;

	for (const auto &symbol : stack) { oss << toString(symbol); }

	return oss.str();
}

std::string OperatorPrecedenceParser::remainingInputToString(
    const std::vector<InputToken> &input, std::size_t position) {
	if (position >= input.size()) {
		return "空";
	}

	std::ostringstream oss;

	for (std::size_t i = position; i < input.size(); ++i) {
		if (!input[i].lexeme.empty()) {
			oss << input[i].lexeme;
		}
		else {
			oss << ::toString(input[i].terminal);
		}
	}

	return oss.str();
}

std::string OperatorPrecedenceParser::phraseToString(
    const std::vector<StackSymbol> &phrase) {
	if (phrase.empty()) {
		return "空";
	}

	std::ostringstream oss;

	for (const auto &symbol : phrase) { oss << toString(symbol); }

	return oss.str();
}

std::string OperatorPrecedenceParser::toString(const StackSymbol &symbol) {
	if (symbol.kind == StackSymbolKind::Terminal) {
		return ::toString(symbol.terminal);
	}
	return "N";
}

bool OperatorPrecedenceParser::isAcceptStack(
    const std::vector<StackSymbol> &stack) {
	return stack.size() == 2 && stack[0].kind == StackSymbolKind::Terminal &&
	       stack[0].terminal == Terminal::End &&
	       stack[1].kind == StackSymbolKind::NonTerminalPlaceholder;
}