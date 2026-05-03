#include "result_writer.h"
#include "grammar.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

ResultWriteSummary
ResultWriter::write(const std::string &output_dir,
                    const TokenReaderResult &read_result,
                    const std::vector<ExpressionReport> &reports) const {
	fs::create_directories(output_dir);

	const fs::path result_path = fs::path(output_dir) / "result.txt";
	const fs::path error_path = fs::path(output_dir) / "error.txt";
	const fs::path steps_path = fs::path(output_dir) / "steps.txt";

	std::ofstream result_ofs(result_path);
	if (!result_ofs) {
		throw std::runtime_error("无法打开语法分析结果文件: " +
		                         result_path.string());
	}

	std::ofstream error_ofs(error_path);
	if (!error_ofs) {
		throw std::runtime_error("无法打开语法分析错误文件: " +
		                         error_path.string());
	}

	std::ofstream steps_ofs(steps_path);
	if (!steps_ofs) {
		throw std::runtime_error("无法打开语法分析步骤文件: " +
		                         steps_path.string());
	}

	ResultWriteSummary summary;

	// 未通过词法分析
	if (!read_result.lexical_passed) {
		result_ofs << "词法分析未通过，语法分析未执行\n";

		error_ofs << "词法分析错误如下: \n";
		error_ofs << read_result.lexical_error_text << '\n';

		steps_ofs << "词法分析未通过，无预测分析步骤\n";
		return summary;
	}

	// 输出错误文件
	for (const auto &file_error : read_result.file_errors) {
		writeReadError(error_ofs, file_error);
	}

	// 逐表达式输出结果文件
	for (const auto &report : reports) {
		++summary.total;

		const bool accepted = report.parser_ran &&
		                      report.parse_result.accepted &&
		                      report.input_errors.empty();

		if (accepted) {
			++summary.accpeted;
			result_ofs << "表达式 " << report.index << ": 正确\n";
		}
		else {
			++summary.rejected;
			result_ofs << "表达式 " << report.index << ": 错误\n";
		}

		result_ofs << "输入符号: " << tokenSequenceToString(report.tokens)
		           << "\n\n";

		// 表达式存在输入错误
		if (!report.input_errors.empty()) {
			error_ofs << "表达式 " << report.index << " 存在输入错误: \n";
			for (const auto &input_error : report.input_errors) {
				writeReadError(error_ofs, input_error);
			}
			error_ofs << "\n";
		}
		// 表达式存在语法错误
		else if (report.parser_ran && !report.parse_result.accepted) {
			error_ofs << "表达式 " << report.index << " 语法错误: ";
			error_ofs << report.parse_result.error_message;
			error_ofs << ", 错误 token 来源行号 ";
			error_ofs << report.parse_result.error_token_index << "\n\n";
		}

		// 输出步骤文件
		steps_ofs << "表达式" << report.index << ": ";
		steps_ofs << (accepted ? "正确" : "错误") << "\n";
		steps_ofs << "输入符号: " << tokenSequenceToString(report.tokens)
		          << "\n";

		if (!report.parser_ran) {
			steps_ofs << "未执行预测分析\n\n";
			continue;
		}

		steps_ofs << "步骤\t分析栈\t\t剩余输入\t\t动作\n";
		for (const auto &step : report.parse_result.steps) {
			steps_ofs << step.index << "\t\t";
			steps_ofs << step.stack << "\t\t";
			steps_ofs << step.remaining_input << "\t\t";
			steps_ofs << step.action << "\n";
		}

		steps_ofs << "\n";
	}

	if (summary.rejected == 0 && read_result.file_errors.empty()) {
		error_ofs << "无错误\n";
	}

	return summary;
}

std::string
ResultWriter::tokenSequenceToString(const std::vector<InputToken> &tokens) {
	if (tokens.empty()) {
		return "空";
	}

	std::ostringstream oss;

	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (i > 0) {
			oss << ' ';
		}

		// 将 INT_LITERAL 和 i 都还原成原字面量
		if (!tokens[i].lexeme.empty()) {
			oss << tokens[i].lexeme;
		}
		else {
			oss << toString(tokens[i].terminal);
		}
	}

	return oss.str();
}

void ResultWriter::writeReadError(std::ostream &os,
                                  const TokenReadError &error) {
	if (error.expression_index > 0) {
		os << "表达式 " << error.expression_index << ": ";
	}

	if (error.line > 0) {
		os << "第 " << error.line << " 行: ";
	}

	os << error.message << "\n";
}