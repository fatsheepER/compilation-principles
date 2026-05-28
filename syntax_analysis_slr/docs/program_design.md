# SLR(1) 语法分析程序设计

本文档根据 `task.md` 和 `analysis.md`，整理第四次实验 SLR(1) 语法分析程序的结构设计。当前阶段先明确模块边界、核心数据结构、文件职责、执行流程和输出约定，后续实现代码时应以本文档为依据。

本实验继续沿用本仓库已有约定：语法分析阶段不直接扫描原始表达式字符串，而是读取词法分析阶段产生的 token 文件，将 token 归一化为表达式文法中的终结符后再进行 SLR(1) 分析。

---

## 1. 设计目标

本实验要求使用 SLR(1) 分析方法，对给定表达式文法识别的句子进行语法分析，并判断输入符号串是否正确。

作业给定文法为：

```text
S -> E
E -> E + T
E -> T
T -> T * F
T -> F
F -> ( E )
F -> i
```

程序实现时需要在此基础上增加增广开始产生式：

```text
S' -> S
```

本实验的核心目标包括：

1. 表示增广文法、终结符、非终结符和产生式编号。
2. 计算构造 SLR(1) 表所需的 `FOLLOW` 集。
3. 构造 LR(0) 项目集规范族和状态转换关系。
4. 根据 LR(0) 项目集与 `FOLLOW` 集生成 `ACTION` / `GOTO` 表，并检查冲突。
5. 读取词法分析输出中的表达式 token 序列。
6. 使用状态栈和符号栈执行 SLR(1) 总控分析。
7. 输出分析表、项目集、每条表达式的分析结果、错误原因和移进-归约步骤。

本实验不要求表达式求值。即使输入 token 中包含整数常量，也只需要在语法分析层面将它归一化为终结符 `i`。

---

## 2. 程序整体组成

SLR(1) 语法分析程序建议拆分为两部分：

1. `syntax_analysis_slr_lib`：语法分析库，负责文法表示、`FOLLOW` 集计算、LR(0) 自动机构造、SLR(1) 分析表构造、状态驱动分析、诊断信息和结果输出。
2. `syntax_analysis_slr`：可执行程序，负责程序入口、默认路径组织、调用 library 和打印运行摘要。

这样的划分可以让核心算法与命令行入口分离，后续如果需要单独测试 `FOLLOW` 集、项目集、表构造或 parser，只需要调用 library 中的模块。

SLR(1) 模块应复用 `syntax_analysis_common` 中已经存在的输入边界：

```text
Terminal
InputToken
TokenReader
ExpressionInput
TokenReaderResult
```

LR 相关的非终结符、产生式、项目集和分析表不应放入 `syntax_analysis_common`。公共模块只负责 token 输入和终结符归一化，SLR(1) 专属语义留在 `syntax_analysis_slr` 内部。

---

## 3. 建议目录结构

```text
syntax_analysis_slr/
  docs/
    task.md
    analysis.md
    program_design.md

  src/
    main.cpp

    grammar.h
    grammar.cpp

    follow_set.h
    follow_set.cpp

    lr0_automaton.h
    lr0_automaton.cpp

    slr_table.h
    slr_table.cpp

    parser.h
    parser.cpp

    diagnostic.h

    result_writer.h
    result_writer.cpp
```

公共输入模块继续放在：

```text
syntax_analysis_common/
  src/
    terminal.h
    terminal.cpp
    input_token.h
    token_reader.h
    token_reader.cpp
```

后续接入 CMake 时，建议在根目录 `CMakeLists.txt` 中增加：

```cmake
add_subdirectory(syntax_analysis_slr)
```

并在 `syntax_analysis_slr/CMakeLists.txt` 中定义：

```text
syntax_analysis_slr_lib
syntax_analysis_slr
run_syntax_analysis_slr
```

其中 `syntax_analysis_slr_lib` 应链接 `syntax_analysis_common`，`run_syntax_analysis_slr` 的 `WORKING_DIRECTORY` 应设置为 `syntax_analysis_slr`，使程序内部可以继续使用相对路径访问词法分析输出。

---

## 4. 输入与输出约定

### 4.1 输入来源

SLR(1) 语法分析程序默认读取：

```text
../lexical_analysis/output/result.txt
../lexical_analysis/output/error.txt
```

如果 `../lexical_analysis/output/error.txt` 非空，说明词法分析阶段未通过。此时程序应停止逐表达式分析，并在本实验的 `output/error.txt` 中说明词法分析未通过。

### 4.2 token 归一化

`syntax_analysis_common` 中的 `TokenReader` 已经负责将词法 token 归一化为 `Terminal`：

| 词法 token | 种别码 | 语法终结符 |
| --- | --- | --- |
| `TK_IDENTIFIER`，且 lexeme 为 `i` | 0 | `i` |
| `TK_INT_LITERAL` | 1 | `i` |
| `OP_PLUS` | 200 | `+` |
| `OP_MUL` | 202 | `*` |
| `SEP_LPAREN` | 302 | `(` |
| `SEP_RPAREN` | 303 | `)` |
| `SEP_SEMICOLON` | 301 | 当前表达式结束，并追加 `#` |

公共终结符枚举为：

```text
Id      -> i
Plus    -> +
Mul     -> *
LParen  -> (
RParen  -> )
End     -> #
```

其他 token 均不属于本实验表达式文法，应作为 token 输入错误处理。例如其他标识符、`-`、`/`、关键字、赋值号、关系运算符、字符常量、字符串常量等。

### 4.3 多表达式处理

词法 token 中的分号用于切分多条表达式。每条表达式进入 parser 前都应已经带有结束符 `#`。例如：

```text
(i+i)*i;
i+i)*i;
```

会被整理成两条独立的 `ExpressionInput`，分别执行 SLR(1) 分析。

### 4.4 错误位置含义

当前词法输出文件只保存 `(lexeme, token_code)`，不保存原始源程序的行列号。因此 `InputToken.source_index` 表示 token 在 `lexical_analysis/output/result.txt` 中的行号，而不是源程序行号。

输出错误信息时应明确这一点，避免把 token 文件行号误写为源码行号。

### 4.5 建议输出文件

本实验建议输出目录为：

```text
syntax_analysis_slr/output/
```

建议输出文件如下：

| 文件 | 内容 |
| --- | --- |
| `follow_sets.txt` | `FOLLOW(S)`、`FOLLOW(E)`、`FOLLOW(T)`、`FOLLOW(F)` |
| `lr0_item_sets.txt` | LR(0) 项目集规范族和状态转换关系 |
| `slr_table.txt` | `ACTION` / `GOTO` 表和冲突检查结果 |
| `result.txt` | 每条表达式的最终判断结果 |
| `error.txt` | 词法阶段失败、token 错误或 SLR 分析错误的详细信息 |
| `steps.txt` | 每条表达式的状态栈、符号栈、剩余输入和动作 |

---

## 5. Library 模块职责

### 5.1 `grammar.h` / `grammar.cpp`

该模块负责描述 SLR(1) 分析使用的增广文法。

建议定义非终结符：

```text
AugmentedStart  -> S'
S
E
T
F
```

终结符直接使用 `syntax_analysis_common/src/terminal.h` 中的 `Terminal`：

```text
Id, Plus, Mul, LParen, RParen, End
```

建议定义符号结构：

```cpp
enum class SymbolKind {
    Terminal,
    NonTerminal
};

struct Symbol {
    SymbolKind kind;
    Terminal terminal;
    NonTerminal non_terminal;
};
```

建议定义产生式结构：

```cpp
struct Production {
    int id;
    NonTerminal lhs;
    std::vector<Symbol> rhs;
};
```

产生式编号应与 `analysis.md` 保持一致：

| 编号 | 产生式 |
| --- | --- |
| 0 | `S' -> S` |
| 1 | `S -> E` |
| 2 | `E -> E + T` |
| 3 | `E -> T` |
| 4 | `T -> T * F` |
| 5 | `T -> F` |
| 6 | `F -> ( E )` |
| 7 | `F -> i` |

`Grammar` 建议提供以下接口：

```text
productions()
production(id)
startSymbol()
augmentedStartSymbol()
terminals()
nonTerminals()
symbolsForAutomatonExpansion()
productionsFor(lhs)
toString(non_terminal)
toString(symbol)
toString(production)
```

其中 `symbolsForAutomatonExpansion()` 建议返回固定顺序：

```text
S, E, T, F, +, *, (, ), i
```

这样构造出的项目集编号可以稳定匹配 `analysis.md` 中的 `I0` 至 `I12`。

### 5.2 `follow_set.h` / `follow_set.cpp`

该模块负责计算 `FOLLOW` 集。

虽然本实验文法没有空产生式，`FOLLOW` 集也可以手工写出，但实现时仍建议按通用规则迭代计算，以保证程序和实验原理一致。

建议输出结构：

```cpp
struct FollowSetResult {
    std::map<NonTerminal, std::set<Terminal>> first;
    std::map<NonTerminal, std::set<Terminal>> follow;
};
```

主要职责：

1. 根据文法计算 `FIRST` 集，作为计算 `FOLLOW` 的辅助数据。
2. 将 `#` 加入原开始符号 `S` 的 `FOLLOW` 集。
3. 对每条产生式 `A -> α B β`：
   - 将 `FIRST(β)` 中除 `ε` 外的终结符加入 `FOLLOW(B)`。
   - 若 `β` 为空或可推出 `ε`，将 `FOLLOW(A)` 加入 `FOLLOW(B)`。
4. 反复迭代直到所有集合不再变化。
5. 将结果提供给 `SLRTableBuilder` 和 `ResultWriter`。

本实验应得到：

```text
FOLLOW(S) = { # }
FOLLOW(E) = { +, ), # }
FOLLOW(T) = { *, +, ), # }
FOLLOW(F) = { *, +, ), # }
```

### 5.3 `lr0_automaton.h` / `lr0_automaton.cpp`

该模块负责构造 LR(0) 项目集规范族。

建议定义 LR(0) 项目：

```cpp
struct LR0Item {
    int production_id;
    std::size_t dot_position;
};
```

其中 `dot_position` 表示圆点位于产生式右部的位置：

```text
0                     表示 A -> · X Y
rhs.size()            表示 A -> X Y ·
```

建议定义项目集：

```cpp
struct ItemSet {
    int id;
    std::set<LR0Item> items;
};
```

建议定义状态转换：

```text
struct Transition {
    int from_state;
    Symbol symbol;
    int to_state;
};
```

主要职责：

1. 实现 `closure(items)`。
2. 实现 `goTo(item_set, symbol)`。
3. 从 `closure({ S' -> · S })` 开始构造项目集规范族。
4. 保存每个状态的项目集合。
5. 保存状态之间的转换关系。
6. 保证状态编号稳定，便于和 `analysis.md` 中的 SLR 表对照。

实现后应构造出 13 个项目集：

```text
I0, I1, I2, I3, I4, I5, I6, I7, I8, I9, I10, I11, I12
```

其中 `I3` 和 `I10` 是重要检查点：

```text
I3:
E -> T ·
T -> T · * F

I10:
E -> E + T ·
T -> T · * F
```

这两个状态同时包含可归约项目和可移进项目，能够验证 SLR 表是否正确使用 `FOLLOW` 集避免冲突。

### 5.4 `slr_table.h` / `slr_table.cpp`

该模块负责根据 LR(0) 自动机和 `FOLLOW` 集构造 SLR(1) 分析表。

建议定义动作类型：

```cpp
enum class ActionKind {
    Shift,
    Reduce,
    Accept,
    Error
};

struct Action {
    ActionKind kind;
    int next_state;      // Shift 时使用
    int production_id;   // Reduce 时使用
};
```

建议定义分析表：

```cpp
class SLRTable {
public:
    Action action(int state, Terminal terminal) const;
    std::optional<int> goTo(int state, NonTerminal non_terminal) const;
};
```

表构造规则：

1. 若 `goto(Ii, a) = Ij`，且 `a` 是终结符，则填入 `ACTION[i, a] = shift j`。
2. 若 `Ii` 中存在归约项目 `A -> β ·`，且 `A` 不是增广开始符号，则对每个 `a ∈ FOLLOW(A)` 填入 `ACTION[i, a] = reduce A -> β`。
3. 若 `Ii` 中存在 `S' -> S ·`，则填入 `ACTION[i, #] = accept`。
4. 若 `goto(Ii, A) = Ij`，且 `A` 是非终结符，则填入 `GOTO[i, A] = j`。

构造表时必须检测冲突。如果同一表项已经存在不同动作，应记录为：

```text
移进/归约冲突
归约/归约冲突
接受/其他动作冲突
```

对于本实验文法，正确结果应无冲突：

```text
移进/归约冲突：无
归约/归约冲突：无
```

如果构造过程中发现冲突，程序仍可输出已有项目集和冲突信息，但不应继续执行正常的逐表达式分析。

### 5.5 `parser.h` / `parser.cpp`

该模块负责执行 SLR(1) 总控程序。

parser 输入为一条已经由 `TokenReader` 归一化并追加结束符 `#` 的表达式：

```text
ExpressionInput
```

建议定义单步记录：

```cpp
struct ParseStep {
    std::size_t step_index;
    std::string state_stack;
    std::string symbol_stack;
    std::string combined_stack;
    std::string remaining_input;
    std::string action;
};
```

建议定义分析结果：

```cpp
struct ParseResult {
    std::size_t expression_index;
    bool accepted;
    std::vector<ParseStep> steps;
    std::optional<SyntaxDiagnostic> diagnostic;
};
```

SLR(1) 分析过程如下：

1. 初始化状态栈为 `0`。
2. 初始化符号栈为 `#`。
3. 令当前输入符号为表达式 token 序列的第一个终结符。
4. 取状态栈顶 `s` 和当前输入符号 `a`。
5. 查询 `ACTION[s, a]`。
6. 若动作为 `Shift(j)`：
   - 将 `a` 压入符号栈。
   - 将 `j` 压入状态栈。
   - 输入指针前进。
7. 若动作为 `Reduce(k)`：
   - 取编号 `k` 的产生式 `A -> β`。
   - 从符号栈和状态栈中分别弹出 `|β|` 项。
   - 令弹栈后的状态栈顶为 `t`。
   - 查询 `GOTO[t, A]`。
   - 将 `A` 压入符号栈，将 `GOTO[t, A]` 压入状态栈。
   - 不移动输入指针。
8. 若动作为 `Accept`，则当前表达式分析成功。
9. 若动作为 `Error` 或表项为空，则当前表达式分析失败。

步骤记录应在执行动作前生成栈快照，这样 `steps.txt` 中每一行显示的是“根据当前栈和当前输入准备采取的动作”，不是动作执行后的状态。

parser 不应包含针对 `+`、`*`、括号的特殊语法判断。所有移进、归约、接受和出错行为都应由 `ACTION` / `GOTO` 表驱动。

### 5.6 `diagnostic.h`

该文件负责定义语法分析阶段使用的错误结构。诊断模块只负责组织和描述错误，不负责决定 parser 如何动作。

建议定义错误类型：

```cpp
enum class DiagnosticKind {
    LexicalAnalysisFailed,
    TokenReadError,
    TableConflict,
    EmptyAction,
    MissingGoto,
    InputNotConsumed,
    EmptyExpression
};
```

建议定义错误信息结构：

```cpp
struct SyntaxDiagnostic {
    DiagnosticKind kind;
    std::size_t expression_index;
    std::size_t token_source_line;
    std::string lexeme;
    std::string terminal;
    int state;
    std::string message;
};
```

常见错误场景：

1. 词法分析错误文件非空。
2. token 文件中出现无法归一化的 token。
3. SLR 表存在冲突。
4. `ACTION[state, terminal]` 为空。
5. 归约后 `GOTO[state, nonterminal]` 不存在。
6. 表达式缺少分号，导致没有结束符 `#`。
7. 表达式为空。

错误信息中如果出现 `token_source_line`，应说明它来自 `lexical_analysis/output/result.txt` 的 token 行号。

### 5.7 `result_writer.h` / `result_writer.cpp`

该模块负责把静态分析构造结果和逐表达式分析结果写入文件。

主要职责：

1. 创建或清空 `output/` 目录下的结果文件。
2. 输出 `FOLLOW` 集到 `follow_sets.txt`。
3. 输出 LR(0) 项目集和状态转换到 `lr0_item_sets.txt`。
4. 输出 SLR(1) `ACTION` / `GOTO` 表到 `slr_table.txt`。
5. 输出每条表达式的最终判断到 `result.txt`。
6. 输出详细错误到 `error.txt`。
7. 输出每条表达式的分析步骤到 `steps.txt`。

`result.txt` 建议使用简洁格式：

```text
表达式 1: 正确
表达式 2: 错误
```

`steps.txt` 建议使用固定宽度列或清晰分隔符，至少包含：

```text
步骤
状态栈
符号栈
剩余输入
动作
```

对于接受的表达式，最后一步应显示 `acc`。对于出错表达式，最后一步应显示当前查表失败或归约失败的原因。

---

## 6. Executable 组成

`src/main.cpp` 应保持简单，只负责程序入口和流程编排。

主要职责：

1. 确定默认输入路径：

```text
../lexical_analysis/output/result.txt
../lexical_analysis/output/error.txt
```

2. 确定默认输出目录：

```text
output/
```

3. 构造 `Grammar`。
4. 调用 `FollowSetCalculator` 计算 `FIRST` / `FOLLOW` 集。
5. 调用 `LR0AutomatonBuilder` 构造项目集规范族。
6. 调用 `SLRTableBuilder` 构造分析表并检查冲突。
7. 调用 `ResultWriter` 输出静态构造结果。
8. 调用 `TokenReader` 读取词法分析结果。
9. 若词法阶段失败或分析表存在冲突，则输出错误并结束。
10. 对每条 `ExpressionInput` 调用 `Parser`。
11. 调用 `ResultWriter` 输出最终结果、错误详情和分析步骤。
12. 在控制台打印运行摘要，例如：

```text
SLR(1) 语法分析完成
表达式总数: 2
正确: 1
错误: 1
详细结果见 output/
```

`main.cpp` 不应直接实现 `closure`、`goto`、表构造或具体移进-归约逻辑。这些逻辑应留在对应 library 模块中。

---

## 7. 数据流与执行流程

整体数据流如下：

```text
Grammar
  -> FollowSetCalculator
  -> LR0AutomatonBuilder
  -> SLRTableBuilder
  -> Parser
  -> ResultWriter

lexical_analysis/output/result.txt
lexical_analysis/output/error.txt
  -> TokenReader
  -> ExpressionInput
  -> Parser
  -> ParseResult
  -> ResultWriter
```

更具体的执行步骤为：

1. 构造增广文法。
2. 计算并输出 `FOLLOW` 集。
3. 构造并输出 LR(0) 项目集规范族。
4. 构造并输出 `ACTION` / `GOTO` 表。
5. 检查 SLR 表是否存在冲突。
6. 读取词法分析输出。
7. 检查 token 文件级错误和表达式级 token 错误。
8. 对每条 token 正常的表达式执行 SLR(1) 分析。
9. 将 parser 的步骤和结论统一交给 `ResultWriter`。

如果前置阶段失败，流程应尽量输出已经能确定的结果。例如，词法阶段失败时，仍然可以输出文法、`FOLLOW` 集、项目集和 SLR 表；但不应继续逐表达式分析。

---

## 8. 关键实现约束

### 8.1 SLR 与算符优先的区别

本实验不能沿用算符优先实验的 `N` 占位符归约策略。SLR(1) 归约必须严格按照产生式编号、右部长度和左部非终结符执行。

例如：

```text
T -> F
E -> T
S -> E
```

这三次归约在算符优先实验中可能都可抽象为 `N`，但在 SLR(1) 中必须保留具体左部，因为归约后需要用左部非终结符查 `GOTO` 表。

### 8.2 表驱动优先

parser 中不应写类似“如果当前输入是 `*` 就优先处理乘法”的分支。乘法优先级已经体现在项目集和 SLR 表中，例如 `I3` 在 `*` 列是移进，在 `+`、`)`、`#` 列是按 `E -> T` 归约。

### 8.3 状态编号稳定性

为了便于与 `analysis.md` 和实验报告对照，项目集构造时应使用稳定的符号遍历顺序。建议扩展状态时使用：

```text
S, E, T, F, +, *, (, ), i
```

如果后续实现使用其他顺序，只要表语义一致也可以通过测试，但文档、输出表和报告中的状态编号会更难对照。

### 8.4 诊断职责边界

parser 负责发现错误，例如 `ACTION` 为空或 `GOTO` 缺失；`diagnostic.h` 只负责描述错误结构和格式化信息。不要把 parser 的决策逻辑放进诊断模块。

### 8.5 输出用于报告

本实验后续很可能需要撰写实验报告，因此输出文件应尽量稳定、可读，并保留中间构造结果。至少要能从输出中直接引用：

```text
FOLLOW 集
LR(0) 项目集
ACTION / GOTO 表
移进-归约步骤
正确/错误表达式样例
```

---

## 9. 初步实现顺序

建议按以下顺序实现：

1. 建立 `syntax_analysis_slr` 目录、CMake 目标和空的 library/executable。
2. 实现 `grammar`，先能打印产生式列表。
3. 实现 `follow_set`，验证输出是否为：

```text
FOLLOW(S) = { # }
FOLLOW(E) = { +, ), # }
FOLLOW(T) = { *, +, ), # }
FOLLOW(F) = { *, +, ), # }
```

4. 实现 `lr0_automaton`，验证项目集数量为 13，并检查 `I3`、`I10`。
5. 实现 `slr_table`，验证无冲突，且表项与 `analysis.md` 一致。
6. 实现 `parser`，先用手工构造的 token 序列测试 `(i+i)*i`。
7. 接入 `TokenReader`，读取真实词法分析输出。
8. 实现 `result_writer`，输出所有中间结果和最终结果。
9. 补充错误用例，验证 `error.txt` 和 `steps.txt` 的错误步骤。

---

## 10. 验证计划

实现完成后至少使用以下输入类别验证：

| 类别 | 示例 | 预期 |
| --- | --- | --- |
| 单个操作数 | `i;`、`1;` | 正确 |
| 加法与乘法 | `i+i*i;`、`1+2*3;` | 正确 |
| 括号表达式 | `(i+i)*i;` | 正确，步骤包含括号归约 |
| 多余右括号 | `i+i)*i;` | 错误，`ACTION` 表项为空 |
| 连续运算符 | `i+*i;` | 错误 |
| 缺少右括号 | `(i+i;` | 错误 |
| 文法外 token | `i-i;`、`x+i;` | token 归一化阶段报错 |
| 缺少分号 | `i+i` | token 读取阶段报错 |

验证时不只检查最终结果，还应检查：

1. `follow_sets.txt` 是否与文档一致。
2. `lr0_item_sets.txt` 是否包含 13 个项目集。
3. `slr_table.txt` 是否显示无冲突。
4. `(i+i)*i` 的 `steps.txt` 是否最终到达 `acc`。
5. 错误表达式是否在 `error.txt` 中给出状态、输入符号和原因。

---

## 11. 本阶段结论

SLR(1) 语法分析程序应围绕“文法 -> FOLLOW 集 -> LR(0) 自动机 -> SLR 表 -> 表驱动 parser -> 结果输出”这一流水线组织。`syntax_analysis_common` 继续作为词法 token 到表达式终结符的公共输入边界，而所有 LR 专属结构都保留在 `syntax_analysis_slr` 内。

按照本设计实现后，程序既能满足作业中“构建分析表”和“利用状态栈、符号栈分析输入串”的要求，也能输出后续实验报告需要引用的中间结果和分析过程。
