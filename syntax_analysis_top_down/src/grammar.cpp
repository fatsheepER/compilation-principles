#include "grammar.h"

#include <sstream>

Symbol Symbol::terminalSymbol(Terminal terminal) {
	Symbol symbol;
	symbol.kind = SymbolKind::Terminal;
	symbol.terminal = terminal;
	return symbol;
}
