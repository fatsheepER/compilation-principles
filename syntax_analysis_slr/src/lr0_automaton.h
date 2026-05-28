// 构造 LR(0) 项目集规范组

#pragma once

#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "grammar.h"

// LR(0) item: A -> X . Y

struct LR0Item {
	int production_id = 0;
	std::size_t dot_position = 0; // start from 0
};

bool operator==(const LR0Item &lhs, const LR0Item &rhs);
bool operator!=(const LR0Item &lhs, const LR0Item &rhs);
bool operator<(const LR0Item &lhs, const LR0Item &rhs);

std::string toString(const LR0Item &item, const Grammar &grammar);

// Item set: represents a state in an automaton

struct ItemSet {
	int id = 0;
	std::set<LR0Item> items;
};

std::string toString(const ItemSet &item_set, const Grammar &grammar);

// Transition of states in an automaton

struct Transition {
	int from_state = 0;
	Symbol symbol;
	int to_state = 0;
};

std::string toString(const Transition &transition);

// LR(0) automaton

struct LR0Automaton {
	std::vector<ItemSet> item_sets;
	std::vector<Transition> transitions;

	const ItemSet &itemSet(int id) const;
	std::optional<int> transition(int from_state, const Symbol &symbol) const;
};

class LR0AutomatonBuilder {
  public:
	explicit LR0AutomatonBuilder(const Grammar &grammar);

	LR0Automaton build() const;

  private:
	const Grammar &grammar_;

	// 构造项目集闭包
	std::set<LR0Item> closure(const std::set<LR0Item> &items) const;
	// 构造项目集的后继项目集
	std::set<LR0Item> goTo(const ItemSet &item_set, const Symbol &symbol) const;

	// 是归约/接收项目
	bool isCompleteItem(const LR0Item &item) const;

	std::optional<Symbol> symbolAfterDot(const LR0Item &item) const;

	static int findItemSetId(const std::vector<ItemSet> &item_set,
	                         const std::set<LR0Item> &items);
};