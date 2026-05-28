#include "follow_set.h"
#include "grammar.h"
#include "terminal.h"

#include <cstddef>
#include <sstream>

FollowSetCalculator::FollowSetCalculator(const Grammar &grammar)
    : grammar_(grammar) {
}

FollowSetResult FollowSetCalculator::calculate() const {
	FollowSetResult result;

	// 初始化普通非终结符
	for (NonTerminal non_terminal : grammar_.nonTerminals()) {
		result.first[non_terminal] = FirstSet{};
		result.follow[non_terminal] = {};
	}

	// 增广开始符号也参与计算，便于由 S' -> S 传播 #
	result.first[grammar_.augmentedStartSymbol()] = FirstSet{};
	result.follow[grammar_.augmentedStartSymbol()] = {};

	// Follow(S') = { # }
	result.follow[grammar_.augmentedStartSymbol()].insert(Terminal::End);

	// 计算 First 集
	bool changed = true;
	while (changed) {
		changed = false;

		for (const Production &production : grammar_.productions()) {
			FirstSet sequence_first =
			    firstOfSequence(production.rhs, 0, result.first);

			if (addTerminals(result.first[production.lhs].terminals,
			                 sequence_first.terminals)) {
				changed = true;
			}
		}
	}

	// 计算 Follow 集
	changed = true;
	while (changed) {
		changed = false;

		for (const Production &production : grammar_.productions()) {
			for (std::size_t i = 0; i < production.rhs.size(); ++i) {
				const Symbol &symbol = production.rhs[i];

				if (!isNonTerminal(symbol)) {
					continue;
				}

				NonTerminal current = symbol.non_terminal;

				FirstSet suffix_first =
				    firstOfSequence(production.rhs, i + 1, result.first);

				if (addTerminals(result.follow[current],
				                 suffix_first.terminals)) {
					changed = true;
				}

				if (suffix_first.contains_epsilon) {
					if (addTerminals(result.follow[current],
					                 result.follow[production.lhs])) {
						changed = true;
					}
				}
			}
		}
	}

	return result;
}

FirstSet FollowSetCalculator::firstOfSequence(
    const std::vector<Symbol> &symbols, std::size_t start_index,
    const std::map<NonTerminal, FirstSet> &first_sets) const {
	FirstSet result;

	// 空序列的 First: epsilon
	if (start_index >= symbols.size()) {
		result.contains_epsilon = true;
		return result;
	}

	// 从 start_index 开始的整个后缀是否都能推出空串 epsilon
	// 如果都可以，那么整个序列也能推出 epsilon
	bool all_nullable = true;

	for (std::size_t i = start_index; i < symbols.size(); ++i) {
		const Symbol &symbol = symbols[i];

		// 终结符就是 First 唯一元素，不用往后看了
		if (isTerminal(symbol)) {
			result.terminals.insert(symbol.terminal);
			all_nullable = false;
			break;
		}

		// 非终结符：查其 First，添加全部终结符
		const auto iter = first_sets.find(symbol.non_terminal);
		if (iter == first_sets.end()) {
			all_nullable = false;
			break;
		}

		addTerminals(result.terminals, iter->second.terminals);

		// 如果当前非终结符推不出 epsilon
		// 不能继续往后看了
		if (!iter->second.contains_epsilon) {
			all_nullable = false;
			break;
		}
	}

	result.contains_epsilon = all_nullable;
	return result;
}

bool FollowSetCalculator::addTerminal(std::set<Terminal> &target,
                                      Terminal terminal) {
	const auto [_, inserted] = target.insert(terminal);
	return inserted;
}

bool FollowSetCalculator::addTerminals(std::set<Terminal> &target,
                                       const std::set<Terminal> &source) {
	bool changed = false;
	for (Terminal terminal : source) {
		if (addTerminal(target, terminal)) {
			changed = true;
		}
	}

	return changed;
}

std::string toString(const FirstSet &first_set) {
	std::ostringstream oss;

	oss << "{ ";

	bool first = true;
	for (Terminal terminal : first_set.terminals) {
		if (!first) {
			oss << ", ";
		}

		oss << toString(terminal);
		first = false;
	}

	if (first_set.contains_epsilon) {
		if (!first) {
			oss << ", ";
		}

		oss << "ε";
	}

	oss << " }";
	return oss.str();
}

std::string toString(const std::set<Terminal> &terminals) {
	std::ostringstream oss;

	oss << "{ ";

	bool first = true;
	for (Terminal terminal : terminals) {
		if (!first) {
			oss << ", ";
		}

		oss << toString(terminal);
		first = false;
	}

	oss << " }";
	return oss.str();
}