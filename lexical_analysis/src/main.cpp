#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lexer.h"
#include "tables.h"
#include "token.h"

namespace fs = std::filesystem;

std::string readFile(const std::string &path) {
	std::ifstream ifs(path);
	if (!ifs) {
		throw std::runtime_error("failed to open input file: " + path);
	}

	std::ostringstream oss;
	oss << ifs.rdbuf();
	return oss.str();
}

void writeTokens(const std::string &path, const std::vector<Token> &tokens) {
	std::ofstream ofs(path);
	if (!ofs) {
		throw std::runtime_error("failed to open output file: " + path);
	}

	for (const auto &tk : tokens) {
		if (tk.type == TokenType::TK_EOF) { break; }
		ofs << "(" << tk.lexeme << ", " << tokenCode(tk.type) << ")\n";
	}
}

void writeErrors(const std::string &path, const std::vector<LexError> &errors) {
	std::ofstream ofs(path);
	if (!ofs) {
		throw std::runtime_error("failed to open error file: " + path);
	}

	for (const auto &err : errors) {
		ofs << "[line " << err.line << ", col " << err.column << "] "
		    << err.message;

		if (!err.fragment.empty()) { ofs << " : " << err.fragment; }
		ofs << "\n";
	}
}

void writeIdentifierTable(const std::string &path,
                          const IdentifierTable &table) {
	std::ofstream ofs(path);
	if (!ofs) {
		throw std::runtime_error("failed to open identifier table file: " +
		                         path);
	}

	for (const auto &entry : table.entries()) {
		ofs << entry.index << " " << entry.name << "\n";
	}
}

void writeConstantTable(const std::string &path, const ConstantTable &table) {
	std::ofstream ofs(path);
	if (!ofs) {
		throw std::runtime_error("failed to open constant table file: " + path);
	}

	for (const auto &entry : table.entries()) {
		ofs << entry.index << " " << entry.literal << " "
		    << tokenCode(entry.type) << "\n";
	}
}

fs::path resolveInputPath(int argc, char *argv[]) {
	const fs::path inputDir = "input";

	if (argc <= 1) { return inputDir / "s.txt"; }

	fs::path argPath(argv[1]);
	if (argPath.is_absolute()) { return argPath; }
	return inputDir / argPath;
}

int main(int argc, char *argv[]) {
	try {
		const fs::path inputPath = resolveInputPath(argc, argv);
		const fs::path outputDir = "output";
		const fs::path resultPath = outputDir / "result.txt";
		const fs::path errorPath = outputDir / "error.txt";
		const fs::path identifierTablePath = outputDir / "identifier_table.txt";
		const fs::path constantTablePath = outputDir / "constant_table.txt";

		fs::create_directories(outputDir);

		std::string source = readFile(inputPath.string());

		Lexer lexer(source);
		lexer.tokenize();

		writeTokens(resultPath.string(), lexer.getTokens());
		writeErrors(errorPath.string(), lexer.getErrors());
		writeIdentifierTable(identifierTablePath.string(),
		                     lexer.getIdentifierTable());
		writeConstantTable(constantTablePath.string(),
		                   lexer.getConstantTable());

		std::cout << "词法分析完毕.\n";
		std::cout << "输入文件: " << inputPath << "\n";
		std::cout << "分析结果写入 " << resultPath << "\n";
		std::cout << "错误信息写入 " << errorPath << "\n";
		return 0;
	} catch (const std::exception &ex) {
		std::cerr << "Fatal error: " << ex.what() << '\n';
		return 1;
	}
}
