#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "follow_set.h"
#include "grammar.h"
#include "lr0_automaton.h"
#include "parser.h"
#include "slr_table.h"
#include "token_reader.h"

class ResultWriter {
  public:
	explicit ResultWriter(std::filesystem::path output_dir);

	void writeStaticOutputs(const Grammar &grammar,
	                        const FollowSetResult &follow_result,
	                        const LR0Automaton &automaton,
	                        const SLRTableBuildResult &table_result) const;

	void writeParseOutputs(const std::vector<ParseResult> &parse_results) const;

	void writeLexicalFailure(const std::string &lexical_error_text) const;

	void
	writeTokenFileErrors(const std::vector<TokenReadError> &file_errors) const;

  private:
	std::filesystem::path output_dir_;

	std::filesystem::path outputPath(const std::string &filename) const;

	void ensureOutputDirectory() const;

	static std::string diagnosticKindToString(ParseDiagnosticKind kind);

	static void writeDiagnostic(std::ostream &os,
	                            const ParseDiagnostic &diagnostic);

	static std::string actionText(const SLRTable &table, int state,
	                              Terminal terminal);

	static std::string gotoText(const SLRTable &table, int state,
	                            NonTerminal non_terminal);
};