#include "precedence_table.h"
#include "first_last_vt.h"
#include "grammar.h"
#include "terminal.h"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>

std::optional<PrecedenceRelation>
PrecedenceTable::lookup(Terminal left, Terminal right) const {
	const auto it = table_.find(std::make_pair(left, right));

	if (it == table_.end()) {
		return std::nullopt;
	}

	return it->second;
}

bool PrecedenceTable::set(Terminal left, Terminal right,
                          PrecedenceRelation relation) {
	const auto key = std::make_pair(left, right);
	const auto it = table_.find(key);

	if (it == table_.end()) {
		table_[key] = relation;
		return true;
	}

	// false => conflict
	return it->second == relation;
}

const std::map<std::pair<Terminal, Terminal>, PrecedenceRelation> &
PrecedenceTable::entries() const {
	return table_;
}

bool PrecedenceTableBuildResult::success() const { return conflicts.empty(); }

PrecedenceTableBuildResult
PrecedenceTableBuilder::build(const Grammar &grammar,
                              const FirstLastVTResult &first_last_vt) const {
	PrecedenceTableBuildResult result;

	for (const auto &production : grammar.productions()) {
		addRelationsFromProduction(production, first_last_vt, result);
	}

	addEndMarkerRelations(grammar, first_last_vt, result);

	return result;
}

void PrecedenceTableBuilder::addRelationsFromProduction(
    const Production &production, const FirstLastVTResult &first_last_vt,
    PrecedenceTableBuildResult &result) {
	const auto &rhs = production.rhs;

	for (std::size_t i = 0; i < rhs.size(); ++i) {
		// ... a b ...
		// => a = b
		if (i + 1 < rhs.size() && isTerminal(rhs[i]) &&
		    isTerminal(rhs[i + 1])) {
			addRelation(result, rhs[i].terminal, rhs[i + 1].terminal,
			            PrecedenceRelation::Equal,
			            "产生式中出现相邻终结符: " + toString(production));
		}

		// ... a Q b ...
		// => a = b
		if (i + 2 < rhs.size() && isTerminal(rhs[i]) &&
		    isNonTerminal(rhs[i + 1]) && isTerminal(rhs[i + 2])) {
			addRelation(result, rhs[i].terminal, rhs[i + 2].terminal,
			            PrecedenceRelation::Equal,
			            "产生式中出现 aQb 形式: " + toString(production));
		}

		// ... a Q ...
		// => a < FirstVT(Q)
		if (i + 1 < rhs.size() && isTerminal(rhs[i]) &&
		    isNonTerminal(rhs[i + 1])) {
			const auto nt = rhs[i + 1].non_terminal;
			const auto first_it = first_last_vt.first_vt.find(nt);

			if (first_it != first_last_vt.first_vt.end()) {
				for (const auto terminal : first_it->second) {
					addRelation(result, rhs[i].terminal, terminal,
					            PrecedenceRelation::Less,
					            "产生式中出现 aQ: " + toString(production));
				}
			}
		}

		// ... Q a ...
		if (i + 1 < rhs.size() && isNonTerminal(rhs[i]) &&
		    isTerminal(rhs[i + 1])) {
			const auto nt = rhs[i].non_terminal;
			const auto last_it = first_last_vt.last_vt.find(nt);

			if (last_it != first_last_vt.last_vt.end()) {
				for (const auto terminal : last_it->second) {
					addRelation(result, terminal, rhs[i + 1].terminal,
					            PrecedenceRelation::Greater,
					            "产生式中出现 Qa: " + toString(production));
				}
			}
		}
	}
}

void PrecedenceTableBuilder::addEndMarkerRelations(
    const Grammar &grammar, const FirstLastVTResult &first_last_vt,
    PrecedenceTableBuildResult &result) {
	const auto start = grammar.startSymbol();

	// # < FirstVT(E)
	const auto first_it = first_last_vt.first_vt.find(start);
	if (first_it != first_last_vt.first_vt.end()) {
		for (const auto terminal : first_it->second) {
			addRelation(result, Terminal::End, terminal,
			            PrecedenceRelation::Less,
			            "结束符号小于开始符号 FirstVT 集的任何元素");
		}
	}

	// LastVT(E) > #
	const auto last_it = first_last_vt.last_vt.find(start);
	if (last_it != first_last_vt.last_vt.end()) {
		for (const auto terminal : last_it->second) {
			addRelation(result, terminal, Terminal::End,
			            PrecedenceRelation::Greater,
			            "开始符号 LastVT 集任何元素大于 #");
		}
	}

	// # = #
	addRelation(result, Terminal::End, Terminal::End, PrecedenceRelation::Equal,
	            "结束符号等于结束符号");
}

void PrecedenceTableBuilder::addRelation(PrecedenceTableBuildResult &result,
                                         Terminal left, Terminal right,
                                         PrecedenceRelation relation,
                                         const std::string &reason) {
	const auto existing = result.table.lookup(left, right);

	if (existing.has_value() && existing.value() != relation) {
		result.conflicts.push_back({
		    left,
		    right,
		    existing.value(),
		    relation,
		    reason,
		});
		return;
	}

	result.table.set(left, right, relation);
}

bool PrecedenceTableBuilder::isTerminal(const Symbol &symbol) {
	return symbol.kind == SymbolKind::Terminal;
}

bool PrecedenceTableBuilder::isNonTerminal(const Symbol &symbol) {
	return symbol.kind == SymbolKind::NonTerminal;
}

std::string toString(PrecedenceRelation relation) {
	switch (relation) {
	case PrecedenceRelation::Less:
		return "<";
	case PrecedenceRelation::Equal:
		return "=";
	case PrecedenceRelation::Greater:
		return ">";
	}

	return "?";
}

std::string relationCell(const std::optional<PrecedenceRelation> &relation) {
	if (!relation.has_value()) {
		return "";
	}
	return toString(relation.value());
}