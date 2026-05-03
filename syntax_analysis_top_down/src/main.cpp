#include "evaluator.h"
#include "parser.h"
#include "result_writer.h"
#include "token_reader.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

// usage: sytax_analysis_top_down <token_path> <lexical_error_path> <output_dir>
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

		// 从 token 文件读取出表达式
		TokenReader reader;
		TokenReaderResult read_result =
		    reader.read(token_path.string(), lexical_error_path.string());

		// 各表达式的语法分析结果
		std::vector<ExpressionReport> reports;

		if (read_result.lexical_passed) {
			PredictiveParser parser;
			ExpressionEvaluator evaluatar;

			// 逐表达式进行语法分析
			for (const auto &expression : read_result.expressions) {
				ExpressionReport report;
				report.index = expression.index;
				report.tokens = expression.tokens;
				report.input_errors = expression.errors;

				if (report.input_errors.empty()) {
					report.parser_ran = true;
					report.parse_result = parser.parse(report.tokens);

					if (report.parse_result.accepted) {
						report.evaluation_ran = true;
						report.evaluation_result =
						    evaluatar.evaluate(report.tokens);
					}
				}

				reports.push_back(report);
			}
		}

		// 写结果
		ResultWriter writer;
		ResultWriteSummary summary =
		    writer.write(output_dir.string(), read_result, reports);

		if (!read_result.lexical_passed) {
			std::cout << "词法分析未通过，未执行语法分析\n";
		}
		else {
			std::cout << "语法分析完毕\n";
			std::cout << "共分析 " << summary.total << " 条表达式\n";
			std::cout << "正确共 " << summary.accpeted << "条\n";
			std::cout << "错误共 " << summary.rejected << "条\n";
		}

		std::cout << "token 输入文件: " << token_path << "\n";
		std::cout << "词法错误文件: " << lexical_error_path << "\n";
		std::cout << "语法分析输出目录: " << output_dir << "\n";

		return 0;
	} catch (const std::exception &ex) {
		std::cerr << "Fatal error: " << ex.what() << "\n";
		return 1;
	}
}