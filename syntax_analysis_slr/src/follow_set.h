// 计算 Follow 集

#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "grammar.h"
#include "terminal.h"

struct FirstSet {
	std::set<Terminal> terminals;
	bool contains_epsilon = false;
};

struct FollowSetResult {
	std::map<NonTerminal, FirstSet> first;
	std::map<NonTerminal, std::set<Terminal>> follow;
};

class FollowSetCalculator {
  public:
	explicit FollowSetCalculator(const Grammar &grammar);

	FollowSetResult calculate() const;

  private:
	const Grammar &grammar_;

	// 给定一个符号序列 symbols，
	// 从 start_index 开始，
	// 计算这个后缀序列的 First 集。
	FirstSet
	firstOfSequence(const std::vector<Symbol> &symbols, std::size_t start_index,
	                const std::map<NonTerminal, FirstSet> &first_sets) const;

	static bool addTerminal(std::set<Terminal> &target, Terminal terminal);
	static bool addTerminals(std::set<Terminal> &target,
	                         const std::set<Terminal> &source);
};

std::string toString(const FirstSet &first_set);
std::string toString(const std::set<Terminal> &terminals);