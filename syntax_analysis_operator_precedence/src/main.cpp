#include "first_last_vt.h"
#include "grammar.h"

#include <iostream>
#include <set>

namespace {
void printTerminalSet(const std::set<Terminal> &terminals) {
	std::cout << "{ ";
	bool first = true;

	for (const auto terminal : terminals) {
		if (!first) {
			std::cout << ", ";
		}

		std::cout << toString(terminal);
		first = false;
	}

	std::cout << " }";
}

void printFirstLastVT(const Grammar &grammar, const FirstLastVTResult &result) {
	std::cout << "FirstVT:\n";
	for (const auto non_terminal : grammar.nonTerminals()) {
		std::cout << "FirstVT(" << toString(non_terminal) << ") = ";
		printTerminalSet(result.first_vt.at(non_terminal));
		std::cout << '\n';
	}

	std::cout << "\nLastVT:\n";
	for (const auto non_terminal : grammar.nonTerminals()) {
		std::cout << "LastVT(" << toString(non_terminal) << ") = ";
		printTerminalSet(result.last_vt.at(non_terminal));
		std::cout << '\n';
	}
}
} // namespace

int main() {
	Grammar grammar;
	FirstLastVTCalculator calculator;
	const FirstLastVTResult result = calculator.calculate(grammar);

	printFirstLastVT(grammar, result);

	return 0;
}
