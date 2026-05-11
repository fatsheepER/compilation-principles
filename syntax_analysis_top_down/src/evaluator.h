#pragma once

#include <string>
#include <vector>

#include "input_token.h"

struct EvaluationResult {
	bool available = false; // 是否产生了有效表达式值
	long long value = 0;    // 表达式求值结果
	std::string message;
};

class ExpressionEvaluator {
  public:
	EvaluationResult evaluate(const std::vector<InputToken> &tokens) const;
};
