#include "lr0_automaton.h"
#include "grammar.h"

#include <deque>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// LR(0) item

bool operator==(const LR0Item &lhs, const LR0Item &rhs) {
	return lhs.production_id == rhs.production_id &&
	       lhs.dot_position == rhs.dot_position;
}

bool operator!=(const LR0Item &lhs, const LR0Item &rhs) {
	return !(lhs == rhs);
}

bool operator<(const LR0Item &lhs, const LR0Item &rhs) {
	if (lhs.production_id != rhs.production_id) {
		return lhs.production_id < rhs.production_id;
	}

	return lhs.dot_position < rhs.dot_position;
}

std::string toString(const LR0Item &item, const Grammar &grammar) {
	const Production &production = grammar.production(item.production_id);

	std::ostringstream oss;
	oss << toString(production.lhs) << " -> ";

	for (std::size_t i = 0; i <= production.rhs.size(); ++i) {
		if (i > 0) {
			oss << " ";
		}

		if (i == item.dot_position) {
			oss << "·";

			if (i < production.rhs.size()) {
				oss << " ";
			}
		}

		if (i < production.rhs.size()) {
			oss << toString(production.rhs[i]);
		}
	}

	return oss.str();
}

// Item set

std::string toString(const ItemSet &item_set, const Grammar &grammar) {
	std::ostringstream oss;

	oss << "I" << item_set.id << "\n";

	for (const LR0Item &item : item_set.items) {
		oss << "  " << toString(item, grammar) << "\n";
	}

	return oss.str();
}

// Transition

std::string toString(const Transition &transition) {
	std::ostringstream oss;

	oss << "I" << transition.from_state;
	oss << " -- " << toString(transition.symbol);
	oss << " --> ";
	oss << "I" << transition.to_state;

	return oss.str();
}

// LR(0) automaton

const ItemSet &LR0Automaton::itemSet(int id) const {
	for (const ItemSet &item_set : item_sets) {
		if (item_set.id == id) {
			return item_set;
		}
	}

	throw std::out_of_range("unknown LR(0) item set id: " + std::to_string(id));
}

std::optional<int> LR0Automaton::transition(int from_state,
                                            const Symbol &symbol) const {
	for (const Transition &transition : transitions) {
		if (transition.from_state == from_state &&
		    transition.symbol == symbol) {
			return transition.to_state;
		}
	}

	return std::nullopt;
}

LR0AutomatonBuilder::LR0AutomatonBuilder(const Grammar &grammar)
    : grammar_(grammar) {
}

LR0Automaton LR0AutomatonBuilder::build() const {
	LR0Automaton automaton;

	// 开始状态：增光开始符号的闭包
	std::set<LR0Item> start_items = closure({
	    {0, 0}, // S' -> . S
	});

	automaton.item_sets.push_back({
	    0,
	    start_items,
	});

	std::deque<int> pending; // 有新添加的状态
	pending.push_back(0);

	while (!pending.empty()) {
		const int current_id = pending.front();
		pending.pop_front();

		const ItemSet current_set = automaton.itemSet(current_id);

		for (const Symbol &symbol : grammar_.symbolsForAutomatonExpansion()) {
			std::set<LR0Item> next_items = goTo(current_set, symbol);

			if (next_items.empty()) {
				continue;
			}

			// 自动机状态集中是否存在该项目集
			int next_id = findItemSetId(automaton.item_sets, next_items);

			// 不存在，添加
			if (next_id == -1) {
				next_id = static_cast<int>(automaton.item_sets.size());

				automaton.item_sets.push_back({
				    next_id,
				    next_items,
				});

				pending.push_back(next_id);
			}

			automaton.transitions.push_back({
			    current_id,
			    symbol,
			    next_id,
			});
		}
	}

	return automaton;
}

std::set<LR0Item>
LR0AutomatonBuilder::closure(const std::set<LR0Item> &items) const {
	std::set<LR0Item> result = items;

	bool changed = true;
	while (changed) {
		changed = false;

		std::vector<LR0Item> snapshot(result.begin(), result.end());

		// 将 dot 后非终结符的产生式添加到闭包
		for (const LR0Item &item : snapshot) {
			std::optional<Symbol> next_symbol = symbolAfterDot(item);

			if (!next_symbol.has_value() || !isNonTerminal(*next_symbol)) {
				continue;
			}

			for (const Production *production :
			     grammar_.productionsFor(next_symbol->non_terminal)) {
				LR0Item new_item{
				    production->id,
				    0,
				};

				const auto [_, inserted] = result.insert(new_item);
				if (inserted) {
					changed = true;
				}
			}
		}
	}

	return result;
}

std::set<LR0Item> LR0AutomatonBuilder::goTo(const ItemSet &item_set,
                                            const Symbol &symbol) const {
	std::set<LR0Item> moved_items; // 后继项目集

	for (const LR0Item &item : item_set.items) {
		//      A --{symbol}-> X . symbol Y
		// =>   A -> X symbol . Y
		std::optional<Symbol> next_symbol = symbolAfterDot(item);

		if (!next_symbol.has_value() || *next_symbol != symbol) {
			continue;
		}

		moved_items.insert({
		    item.production_id,
		    item.dot_position + 1,
		});
	}

	if (moved_items.empty()) {
		return {};
	}

	return closure(moved_items);
}

bool LR0AutomatonBuilder::isCompleteItem(const LR0Item &item) const {
	const Production &production = grammar_.production(item.production_id);
	return item.dot_position >= production.rhs.size();
}

std::optional<Symbol>
LR0AutomatonBuilder::symbolAfterDot(const LR0Item &item) const {
	if (isCompleteItem(item)) {
		return std::nullopt;
	}

	const Production &production = grammar_.production(item.production_id);
	return production.rhs[item.dot_position];
}

int LR0AutomatonBuilder::findItemSetId(const std::vector<ItemSet> &item_sets,
                                       const std::set<LR0Item> &items) {
	for (const ItemSet &item_set : item_sets) {
		if (item_set.items == items) {
			return item_set.id;
		}
	}

	return -1;
}
