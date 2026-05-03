#include "evaluator.h"

#include <cctype>
#include <cstddef>
#include <exception>

// helpers
namespace {
bool isIntegerLexeme(const std::string &text) {
	if (text.empty()) {
		return false;
	}

	for (char ch : text) {
		if (!std::isdigit(static_cast<unsigned char>(ch))) {
			return false;
		}
	}

	return true;
}

struct EvaluatorState {
	const std::vector<InputToken> &tokens;
	std::size_t position = 0;

	const InputToken &current() const { return tokens[position]; }

	EvaluationResult unavailable(const std::string &message) const {
		return {
		    false,
		    0,
		    message,
		};
	}

	EvaluationResult value(long long number) const {
		return {
		    true,
		    number,
		    "",
		};
	}

	// 处理加法 E -> T E' || E' -> + T E' | e
	EvaluationResult parseExpression() {
		EvaluationResult left = parseTerm();
		if (!left.available) {
			return left;
		}

		// T + T + T + ... + F
		while (current().terminal == Terminal::Plus) {
			++position;
			EvaluationResult right = parseTerm();
			if (!right.available) {
				return right;
			}

			left.value += right.value;
		}

		return left;
	}

	// 处理乘法 T -> F T' || T' -> * F T' | e
	EvaluationResult parseTerm() {
		EvaluationResult left = parseFactor();
		if (!left.available) {
			return left;
		}

		// T * T * T * ... * F
		while (current().terminal == Terminal::Mul) {
			++position;
			EvaluationResult right = parseFactor();
			if (!right.available) {
				return right;
			}

			left.value *= right.value;
		}

		return left;
	}

	// 处理因子 F -> ( E ) | number
	EvaluationResult parseFactor() {
		// ( E )
		if (current().terminal == Terminal::LParen) {
			++position;

			EvaluationResult inner = parseExpression();
			if (!inner.available) {
				return inner;
			}

			if (current().terminal != Terminal::RParen) {
				return unavailable("求值失败: 缺少右括号");
			}

			++position;
			return inner;
		}

		// number
		if (current().terminal == Terminal::Id) {
			const std::string lexeme = current().lexeme;
			++position;

			if (lexeme == "i") {
				return unavailable("无法计算，表达式含有标识符 i");
			}

			if (!isIntegerLexeme(lexeme)) {
				return unavailable("无法计算，操作符不是无符号整数");
			}

			try {
				// success
				return value(std::stoll(lexeme));
			} catch (const std::exception &) {
				return unavailable("无法计算，无法转换成 long long");
			}
		}

		return unavailable("求值失败: factor 处不是操作数或左括号");
	}
};

} // namespace

EvaluationResult
ExpressionEvaluator::evaluate(const std::vector<InputToken> &tokens) const {
	if (tokens.empty()) {
		return {
		    false,
		    0,
		    "无法计算，表达式为空",
		};
	}

	EvaluatorState state{tokens, 0};
	EvaluationResult result = state.parseExpression();

	if (state.current().terminal != Terminal::End) {
		return {
		    false,
		    0,
		    "求值失败: 表达式未在结束符停止",
		};
	}

	return result;
}