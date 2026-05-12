#include "first_last_vt.h"
#include "grammar.h"
#include "parser.h"
#include "precedence_table.h"
#include "result_writer.h"
#include "token_reader.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

// usage:
// syntax_analysis_operator_precedence
// <token_path> <lexical_error_path> <output_dir>
int main(int argc, char *argv[]) {
	try {
		fs::path token_path = "../lexical_analysis/output/result.txt";
		fs::path lexical_error_path = "../lexical_analysis/output/error.txt";
		fs::path output_dir = "output";

		if (argc >= 2) {
			token_path = argv[1];
		}

		if (argc >= 3) {
			lexical_error_path = argv[2];
		}

		if (argc >= 4) {
			output_dir = argv[3];
		}

		Grammar grammar;

		FirstLastVTCalculator vt_calculator;
		const FirstLastVTResult first_last_vt =
		    vt_calculator.calculate(grammar);

		PrecedenceTableBuilder table_builder;
		const PrecedenceTableBuildResult table_result =
		    table_builder.build(grammar, first_last_vt);

		TokenReader reader;
		const TokenReaderResult read_result =
		    reader.read(token_path.string(), lexical_error_path.string());

		std::vector<OperatorExpressionReport> reports;

		if (table_result.success() && read_result.lexical_passed) {
			OperatorPrecedenceParser parser;

			for (const auto &expression : read_result.expressions) {
				OperatorExpressionReport report;
				report.index = expression.index;
				report.tokens = expression.tokens;
				report.input_errors = expression.errors;

				if (report.input_errors.empty()) {
					report.parser_ran = true;
					report.parse_result =
					    parser.parse(report.tokens, table_result.table);
				}

				reports.push_back(report);
			}
		}

		OperatorPrecedenceResultWriter writer;
		const OperatorPrecedenceResultWriteSummary summary =
		    writer.write(output_dir.string(), grammar, first_last_vt,
		                 table_result, read_result, reports);

		if (!table_result.success()) {
			std::cout << "算符优先关系表构造失败，未执行表达式分析\n";
			std::cout << "冲突数量: " << table_result.conflicts.size() << "\n";
		}
		else if (!read_result.lexical_passed) {
			std::cout << "词法分析未通过，未执行算符优先语法分析\n";
		}
		else {
			std::cout << "算符优先语法分析完毕\n";
			std::cout << "共分析 " << summary.total << " 条表达式\n";
			std::cout << "正确共 " << summary.accepted << " 条\n";
			std::cout << "错误共 " << summary.rejected << " 条\n";
		}

		std::cout << "token 输入文件: " << token_path << "\n";
		std::cout << "词法错误文件: " << lexical_error_path << "\n";
		std::cout << "算符优先分析输出目录: " << output_dir << "\n";

		return table_result.success() && read_result.lexical_passed ? 0 : 1;
	} catch (const std::exception &ex) {
		std::cerr << "Fatal error: " << ex.what() << "\n";
		return 1;
	}
}