#pragma once

#include <map>
#include <set>

#include "grammar.h"
#include "terminal.h"

struct FirstLastVTResult {
	std::map<NonTerminal, std::set<Terminal>> first_vt;
	std::map<NonTerminal, std::set<Terminal>> last_vt;
};

class FirstLastVTCalculator {
  public:
	FirstLastVTResult calculate(const Grammar &) const;

  private:
	static void initializeSets(const Grammar &, FirstLastVTResult &);

	static bool addFirstVTByProduction(const Production &, FirstLastVTResult &);
	static bool addLastVTByProduction(const Production &, FirstLastVTResult &);

	static bool propagateFirstVT(const Grammar &, FirstLastVTResult &);
	static bool propagateLastVT(const Grammar &, FirstLastVTResult &);

	static bool addSet(std::set<Terminal> &, const std::set<Terminal> &);

	static bool isTerminal(const Symbol &);
	static bool isNonTerminal(const Symbol &);
};
