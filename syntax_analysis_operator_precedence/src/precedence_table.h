#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "first_last_vt.h"
#include "grammar.h"
#include "terminal.h"

enum class PrecedenceRelation {
	Less,    // <
	Equal,   // =
	Greater, // >
};

struct PrecedenceConflict {
	Terminal left = Terminal::End;
	Terminal right = Terminal::End;
	PrecedenceRelation existing = PrecedenceRelation::Equal;
	PrecedenceRelation incoming = PrecedenceRelation::Equal;
	std::string reason;
};

class PrecedenceTable {
  public:
	std::optional<PrecedenceRelation> lookup(Terminal left,
	                                         Terminal right) const;

	bool set(Terminal left, Terminal right, PrecedenceRelation relation);

	const std::map<std::pair<Terminal, Terminal>, PrecedenceRelation> &
	entries() const;

  private:
	std::map<std::pair<Terminal, Terminal>, PrecedenceRelation> table_;
};

struct PrecedenceTableBuildResult {
	PrecedenceTable table;
	std::vector<PrecedenceConflict> conflicts;

	bool success() const;
};

class PrecedenceTableBuilder {
  public:
	PrecedenceTableBuildResult
	build(const Grammar &grammar, const FirstLastVTResult &first_last_vt) const;

  private:
	static void
	addRelationsFromProduction(const Production &production,
	                           const FirstLastVTResult &first_last_vt,
	                           PrecedenceTableBuildResult &result);

	static void addEndMarkerRelations(const Grammar &grammar,
	                                  const FirstLastVTResult &first_last_vt,
	                                  PrecedenceTableBuildResult &result);

	static void addRelation(PrecedenceTableBuildResult &result, Terminal left,
	                        Terminal right, PrecedenceRelation relation,
	                        const std::string &reason);

	static bool isTerminal(const Symbol &symbol);
	static bool isNonTerminal(const Symbol &symbol);
};

std::string toString(PrecedenceRelation relation);
std::string relationCell(const std::optional<PrecedenceRelation> &relation);