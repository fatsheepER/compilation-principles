#include "first_last_vt.h"
#include "grammar.h"
#include <cstddef>

FirstLastVTResult
FirstLastVTCalculator::calculate(const Grammar &grammar) const {
	FirstLastVTResult result;
	initializeSets(grammar, result);

	bool changed = true;

	while (changed) {
		changed = false;

		for (const auto &production : grammar.productions()) {
			// P -> a... || P -> Qa... => a in FirstVT(P)
			if (addFirstVTByProduction(production, result)) {
				changed = true;
			}

			// P -> ...a || P -> ...aQ => a in LastVT(P)
			if (addLastVTByProduction(production, result)) {
				changed = true;
			}
		}

		// P -> Q... => FirstVT(Q) join FirstVT(P)
		if (propagateFirstVT(grammar, result)) {
			changed = true;
		}

		// P -> ...Q => LastVT(Q) join LastVT(P)
		if (propagateLastVT(grammar, result)) {
			changed = true;
		}
	}

	return result;
}

void FirstLastVTCalculator::initializeSets(const Grammar &grammar,
                                           FirstLastVTResult &result) {
	for (const auto non_terminal : grammar.nonTerminals()) {
		result.first_vt[non_terminal];
		result.last_vt[non_terminal];
	}
}

bool FirstLastVTCalculator::addFirstVTByProduction(const Production &production,
                                                   FirstLastVTResult &result) {
	bool changed = false;
	const auto &rhs = production.rhs;

	if (rhs.empty()) {
		return false;
	}

	// P -> a...
	// try insert FirstVT(P) with a
	if (isTerminal(rhs[0])) {
		changed |=
		    result.first_vt[production.lhs].insert(rhs[0].terminal).second;
	}

	// P -> Qa...
	// try insert FirstVT(P) with a
	if (rhs.size() >= 2 && isNonTerminal(rhs[0]) && isTerminal(rhs[1])) {
		changed |=
		    result.first_vt[production.lhs].insert(rhs[1].terminal).second;
	}

	return changed;
}

bool FirstLastVTCalculator::addLastVTByProduction(const Production &production,
                                                  FirstLastVTResult &result) {
	bool changed = false;
	const auto &rhs = production.rhs;

	if (rhs.empty()) {
		return false;
	}

	const std::size_t last = rhs.size() - 1;

	// P -> ...a
	// try insert FirstVT(P) with a
	if (isTerminal(rhs[last])) {
		changed |=
		    result.last_vt[production.lhs].insert(rhs[last].terminal).second;
	}

	// P -> ...aQ
	// try insert FirstVT(P) with a
	if (rhs.size() >= 2 && isNonTerminal(rhs[last]) &&
	    isTerminal(rhs[last - 1])) {
		changed |= result.last_vt[production.lhs]
		               .insert(rhs[last - 1].terminal)
		               .second;
	}

	return changed;
}

bool FirstLastVTCalculator::propagateFirstVT(const Grammar &grammar,
                                             FirstLastVTResult &result) {
	bool changed = false;

	for (const auto &production : grammar.productions()) {
		const auto rhs = production.rhs;

		// P -> Q...
		if (!rhs.empty() && isNonTerminal(rhs[0])) {
			changed |= addSet(result.first_vt[production.lhs],
			                  result.first_vt[rhs[0].non_terminal]);
		}
	}

	return changed;
}

bool FirstLastVTCalculator::propagateLastVT(const Grammar &grammar,
                                            FirstLastVTResult &result) {
	bool changed = false;

	for (const auto &production : grammar.productions()) {
		const auto &rhs = production.rhs;

		// P -> ...Q
		if (!rhs.empty() && isNonTerminal(rhs.back())) {
			changed |= addSet(result.last_vt[production.lhs],
			                  result.last_vt[rhs.back().non_terminal]);
		}
	}

	return changed;
}

bool FirstLastVTCalculator::addSet(std::set<Terminal> &target,
                                   std::set<Terminal> &source) {
	bool changed = false;

	for (const auto terminal : source) {
		changed |= target.insert(terminal).second;
	}

	return changed;
}

bool FirstLastVTCalculator::isTerminal(const Symbol &symbol) {
	return symbol.kind == SymbolKind::Terminal;
}

bool FirstLastVTCalculator::isNonTerminal(const Symbol &symbol) {
	return symbol.kind == SymbolKind::NonTerminal;
}