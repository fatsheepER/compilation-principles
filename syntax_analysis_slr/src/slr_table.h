// 构造 SLR(1) ACTION / GOTO 分析表

#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "follow_set.h"
#include "grammar.h"
#include "lr0_automaton.h"
#include "terminal.h"

// Action

enum class ActionKind { Shift, Reduce, Accept, Error };

struct Action {
	ActionKind kind = ActionKind::Error;
	int next_state = -1;    // for Shift
	int production_id = -1; // for Reduce
};

bool operator==(const Action &lhs, const Action &rhs);
bool operator!=(const Action &lhs, const Action &rhs);

std::string toString(const Action &action);

// SLR(1) conflict

struct SLRConflict {
	int state = 0;
	Terminal terminal = Terminal::End;
	Action existing_action;
	Action new_action;
};

std::string toString(const SLRConflict &conflict);

// SLR(1) table

class SLRTable {
  public:
	// ACTION 表使用 (状态, 终结符) 作为 key -> Action 表项
	using ActionKey = std::pair<int, Terminal>;
	// GOTO 表使用 (状态, 非终结符) 作为 key -> Int 下一状态
	using GotoKey = std::pair<int, NonTerminal>;

	// 查 Action 表
	Action action(int state, Terminal terminal) const;
	// 查 GoTo 表
	std::optional<int> goTo(int state, NonTerminal non_terminal) const;

	const std::map<ActionKey, Action> &actions() const;
	const std::map<GotoKey, int> &gotos() const;

	void setAction(int state, Terminal terminal, const Action &action);
	void setGoto(int state, NonTerminal non_terminal, int next_state);

  private:
	std::map<ActionKey, Action> actions_;
	std::map<GotoKey, int> gotos_;
};

// SLR(1) table builder

// SLR(1) 表构造结果
struct SLRTableBuildResult {
	SLRTable table;
	std::vector<SLRConflict> conflicts;
};

class SLRTableBuilder {
  public:
	SLRTableBuilder(const Grammar &grammar, const LR0Automaton &automaton,
	                const FollowSetResult &follow_result);

	// 根据 LR(0) 自动机和 Follow 集构造 SLR(1) 表
	SLRTableBuildResult build() const;

  private:
	const Grammar &grammar_;
	const LR0Automaton &automaton_;
	const FollowSetResult &follow_result_;

	// 判断是否是可归约/接收项目，形如 A -> X Y .
	bool isCompleteItem(const LR0Item &item) const;

	// 写入 ACTION 表项
	// 如果已存在 ACTION 则记录冲突
	void putAction(SLRTableBuildResult &result, int state, Terminal terminal,
	               const Action &action) const;
};