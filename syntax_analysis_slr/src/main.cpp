#include "follow_set.h"
#include "grammar.h"
#include "lr0_automaton.h"
#include "parser.h"
#include "result_writer.h"
#include "slr_table.h"
#include "token_reader.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr const char *TOKEN_PATH = "./lexical_analysis/output/result.txt";
constexpr const char *LEXICAL_ERROR_PATH =
    "./lexical_analysis/output/error.txt";
constexpr const char *OUTPUT_DIR = "./syntax_analysis_slr/output";

struct Summary {
	std::size_t total = 0;
	std::size_t accepted = 0;
	std::size_t rejected = 0;
};

Summary summarize(const std::vector<ParseResult> &parse_results) {
	Summary summary;
	summary.total = parse_results.size();

	for (const ParseResult &result : parse_results) {
		if (result.accepted) {
			++summary.accepted;
		}
		else {
			++summary.rejected;
		}
	}

	return summary;
}

void printSummary(const Summary &summary) {
	std::cout << "SLR(1) 语法分析完成\n";
	std::cout << "表达式总数: " << summary.total << "\n";
	std::cout << "正确: " << summary.accepted << "\n";
	std::cout << "错误: " << summary.rejected << "\n";
	std::cout << "详细结果见 " << OUTPUT_DIR << "/\n";
}

void printStaticSummary(const LR0Automaton &automaton,
                        const SLRTableBuildResult &table_result) {
	std::cout << "LR(0) 项目集数量: " << automaton.item_sets.size() << "\n";
	std::cout << "SLR(1) 表冲突数量: " << table_result.conflicts.size() << "\n";
}
} // namespace

int main() {
	try {
		Grammar grammar;

		FollowSetCalculator follow_calculator(grammar);
		FollowSetResult follow_result = follow_calculator.calculate();

		LR0AutomatonBuilder automaton_builder(grammar);
		LR0Automaton automaton = automaton_builder.build();

		SLRTableBuilder table_builder(grammar, automaton, follow_result);
		SLRTableBuildResult table_result = table_builder.build();

		ResultWriter writer(OUTPUT_DIR);
		writer.writeStaticOutputs(grammar, follow_result, automaton,
		                          table_result);

		printStaticSummary(automaton, table_result);

		if (!table_result.conflicts.empty()) {
			std::cerr << "SLR(1) 分析表存在冲突，语法分析未执行。\n";
			std::cerr << "冲突详情见 " << OUTPUT_DIR << "/slr_table.txt\n";
			return 1;
		}

		TokenReader reader;
		TokenReaderResult token_result =
		    reader.read(TOKEN_PATH, LEXICAL_ERROR_PATH);

		if (!token_result.lexical_passed) {
			writer.writeLexicalFailure(token_result.lexical_error_text);
			std::cerr << "词法分析未通过，SLR(1) 语法分析未执行。\n";
			std::cerr << "错误详情见 " << OUTPUT_DIR << "/error.txt\n";
			return 1;
		}

		if (!token_result.file_errors.empty()) {
			writer.writeTokenFileErrors(token_result.file_errors);
			std::cerr << "token 文件存在错误，SLR(1) 语法分析未执行。\n";
			std::cerr << "错误详情见 " << OUTPUT_DIR << "/error.txt\n";
			return 1;
		}

		Parser parser(grammar, table_result.table);

		std::vector<ParseResult> parse_results;
		parse_results.reserve(token_result.expressions.size());

		for (const ExpressionInput &expression : token_result.expressions) {
			parse_results.push_back(parser.parse(expression));
		}

		writer.writeParseOutputs(parse_results);
		printSummary(summarize(parse_results));

		return 0;
	} catch (const std::exception &ex) {
		std::cerr << "SLR(1) 语法分析程序异常: " << ex.what() << "\n";
		return 1;
	}
}
