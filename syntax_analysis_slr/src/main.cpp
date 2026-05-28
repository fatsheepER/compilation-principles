#include "follow_set.h"
#include "grammar.h"
#include "lr0_automaton.h"

#include <iostream>
#include <optional>

namespace {
void printProductions(const Grammar &grammar) {
	std::cout << "=== Productions ===\n";

	for (const Production &production : grammar.productions()) {
		std::cout << toString(production) << "\n";
	}

	std::cout << "\n";
}

void printFirstFollow(const Grammar &grammar,
                      const FollowSetResult &follow_result) {
	std::cout << "=== FIRST sets ===\n";

	for (NonTerminal non_terminal : grammar.nonTerminals()) {
		std::cout << "FIRST(" << toString(non_terminal) << ") = ";
		std::cout << toString(follow_result.first.at(non_terminal)) << "\n";
	}

	std::cout << "\n";

	std::cout << "=== FOLLOW sets ===\n";

	for (NonTerminal non_terminal : grammar.nonTerminals()) {
		std::cout << "FOLLOW(" << toString(non_terminal) << ") = ";
		std::cout << toString(follow_result.follow.at(non_terminal)) << "\n";
	}

	std::cout << "\n";
}

void printAutomaton(const Grammar &grammar, const LR0Automaton &automaton) {
	std::cout << "=== LR(0) item sets ===\n";

	for (const ItemSet &item_set : automaton.item_sets) {
		std::cout << toString(item_set, grammar) << "\n";
	}

	std::cout << "=== LR(0) transitions ===\n";

	for (const Transition &transition : automaton.transitions) {
		std::cout << toString(transition) << "\n";
	}

	std::cout << "\n";
}

bool expectTransition(const LR0Automaton &automaton, int from,
                      const Symbol &symbol, int expected_to) {
	std::optional<int> actual_to = automaton.transition(from, symbol);

	if (!actual_to.has_value()) {
		std::cout << "[FAIL] missing transition: I" << from << " -- "
		          << toString(symbol) << " --> I" << expected_to << "\n";
		return false;
	}

	if (*actual_to != expected_to) {
		std::cout << "[FAIL] wrong transition: I" << from << " -- "
		          << toString(symbol) << " --> I" << *actual_to
		          << ", expected I" << expected_to << "\n";
		return false;
	}

	std::cout << "[OK] I" << from << " -- " << toString(symbol) << " --> I"
	          << expected_to << "\n";
	return true;
}

bool runChecks(const LR0Automaton &automaton) {
	std::cout << "=== Checks ===\n";

	bool ok = true;

	if (automaton.item_sets.size() != 13) {
		std::cout << "[FAIL] LR(0) item set count = "
		          << automaton.item_sets.size() << ", expected 13\n";
		ok = false;
	}
	else {
		std::cout << "[OK] LR(0) item set count = 13\n";
	}

	ok = expectTransition(automaton, 0, makeNonTerminal(NonTerminal::S), 1) &&
	     ok;
	ok = expectTransition(automaton, 0, makeNonTerminal(NonTerminal::E), 2) &&
	     ok;
	ok = expectTransition(automaton, 0, makeNonTerminal(NonTerminal::T), 3) &&
	     ok;
	ok = expectTransition(automaton, 0, makeNonTerminal(NonTerminal::F), 4) &&
	     ok;
	ok =
	    expectTransition(automaton, 0, makeTerminal(Terminal::LParen), 5) && ok;
	ok = expectTransition(automaton, 0, makeTerminal(Terminal::Id), 6) && ok;

	ok = expectTransition(automaton, 2, makeTerminal(Terminal::Plus), 7) && ok;
	ok = expectTransition(automaton, 3, makeTerminal(Terminal::Mul), 8) && ok;
	ok = expectTransition(automaton, 5, makeNonTerminal(NonTerminal::E), 9) &&
	     ok;
	ok = expectTransition(automaton, 10, makeTerminal(Terminal::Mul), 8) && ok;

	std::cout << "\n";
	return ok;
}
} // namespace

int main() {
	try {
		Grammar grammar;

		printProductions(grammar);

		FollowSetCalculator follow_calculator(grammar);
		FollowSetResult follow_result = follow_calculator.calculate();

		printFirstFollow(grammar, follow_result);

		LR0AutomatonBuilder automaton_builder(grammar);
		LR0Automaton automaton = automaton_builder.build();

		printAutomaton(grammar, automaton);

		const bool ok = runChecks(automaton);

		if (!ok) {
			std::cout << "阶段性测试未通过。\n";
			return 1;
		}

		std::cout << "阶段性测试通过。\n";
		return 0;
	} catch (const std::exception &ex) {
		std::cerr << "程序异常: " << ex.what() << "\n";
		return 1;
	}
}