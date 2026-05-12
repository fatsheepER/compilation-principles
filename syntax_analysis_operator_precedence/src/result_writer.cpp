#include "result_writer.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

OperatorPrecedenceResultWriteSummary OperatorPrecedenceResultWriter::write(
    const std::string &output_dir, const Grammar &grammar,
    const FirstLastVTResult &first_last_vt,
    const PrecedenceTableBuildResult &table_result,
    const TokenReaderResult &read_result,
    const std::vector<OperatorExpressionReport> &reports) const {
	fs::create_directories(output_dir);

	const fs::path result_path = fs::path(output_dir) / "result.txt";
	const fs::path error_path = fs::path(output_dir) / "error.txt";
	const fs::path first_last_vt_path =
	    fs::path(output_dir) / "first_last_vt.txt";
	const fs::path precedence_table_path =
	    fs::path(output_dir) / "precedence_table.txt";
	const fs::path steps_path = fs::path(output_dir) / "steps.txt";

	std::ofstream result_ofs(result_path);
	std::ofstream error_ofs(error_path);
	std::ofstream first_last_vt_ofs(first_last_vt_path);
	std::ofstream precedence_table_ofs(precedence_table_path);
	std::ofstream steps_ofs(steps_path);

	if (!result_ofs || !error_ofs || !first_last_vt_ofs ||
	    !precedence_table_ofs || !steps_ofs) {
		throw std::runtime_error("无法打开输出文件路径");
	}

	writeFirstLastVT(first_last_vt_ofs, grammar, first_last_vt);
	writePrecedenceTable(precedence_table_ofs, grammar, table_result.table);

	OperatorPrecedenceResultWriteSummary summary;

	// bad grammar
	if (!table_result.success()) {
		result_ofs << "算符优先关系表构造失败，未执行输入表达式分析\n";

		error_ofs << "算符优先关系表存在冲突:\n";
		writeTableConflicts(error_ofs, table_result.conflicts);

		steps_ofs << "算符优先关系表构造失败，无分析步骤\n";
		return summary;
	}

	// bad input lexically
	if (!read_result.lexical_passed) {
		result_ofs << "词法分析未通过，语法分析未执行\n";

		error_ofs << "词法分析错误如下:\n";
		error_ofs << read_result.lexical_error_text << '\n';

		steps_ofs << "词法分析未通过，无算符优先分析步骤\n";
		return summary;
	}

	// parse carried out successfully

	for (const auto &file_error : read_result.file_errors) {
		writeReadError(error_ofs, file_error);
	}

	// report for each input expression
	for (const auto &report : reports) {
		++summary.total;

		// results
		const bool accepted = report.parser_ran &&
		                      report.parse_result.accepted &&
		                      report.input_errors.empty();

		if (accepted) {
			++summary.accepted;
			result_ofs << "表达式 " << report.index << ": 正确\n";
		}
		else {
			++summary.rejected;
			result_ofs << "表达式 " << report.index << ": 错误\n";
		}

		result_ofs << "输入符号: " << tokenSequenceToString(report.tokens)
		           << "\n\n";
		// results over

		// input errors
		if (!report.input_errors.empty()) {
			error_ofs << "表达式 " << report.index << " 存在输入错误:\n";
			for (const auto &input_error : report.input_errors) {
				writeReadError(error_ofs, input_error);
			}
			error_ofs << "\n";
			// input errors over
		}
		// parse errors
		else if (report.parser_ran && !report.parse_result.accepted) {
			error_ofs << "表达式 " << report.index
			          << " 存在语法错误: " << report.parse_result.error_message
			          << ", 错误 token 来源行号 "
			          << report.parse_result.error_token_index << "\n\n";
			// parse errors over
		}
	}

	// steps
	writeSteps(steps_ofs, reports);
	// steps over

	if (summary.rejected == 0 && read_result.file_errors.empty()) {
		error_ofs << "无错误\n";
	}

	return summary;
}

void OperatorPrecedenceResultWriter::writeFirstLastVT(
    std::ostream &os, const Grammar &grammar,
    const FirstLastVTResult &first_last_vt) {
	os << "FirstVT 集:\n";
	for (const auto non_terminal : grammar.nonTerminals()) {
		os << "FirstVT(" << toString(non_terminal) << ") = ";

		const auto it = first_last_vt.first_vt.find(non_terminal);
		if (it == first_last_vt.first_vt.end()) {
			os << "{}\n";
		}
		else {
			os << terminalSetToString(it->second) << '\n';
		}
	}

	os << "\nLastVT 集:\n";
	for (const auto non_terminal : grammar.nonTerminals()) {
		os << "LastVT(" << toString(non_terminal) << ") = ";

		const auto it = first_last_vt.last_vt.find(non_terminal);
		if (it == first_last_vt.last_vt.end()) {
			os << "{}\n";
		}
		else {
			os << terminalSetToString(it->second) << '\n';
		}
	}
}

void OperatorPrecedenceResultWriter::writePrecedenceTable(
    std::ostream &os, const Grammar &grammar, const PrecedenceTable &table) {
	os << "算符优先关系表:\n\n";

	os << "    ";
	for (const auto right : grammar.terminals()) {
		os << toString(right) << "   ";
	}
	os << '\n';

	for (const auto left : grammar.terminals()) {
		os << toString(left) << "   ";

		for (const auto right : grammar.terminals()) {
			const auto relation = table.lookup(left, right);
			const std::string text = relationCell(relation);

			if (text.empty()) {
				os << "    ";
			}
			else {
				os << text << "   ";
			}
		}

		os << '\n';
	}
}

void OperatorPrecedenceResultWriter::writeTableConflicts(
    std::ostream &os, const std::vector<PrecedenceConflict> &conflicts) {
	for (const auto &conflict : conflicts) {
		os << toString(conflict.left) << ", " << toString(conflict.right)
		   << ": 已有 " << toString(conflict.existing) << ", 新关系 "
		   << toString(conflict.incoming) << ", " << conflict.reason << '\n';
	}
}

void OperatorPrecedenceResultWriter::writeSteps(
    std::ostream &os, const std::vector<OperatorExpressionReport> &reports) {
	for (const auto &report : reports) {
		const bool accepted = report.parser_ran &&
		                      report.parse_result.accepted &&
		                      report.input_errors.empty();

		os << "表达式 " << report.index << ": " << (accepted ? "正确" : "错误")
		   << '\n';
		os << "输入符号: " << tokenSequenceToString(report.tokens) << '\n';

		if (!report.input_errors.empty()) {
			os << "未执行算符优先分析，输入 token 存在错误\n\n";
			continue;
		}

		if (!report.parser_ran) {
			os << "未执行算符优先分析\n\n";
			continue;
		}

		os << "步骤\t分析栈\t\t剩余输入\t\t优先关系\t动作\n";

		for (const auto &step : report.parse_result.steps) {
			os << step.index << "\t";
			os << step.stack << "\t\t";
			os << step.remaining_input << "\t\t";
			os << step.relation << "\t";
			os << step.action << '\n';
		}

		if (!report.parse_result.accepted) {
			os << "错误原因: " << report.parse_result.error_message << '\n';
		}

		os << '\n';
	}
}

void OperatorPrecedenceResultWriter::writeReadError(
    std::ostream &os, const TokenReadError &error) {
	if (error.expression_index > 0) {
		os << "表达式 " << error.expression_index << ": ";
	}

	if (error.line > 0) {
		os << "第 " << error.line << " 行: ";
	}

	os << error.message << '\n';
}

std::string OperatorPrecedenceResultWriter::terminalSetToString(
    const std::set<Terminal> &terminals) {
	std::ostringstream oss;

	oss << "{ ";

	bool first = true;
	for (const auto terminal : terminals) {
		if (!first) {
			oss << ", ";
		}

		oss << toString(terminal);
		first = false;
	}

	oss << " }";
	return oss.str();
}

std::string OperatorPrecedenceResultWriter::tokenSequenceToString(
    const std::vector<InputToken> &tokens) {
	if (tokens.empty()) {
		return "空";
	}

	std::ostringstream oss;

	for (const auto &token : tokens) {
		if (!token.lexeme.empty()) {
			oss << token.lexeme;
		}
		else {
			oss << toString(token.terminal);
		}
	}

	return oss.str();
}