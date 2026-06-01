#include "slr_table.h"
#include "follow_set.h"
#include "grammar.h"
#include "lr0_automaton.h"
#include "terminal.h"

#include <optional>
#include <sstream>

// Action

bool operator==(const Action &lhs, const Action &rhs) {
	return lhs.kind == rhs.kind && lhs.next_state == rhs.next_state &&
	       lhs.production_id == rhs.production_id;
}

bool operator!=(const Action &lhs, const Action &rhs) {
	return !(lhs == rhs);
}

std::string toString(const Action &action) {
	std::ostringstream oss;

	switch (action.kind) {
	case ActionKind::Shift:
		oss << "s" << action.next_state;
		break;
	case ActionKind::Reduce:
		oss << "r" << action.production_id;
		break;
	case ActionKind::Accept:
		oss << "acc";
		break;
	case ActionKind::Error:
		break;
	}

	return oss.str();
}

// SLR(1) conflict

std::string toString(const SLRConflict &conflict) {
	std::ostringstream oss;

	oss << "state I" << conflict.state;
	oss << ", terminal " << toString(conflict.terminal);
	oss << ": existing action " << toString(conflict.existing_action);
	oss << ", new action " << toString(conflict.new_action);

	return oss.str();
}

// SLR(1) table

Action SLRTable::action(int state, Terminal terminal) const {
	const ActionKey key{state, terminal};

	const auto iter = actions_.find(key);
	if (iter != actions_.end()) {
		return iter->second;
	}

	return Action{}; // error
}

std::optional<int> SLRTable::goTo(int state, NonTerminal non_terminal) const {
	const GotoKey key{state, non_terminal};

	const auto iter = gotos_.find(key);
	if (iter != gotos_.end()) {
		return iter->second;
	}

	return std::nullopt;
}

const std::map<SLRTable::ActionKey, Action> &SLRTable::actions() const {
	return actions_;
}

const std::map<SLRTable::GotoKey, int> &SLRTable::gotos() const {
	return gotos_;
}

void SLRTable::setAction(int state, Terminal terminal, const Action &action) {
	actions_[{state, terminal}] = action;
}

void SLRTable::setGoto(int state, NonTerminal non_terminal, int next_state) {
	gotos_[{state, non_terminal}] = next_state;
}

// SLR(1) table builder

SLRTableBuilder::SLRTableBuilder(const Grammar &grammar,
                                 const LR0Automaton &automaton,
                                 const FollowSetResult &follow_result)
    : grammar_(grammar), automaton_(automaton), follow_result_(follow_result) {
}

SLRTableBuildResult SLRTableBuilder::build() const {
	SLRTableBuildResult result;

	// 根据 LR(0) 自动机的边填写移进 ACTION 和 GOTO 表
	//
	// if (goto[Ii, a] = Ij) and (a is Terminal)
	// => ACTION[i, a] = shift j
	//
	// if (goto[Ii, A] = Ij) and (A is NonTerminal)
	// => ACTION[i, A] = j
	for (const Transition &transition : automaton_.transitions) {
		if (isTerminal(transition.symbol)) {
			Action action;
			action.kind = ActionKind::Shift;
			action.next_state = transition.to_state;

			putAction(result, transition.from_state, transition.symbol.terminal,
			          action);
		}
		else { // isNonTerminal
			result.table.setGoto(transition.from_state,
			                     transition.symbol.non_terminal,
			                     transition.to_state);
		}
	}

	// 根据 isCompleteItem 填写归约和接收动作
	//
	// 若 item 为 A -> X .
	// => 对于所有 a in Follow(A), ACTION[i, a] = reduce A -> X
	//
	// 特别的，如 item 为 S' -> S .
	// => ACTION[i, #] = acc
	for (const ItemSet &item_set : automaton_.item_sets) {
		for (const LR0Item &item : item_set.items) {
			if (!isCompleteItem(item)) {
				continue;
			}

			const Production &production =
			    grammar_.production(item.production_id);

			// S' -> S .
			if (production.lhs == grammar_.augmentedStartSymbol()) {
				Action action;
				action.kind = ActionKind::Accept;

				// ACTION[i, #] = acc
				putAction(result, item_set.id, Terminal::End, action);
				continue;
			}

			// A -> X .
			Action action;
			action.kind = ActionKind::Reduce;
			action.production_id = production.id;

			const auto follow_iter = follow_result_.follow.find(production.lhs);

			// 正常情况不应该缺失
			if (follow_iter == follow_result_.follow.end()) {
				continue;
			}

			for (Terminal terminal : follow_iter->second) {
				// ACTION[i, a] = reduce A -> X
				putAction(result, item_set.id, terminal, action);
			}
		}
	}

	return result;
}

bool SLRTableBuilder::isCompleteItem(const LR0Item &item) const {
	const Production &production = grammar_.production(item.production_id);
	return item.dot_position >= production.rhs.size();
}

void SLRTableBuilder::putAction(SLRTableBuildResult &result, int state,
                                Terminal terminal, const Action &action) const {
	Action existing_action = result.table.action(state, terminal);

	// 发生冲突
	if (existing_action.kind != ActionKind::Error &&
	    existing_action != action) {
		result.conflicts.push_back({
		    state,
		    terminal,
		    existing_action,
		    action,
		});

		return;
	}

	result.table.setAction(state, terminal, action);
}