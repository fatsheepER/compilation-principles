#include "result_writer.h"

#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

ResultWriter::ResultWriter(std::filesystem::path output_dir)
    : output_dir_(std::move(output_dir)) {
}

void ResultWriter::writeStaticOutputs(
    const Grammar &grammar, const FollowSetResult &follow_result,
    const LR0Automaton &automaton,
    const SLRTableBuildResult &table_result) const {
	ensureOutputDirectory();

	// FOLLOW 集输出
	{
		std::ofstream ofs(outputPath("follow_sets.txt"));
		if (!ofs) {
			throw std::runtime_error("无法写入 follow_sets.txt");
		}

		ofs << "FIRST sets\n";
		for (NonTerminal non_terminal : grammar.nonTerminals()) {
			ofs << "FIRST(" << toString(non_terminal)
			    << ") = " << toString(follow_result.first.at(non_terminal))
			    << "\n";
		}

		ofs << "\nFOLLOW sets\n";
		for (NonTerminal non_terminal : grammar.nonTerminals()) {
			ofs << "FOLLOW(" << toString(non_terminal)
			    << ") = " << toString(follow_result.follow.at(non_terminal))
			    << "\n";
		}
	}

	// LR(0) 项目集与状态转换输出
	{
		std::ofstream ofs(outputPath("lr0_item_sets.txt"));
		if (!ofs) {
			throw std::runtime_error("无法写入 lr0_item_sets.txt");
		}

		ofs << "LR(0) item sets\n\n";
		for (const ItemSet &item_set : automaton.item_sets) {
			ofs << toString(item_set, grammar) << "\n";
		}

		ofs << "LR(0) transitions\n";
		for (const Transition &transition : automaton.transitions) {
			ofs << toString(transition) << "\n";
		}
	}

	// SLR(1) ACTION / GOTO 表输出
	{
		std::ofstream ofs(outputPath("slr_table.txt"));
		if (!ofs) {
			throw std::runtime_error("无法写入 slr_table.txt");
		}

		const std::vector<Terminal> terminals = {
		    Terminal::Id,     Terminal::Plus,   Terminal::Mul,
		    Terminal::LParen, Terminal::RParen, Terminal::End,
		};

		const std::vector<NonTerminal> non_terminals = grammar.nonTerminals();

		ofs << "SLR(1) ACTION / GOTO table\n\n";

		ofs << std::left << std::setw(8) << "State";
		for (Terminal terminal : terminals) {
			ofs << std::setw(10) << toString(terminal);
		}
		for (NonTerminal non_terminal : non_terminals) {
			ofs << std::setw(10) << toString(non_terminal);
		}
		ofs << "\n";

		for (const ItemSet &item_set : automaton.item_sets) {
			std::ostringstream state_name;
			state_name << "I" << item_set.id;

			ofs << std::left << std::setw(8) << state_name.str();

			for (Terminal terminal : terminals) {
				ofs << std::setw(10)
				    << actionText(table_result.table, item_set.id, terminal);
			}

			for (NonTerminal non_terminal : non_terminals) {
				ofs << std::setw(10)
				    << gotoText(table_result.table, item_set.id, non_terminal);
			}

			ofs << "\n";
		}

		ofs << "\nConflicts\n";
		if (table_result.conflicts.empty()) {
			ofs << "移进/归约冲突：无\n";
			ofs << "归约/归约冲突：无\n";
		}
		else {
			for (const SLRConflict &conflict : table_result.conflicts) {
				ofs << toString(conflict) << "\n";
			}
		}
	}
}

void ResultWriter::writeParseOutputs(
    const std::vector<ParseResult> &parse_results) const {
	ensureOutputDirectory();

	// result.txt：每条表达式的最终结论
	{
		std::ofstream ofs(outputPath("result.txt"));
		if (!ofs) {
			throw std::runtime_error("无法写入 result.txt");
		}

		for (const ParseResult &result : parse_results) {
			ofs << "表达式 " << result.expression_index << ": "
			    << (result.accepted ? "正确" : "错误") << "\n";
		}
	}

	// error.txt：详细错误信息
	{
		std::ofstream ofs(outputPath("error.txt"));
		if (!ofs) {
			throw std::runtime_error("无法写入 error.txt");
		}

		bool has_error = false;

		for (const ParseResult &result : parse_results) {
			if (!result.diagnostic.has_value()) {
				continue;
			}

			has_error = true;
			writeDiagnostic(ofs, *result.diagnostic);
			ofs << "\n";
		}

		if (!has_error) {
			ofs << "无错误\n";
		}
	}

	// steps.txt：移进、归约、接受或出错步骤
	{
		std::ofstream ofs(outputPath("steps.txt"));
		if (!ofs) {
			throw std::runtime_error("无法写入 steps.txt");
		}

		for (const ParseResult &result : parse_results) {
			ofs << "表达式 " << result.expression_index << "\n";

			if (result.steps.empty()) {
				ofs << "未执行 SLR(1) 分析。\n\n";
				continue;
			}

			ofs << std::left << std::setw(8) << "步骤" << std::setw(20)
			    << "状态栈" << std::setw(24) << "符号栈" << std::setw(32)
			    << "综合栈" << std::setw(24) << "剩余输入"
			    << "动作\n";

			for (const ParseStep &step : result.steps) {
				ofs << std::left << std::setw(8) << step.step_index
				    << std::setw(20) << step.state_stack << std::setw(24)
				    << step.symbol_stack << std::setw(32) << step.combined_stack
				    << std::setw(24) << step.remaining_input << step.action
				    << "\n";
			}

			ofs << "\n";
		}
	}
}

void ResultWriter::writeLexicalFailure(
    const std::string &lexical_error_text) const {
	ensureOutputDirectory();

	std::ofstream result_ofs(outputPath("result.txt"));
	std::ofstream error_ofs(outputPath("error.txt"));
	std::ofstream steps_ofs(outputPath("steps.txt"));

	if (!result_ofs || !error_ofs || !steps_ofs) {
		throw std::runtime_error("无法写入词法错误输出文件");
	}

	result_ofs << "词法分析未通过，SLR(1) 语法分析未执行。\n";

	error_ofs << "词法分析未通过，SLR(1) 语法分析未执行。\n";
	error_ofs << lexical_error_text << "\n";

	steps_ofs << "词法分析未通过，无 SLR(1) 分析步骤。\n";
}

void ResultWriter::writeTokenFileErrors(
    const std::vector<TokenReadError> &file_errors) const {
	ensureOutputDirectory();

	std::ofstream result_ofs(outputPath("result.txt"));
	std::ofstream error_ofs(outputPath("error.txt"));
	std::ofstream steps_ofs(outputPath("steps.txt"));

	if (!result_ofs || !error_ofs || !steps_ofs) {
		throw std::runtime_error("无法写入 token 文件错误输出文件");
	}

	result_ofs << "token 文件无法分析，SLR(1) 语法分析未执行。\n";

	for (const TokenReadError &error : file_errors) {
		error_ofs << "表达式 " << error.expression_index << "，token 文件行号 "
		          << error.line << ": " << error.message << "\n";
	}

	steps_ofs << "token 文件存在错误，无 SLR(1) 分析步骤。\n";
}

std::filesystem::path
ResultWriter::outputPath(const std::string &filename) const {
	return output_dir_ / filename;
}

void ResultWriter::ensureOutputDirectory() const {
	std::filesystem::create_directories(output_dir_);
}

std::string ResultWriter::diagnosticKindToString(ParseDiagnosticKind kind) {
	switch (kind) {
	case ParseDiagnosticKind::TokenInputError:
		return "token 输入错误";
	case ParseDiagnosticKind::EmptyInput:
		return "空表达式";
	case ParseDiagnosticKind::InputExhausted:
		return "输入提前耗尽";
	case ParseDiagnosticKind::EmptyAction:
		return "ACTION 表项为空";
	case ParseDiagnosticKind::MissingGoto:
		return "GOTO 表项缺失";
	case ParseDiagnosticKind::StackUnderflow:
		return "分析栈下溢";
	}

	return "未知错误";
}

void ResultWriter::writeDiagnostic(std::ostream &os,
                                   const ParseDiagnostic &diagnostic) {
	os << "表达式 " << diagnostic.expression_index << ": "
	   << diagnosticKindToString(diagnostic.kind) << "\n";

	if (diagnostic.state >= 0) {
		os << "当前状态: I" << diagnostic.state << "\n";
	}

	if (!diagnostic.lexeme.empty()) {
		os << "当前词素: " << diagnostic.lexeme << "\n";
	}

	os << "当前终结符: " << toString(diagnostic.terminal) << "\n";

	if (diagnostic.token_source_line > 0) {
		os << "token 文件行号: " << diagnostic.token_source_line << "\n";
	}

	os << "错误说明: " << diagnostic.message << "\n";
}

std::string ResultWriter::actionText(const SLRTable &table, int state,
                                     Terminal terminal) {
	Action action = table.action(state, terminal);

	if (action.kind == ActionKind::Error) {
		return "";
	}

	return toString(action);
}

std::string ResultWriter::gotoText(const SLRTable &table, int state,
                                   NonTerminal non_terminal) {
	std::optional<int> next_state = table.goTo(state, non_terminal);

	if (!next_state.has_value()) {
		return "";
	}

	return std::to_string(*next_state);
}