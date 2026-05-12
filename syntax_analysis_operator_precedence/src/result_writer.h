#pragma once

#include <cstddef>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "first_last_vt.h"
#include "parser.h"
#include "precedence_table.h"
#include "token_reader.h"

struct OperatorExpressionReport {
	std::size_t index = 0;
	std::vector<InputToken> tokens;
	std::vector<TokenReadError> input_errors;

	bool parser_ran = false;
	OperatorPrecedenceParseResult parse_result;
};

struct OperatorPrecedenceResultWriteSummary {
	std::size_t total = 0;
	std::size_t accepted = 0;
	std::size_t rejected = 0;
};

class OperatorPrecedenceResultWriter {
  public:
	OperatorPrecedenceResultWriteSummary
	write(const std::string &output_dir, const Grammar &grammar,
	      const FirstLastVTResult &first_last_vt,
	      const PrecedenceTableBuildResult &table_result,
	      const TokenReaderResult &read_result,
	      const std::vector<OperatorExpressionReport> &reports) const;

  private:
	static void writeFirstLastVT(std::ostream &os, const Grammar &grammar,
	                             const FirstLastVTResult &first_last_vt);

	static void writePrecedenceTable(std::ostream &os, const Grammar &grammar,
	                                 const PrecedenceTable &table);

	static void
	writeTableConflicts(std::ostream &os,
	                    const std::vector<PrecedenceConflict> &conflicts);

	static void
	writeSteps(std::ostream &os,
	           const std::vector<OperatorExpressionReport> &reports);

	static void writeReadError(std::ostream &os, const TokenReadError &error);

	static std::string terminalSetToString(const std::set<Terminal> &terminals);

	static std::string
	tokenSequenceToString(const std::vector<InputToken> &tokens);
};