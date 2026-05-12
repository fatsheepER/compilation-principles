#include "first_last_vt.h"
#include "grammar.h"
#include "parser.h"
#include "precedence_table.h"

#include <iostream>

namespace {
void printPrecedenceTable(const Grammar &grammar,
                          const PrecedenceTable &table) {
	std::cout << "\nPrecedence Table:\n";

	std::cout << "    ";
	for (const auto right : grammar.terminals()) {
		std::cout << toString(right) << "   ";
	}
	std::cout << '\n';

	for (const auto left : grammar.terminals()) {
		std::cout << toString(left) << "   ";

		for (const auto right : grammar.terminals()) {
			const auto relation = table.lookup(left, right);
			const std::string text = relationCell(relation);

			if (text.empty()) {
				std::cout << "    ";
			}
			else {
				std::cout << text << "   ";
			}
		}

		std::cout << '\n';
	}
}

std::vector<InputToken> input = {
    {Terminal::LParen, "(", 1}, {Terminal::Id, "i", 2},
    {Terminal::Plus, "+", 3},   {Terminal::Id, "i", 4},
    {Terminal::RParen, ")", 5}, {Terminal::Mul, "*", 6},
    {Terminal::Id, "i", 7},     {Terminal::End, "#", 8},
};
} // namespace

int main() {
	Grammar grammar;

	FirstLastVTCalculator vt_calculator;
	const FirstLastVTResult first_last_vt = vt_calculator.calculate(grammar);

	PrecedenceTableBuilder table_builder;
	const PrecedenceTableBuildResult table_result =
	    table_builder.build(grammar, first_last_vt);

	if (!table_result.success()) {
		std::cout << "算符优先关系表存在冲突:\n";

		for (const auto &conflict : table_result.conflicts) {
			std::cout << toString(conflict.left) << ", "
			          << toString(conflict.right) << ": 已有 "
			          << toString(conflict.existing) << ", 新关系 "
			          << toString(conflict.incoming) << ", " << conflict.reason
			          << '\n';
		}

		return 1;
	}

	printPrecedenceTable(grammar, table_result.table);

	OperatorPrecedenceParser parser;
	const auto parse_result = parser.parse(input, table_result.table);

	for (const auto &step : parse_result.steps) {
		std::cout << step.index << "\t" << step.stack << "\t"
		          << step.remaining_input << "\t" << step.relation << "\t"
		          << step.action << '\n';
	}

	std::cout << (parse_result.accepted ? "正确" : "错误") << '\n';
	if (!parse_result.accepted) {
		std::cout << parse_result.error_message << '\n';
	}

	return 0;
}
