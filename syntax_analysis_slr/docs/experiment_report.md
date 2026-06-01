# SLR(1) 语法分析程序实验报告

## 一、实验目的

1. 根据 SLR(1) 分析方法，对输入表达式进行语法分析，使程序能够判断输入符号串是否为给定文法识别的句子。
2. 通过实现 LR(0) 项目集规范族、FOLLOW 集和 SLR(1) 分析表，理解 LR 系列自下而上分析方法的基本思想。
3. 掌握 SLR(1) 分析器中 ACTION 表、GOTO 表、状态栈和符号栈之间的配合关系。
4. 在已有词法分析程序输出的基础上完成语法分析阶段，继续通过 token 文件实现词法分析与语法分析之间的数据衔接。
5. 通过输出 FIRST/FOLLOW 集、LR(0) 项目集、SLR(1) 分析表、分析步骤和错误信息，验证分析表构造结果和语法分析过程是否正确。

## 二、实验原理

### 1. 使用的文法

本实验使用的表达式文法如下：

```text
S -> E
E -> E + T
E -> T
T -> T * F
T -> F
F -> ( E )
F -> i
```

其中，S 为原开始符号，E 表示表达式，T 表示项，F 表示因子。终结符集合为：

```text
{ i, +, *, (, ), # }
```

`#` 为输入结束符，不属于原文法普通终结符，只用于分析表接受动作和输入结束判断。非终结符集合为：

```text
{ S, E, T, F }
```

为了构造 LR 项目集，程序在原文法基础上增加增广开始符号 S'：

```text
S' -> S
```

完整的增广文法产生式编号如下：

| 编号 | 产生式 |
| --- | --- |
| 0 | S' -> S |
| 1 | S -> E |
| 2 | E -> E + T |
| 3 | E -> T |
| 4 | T -> T * F |
| 5 | T -> F |
| 6 | F -> ( E ) |
| 7 | F -> i |

其中，0 号产生式只用于构造接受项目，实际归约输出中使用 r1 至 r7。程序实际分析时，会将词法分析输出中的标识符 i 和整数常量统一映射为文法终结符 i。

### 2. SLR(1) 分析的基本思想

SLR(1) 分析是一种表驱动的自下而上语法分析方法。它从输入符号串出发，通过状态栈、符号栈和分析表不断执行移进、归约、接受或报错动作，最终判断输入串是否能够归约为开始符号。

SLR(1) 分析器的核心由两张表组成：

```text
ACTION[state, terminal]   决定在某状态遇到某终结符时执行移进、归约、接受或报错
GOTO[state, nonterminal]  决定归约出某非终结符后应转入哪个状态
```

分析时，程序查看状态栈栈顶状态 s 和当前输入终结符 a：

1. 若 ACTION[s, a] = sj，则移进当前输入符号 a，并将状态 j 压入状态栈。
2. 若 ACTION[s, a] = rk，则按编号 k 的产生式 A -> β 进行归约，弹出 |β| 个符号和状态，再根据 GOTO[t, A] 转入新状态。
3. 若 ACTION[s, a] = acc，则说明输入符号串分析成功。
4. 若 ACTION[s, a] 为空，则说明当前状态下不能接受该输入符号，语法分析出错。

SLR(1) 与算符优先分析同属移进-归约分析，但二者的决策依据不同。算符优先分析依据栈顶终结符和当前输入终结符之间的优先关系决定移进或归约；SLR(1) 分析依据 LR(0) 状态和 FOLLOW 集构造出的 ACTION/GOTO 表进行决策。归约时，SLR(1) 必须保留具体的非终结符 S、E、T、F，不能统一折叠为占位符。

### 3. FIRST 集和 FOLLOW 集

SLR(1) 表构造需要 FOLLOW 集来限制归约动作。若某状态中存在归约项目：

```text
A -> β ·
```

则只对 FOLLOW(A) 中的终结符 a 填入：

```text
ACTION[state, a] = reduce A -> β
```

这样可以避免 LR(0) 在所有终结符上盲目归约，从而减少移进/归约冲突。

程序先按通用不动点方法计算 FIRST 集，再计算 FOLLOW 集。FIRST 集用于处理产生式中非终结符后面还有符号的情况；FOLLOW 集用于确定归约动作可以出现在哪些输入符号列中。

本实验文法没有空产生式，最终得到的 FIRST 集为：

```text
FIRST(S) = { i, ( }
FIRST(E) = { i, ( }
FIRST(T) = { i, ( }
FIRST(F) = { i, ( }
```

最终得到的 FOLLOW 集为：

```text
FOLLOW(S) = { # }
FOLLOW(E) = { +, ), # }
FOLLOW(T) = { +, *, ), # }
FOLLOW(F) = { +, *, ), # }
```

例如，项目 E -> T · 只在 +、)、# 上归约为 E，而不会在 * 上归约。这样当 T 后面遇到 * 时，分析器可以继续移进乘号，从而正确体现乘法优先级。

### 4. LR(0) 项目集规范族

LR(0) 项目是在产生式右部加入圆点形成的项目。圆点左边表示已经识别的部分，圆点右边表示下一步希望识别的文法符号。例如：

```text
E -> · E + T
E -> E · + T
E -> E + · T
E -> E + T ·
```

构造 LR(0) 项目集规范族需要两个基本运算：

1. closure(I)：如果项目集中有 A -> α · B β，且 B 是非终结符，则加入 B 的所有产生式 B -> · γ，并重复直到不再新增项目。
2. goto(I, X)：找出 I 中圆点后为 X 的项目，将圆点越过 X，再对所得项目集执行 closure。

程序从初始项目：

```text
S' -> · S
```

开始构造闭包，然后按照固定符号顺序不断求 goto，最终得到 13 个 LR(0) 项目集 I0 至 I12。

### 5. SLR(1) 分析表构造

在得到 LR(0) 项目集和 FOLLOW 集后，程序按照以下规则构造 SLR(1) 分析表：

1. 若 goto(Ii, a) = Ij，且 a 是终结符，则填入 ACTION[i, a] = sj。
2. 若 goto(Ii, A) = Ij，且 A 是非终结符，则填入 GOTO[i, A] = j。
3. 若 Ii 中存在归约项目 A -> β ·，且 A 不是增广开始符号，则对每个 a in FOLLOW(A)，填入 ACTION[i, a] = rk。
4. 若 Ii 中存在 S' -> S ·，则填入 ACTION[i, #] = acc。
5. 未填入的 ACTION 表项为空，分析时遇到空表项即报告语法错误。

构造表时，如果同一个 ACTION 单元已经存在动作，而新规则又试图加入不同动作，则说明出现 SLR(1) 表冲突。程序会记录冲突信息，并在存在冲突时停止后续输入表达式分析。本实验文法构造出的表没有移进/归约冲突，也没有归约/归约冲突。

## 三、实验完成情况

### 1. 功能完成情况

本实验程序已经完成以下功能：

1. 固定定义实验所需表达式文法，并在原开始符号 S 之前增加增广开始符号 S'。
2. 使用编号 0 至 7 管理增广文法中的所有产生式。
3. 自动计算文法中每个普通非终结符的 FIRST 集和 FOLLOW 集。
4. 自动构造 LR(0) 项目集规范族和状态转换关系。
5. 根据 LR(0) 项目集和 FOLLOW 集自动生成 SLR(1) ACTION/GOTO 分析表。
6. 在分析表构造阶段检查 ACTION 表冲突。
7. 复用公共 TokenReader 模块读取词法分析程序生成的 token 文件。
8. 按分号将 token 文件切分为多条表达式，并为每条表达式补充结束符 #。
9. 将词法 token 映射到本实验文法终结符，包括将整数常量映射为 i。
10. 对每条表达式执行 SLR(1) 表驱动分析，输出正确或错误结论。
11. 输出完整分析步骤，包括状态栈、符号栈、剩余输入和当前动作。
12. 对错误表达式输出错误类型、当前状态、当前输入终结符和 token 文件行号。
13. 输出 FIRST/FOLLOW 集、LR(0) 项目集、状态转换、SLR(1) 分析表、结果文件、错误文件和步骤文件。

程序目前支持的表达式符号范围为：

```text
i, 整数常量, +, *, (, ), ;
```

其中分号 ; 不属于表达式文法本身，而是作为输入文件中多条表达式的分隔符。

### 2. 程序流程图

以下位置预留给已经绘制好的八个流程图：

> 【流程图 1 占位：SLR(1) 语法分析程序总体流程图】

> 【流程图 2 占位：TokenReader 读取词法分析输出并切分表达式流程图】

> 【流程图 3 占位：FIRST/FOLLOW 集计算流程图】

> 【流程图 4 占位：LR(0) 项目集 closure 和 goto 构造流程图】

> 【流程图 5 占位：SLR(1) ACTION/GOTO 分析表生成流程图】

> 【流程图 6 占位：单条表达式 SLR(1) 分析总控流程图】

> 【流程图 7 占位：归约动作与 GOTO 转移流程图】

> 【流程图 8 占位：结果文件、步骤文件和错误文件输出流程图】

### 3. 工程项目组织与各模块说明

本实验位于工程目录 syntax_analysis_slr/ 下，同时复用了公共模块 syntax_analysis_common/。项目主要结构如下：

```text
syntax_analysis_slr/
├── CMakeLists.txt
├── docs/
│   ├── task.md
│   ├── analysis.md
│   ├── program_design.md
│   └── experiment_report.md
├── output/
│   ├── result.txt
│   ├── error.txt
│   ├── follow_sets.txt
│   ├── lr0_item_sets.txt
│   ├── slr_table.txt
│   └── steps.txt
└── src/
    ├── grammar.h / grammar.cpp
    ├── follow_set.h / follow_set.cpp
    ├── lr0_automaton.h / lr0_automaton.cpp
    ├── slr_table.h / slr_table.cpp
    ├── parser.h / parser.cpp
    ├── result_writer.h / result_writer.cpp
    └── main.cpp
```

各模块职责如下：

grammar.h / grammar.cpp：定义 SLR(1) 分析使用的文法数据结构，包括 NonTerminal、Symbol、Production 和 Grammar，并在 Grammar 构造函数中初始化增广文法。

follow_set.h / follow_set.cpp：定义 FirstSet、FollowSetResult 和 FollowSetCalculator，负责根据文法自动计算 FIRST 集和 FOLLOW 集。

lr0_automaton.h / lr0_automaton.cpp：定义 LR0Item、ItemSet、Transition 和 LR0Automaton，负责实现 closure、goto，并构造 LR(0) 项目集规范族。

slr_table.h / slr_table.cpp：定义 Action、SLRTable、SLRConflict 和 SLRTableBuilder，负责根据 LR(0) 自动机和 FOLLOW 集生成 ACTION/GOTO 表，并记录冲突。

parser.h / parser.cpp：实现 SLR(1) 表驱动分析算法。该模块维护状态栈和符号栈，根据 ACTION/GOTO 表执行移进、归约、接受和报错动作，并记录每一步分析过程。

result_writer.h / result_writer.cpp：负责将实验结果写入 output/ 目录，包括分析结论、错误信息、FIRST/FOLLOW 集、LR(0) 项目集、SLR(1) 分析表和步骤表。

main.cpp：程序入口，负责串联完整流程：初始化文法、计算 FIRST/FOLLOW 集、构造 LR(0) 自动机、构造 SLR 表、读取 token 文件、逐条表达式分析并输出结果。

syntax_analysis_common/：公共模块，提供 Terminal、InputToken、TokenReader 等类型和工具函数。本实验复用 token 读取与终结符归一化，不复用其他语法分析实验中的分析器。

CMake 中定义了三个相关目标：

```text
syntax_analysis_common
syntax_analysis_slr_lib
syntax_analysis_slr
```

此外还定义了运行目标：

```text
run_syntax_analysis_slr
```

程序当前默认使用以工程根目录为基准的输入输出路径：

```text
./lexical_analysis/output/result.txt
./lexical_analysis/output/error.txt
./syntax_analysis_slr/output/
```

因此从工程根目录运行可执行程序时，会读取词法分析输出并将本实验结果写入 syntax_analysis_slr/output/。

## 四、核心算法和代码

本实验程序复用了第 2 次实验设计的公共 TokenReader 模块，并使用与其他语法分析实验一致的方式定义文法结构。因此本文省略 token 文件读取的细节，只说明 SLR(1) 分析程序的核心算法。

### 1. 增广文法的表示

程序使用 NonTerminal 枚举表示非终结符：

```text
AugmentedStart -> S'
S
E
T
F
```

终结符复用公共模块中的 Terminal 枚举：

```text
Id      -> i
Plus    -> +
Mul     -> *
LParen  -> (
RParen  -> )
End     -> #
```

文法符号 Symbol 由 SymbolKind 区分终结符和非终结符，产生式 Production 记录编号、左部和右部符号序列。Grammar 构造函数中固定初始化 8 条增广文法产生式。

为了使 LR(0) 项目集编号稳定，Grammar 提供 symbolsForAutomatonExpansion()，按如下顺序扩展状态：

```text
S, E, T, F, +, *, (, ), i
```

该顺序保证程序输出的 I0 至 I12 能与分析文档和实验报告中的项目集编号对应。

> 【代码占位：syntax_analysis_slr/src/grammar.h 中 NonTerminal、Symbol、Production 和 Grammar 的定义】

> 【代码占位：syntax_analysis_slr/src/grammar.cpp 中 Grammar::Grammar、terminals、nonTerminals、symbolsForAutomatonExpansion 和 productionsFor 的实现】

### 2. FIRST/FOLLOW 集的计算

程序使用 FirstSet 表示 FIRST 集，其中包含终结符集合和 contains_epsilon 标记；使用 FollowSetResult 同时保存 FIRST 集和 FOLLOW 集。

FIRST 集计算采用不动点迭代：

1. 为每个非终结符建立空 FIRST 集。
2. 反复遍历所有产生式 A -> α。
3. 计算 α 的 FIRST 集并加入 FIRST(A)。
4. 若本轮有集合发生变化，则继续迭代；否则 FIRST 集计算结束。

FOLLOW 集计算同样采用不动点迭代：

1. 初始化普通非终结符和增广开始符号的 FOLLOW 集。
2. 将 # 加入 FOLLOW(S')。
3. 遍历每条产生式 A -> α B β。
4. 将 FIRST(β) 中除空串外的终结符加入 FOLLOW(B)。
5. 若 β 为空或 β 可推出空串，则将 FOLLOW(A) 加入 FOLLOW(B)。
6. 重复上述过程直到所有 FOLLOW 集不再变化。

本实验文法没有空产生式，但程序仍保留 contains_epsilon 处理逻辑，使算法具备通用性。由于 S' -> S 且 FOLLOW(S') 包含 #，结束符 # 会传播到 FOLLOW(S) 中，再继续传播到 FOLLOW(E)、FOLLOW(T) 和 FOLLOW(F)。

> 【代码占位：syntax_analysis_slr/src/follow_set.h 中 FirstSet、FollowSetResult 和 FollowSetCalculator 的定义】

> 【代码占位：syntax_analysis_slr/src/follow_set.cpp 中 FollowSetCalculator::calculate、firstOfSequence、addTerminal 和 addTerminals 的实现】

### 3. LR(0) 项目集规范族的构造

程序使用 LR0Item 表示 LR(0) 项目：

```text
production_id  产生式编号
dot_position   圆点在产生式右部中的位置
```

ItemSet 表示一个自动机状态，Transition 表示状态之间的转换边。LR0Automaton 中保存所有项目集和状态转换。

closure 运算的实现步骤为：

1. 将初始项目集合复制为结果集合。
2. 扫描结果集合中每个项目。
3. 如果圆点后是非终结符 B，则把 B 的所有产生式 B -> γ 以 B -> · γ 的形式加入集合。
4. 只要有新项目加入，就继续扫描。
5. 当集合不再变化时返回闭包。

goto 运算的实现步骤为：

1. 扫描当前项目集中的每个项目。
2. 找出圆点后符号等于 X 的项目。
3. 将这些项目的圆点向右移动一位。
4. 对移动后的项目集合执行 closure。

LR0AutomatonBuilder 从 closure({ S' -> · S }) 开始，使用队列保存待扩展状态。每次取出一个状态，按固定符号顺序求 goto。如果得到的新项目集尚未存在，则分配新状态编号并加入队列；无论新旧状态，都记录对应转换边。

本实验最终构造出的项目集数量为 13。关键项目集包括：

```text
I3:
E -> T ·
T -> T · * F

I10:
E -> E + T ·
T -> T · * F
```

这两个状态同时包含归约项目和可移进项目，能够体现 SLR(1) 通过 FOLLOW 集限制归约动作的作用。

> 【代码占位：syntax_analysis_slr/src/lr0_automaton.h 中 LR0Item、ItemSet、Transition 和 LR0AutomatonBuilder 的定义】

> 【代码占位：syntax_analysis_slr/src/lr0_automaton.cpp 中 LR0AutomatonBuilder::build、closure、goTo 和 symbolAfterDot 的实现】

### 4. SLR(1) 分析表的生成与表示

程序使用 ActionKind 表示 ACTION 表中的动作类型：

```text
Shift    移进
Reduce   归约
Accept   接受
Error    空表项或错误
```

Action 结构中，next_state 用于 Shift 动作，production_id 用于 Reduce 动作。SLRTable 使用两个映射保存分析表：

```text
std::map<std::pair<int, Terminal>, Action>      ACTION 表
std::map<std::pair<int, NonTerminal>, int>      GOTO 表
```

表构造分为两部分：

1. 遍历 LR(0) 自动机的所有 Transition。若转换符号是终结符，则写入移进动作；若转换符号是非终结符，则写入 GOTO 表。
2. 遍历所有项目集中的完整项目。若项目为 S' -> S ·，则在 # 列写入 acc；否则对产生式左部的 FOLLOW 集中每个终结符写入对应归约动作。

每次写入 ACTION 表时，程序都会检查原表项是否已经存在。如果已经存在相同动作，则保持不变；如果存在不同动作，则记录 SLRConflict。当前实验文法构造结果无冲突。

> 【代码占位：syntax_analysis_slr/src/slr_table.h 中 Action、SLRTable、SLRConflict 和 SLRTableBuilder 的定义】

> 【代码占位：syntax_analysis_slr/src/slr_table.cpp 中 SLRTableBuilder::build、putAction 和 SLRTable 查询接口的实现】

### 5. SLR(1) 分析算法总控程序设计

单条表达式的分析由 Parser::parse 完成。程序输入是一条已经由 TokenReader 归一化并补充结束符 # 的 ExpressionInput。

分析开始时，程序初始化：

```text
状态栈：0
符号栈：#
输入串：token 序列 #
```

每一轮分析的主要步骤如下：

1. 若输入序列已经耗尽但未达到 acc，则报告输入提前耗尽。
2. 取状态栈栈顶状态 s 和当前输入终结符 a。
3. 查询 ACTION[s, a]。
4. 若动作为 Shift(j)，则记录步骤，将 a 压入符号栈，将 j 压入状态栈，并读取下一个输入符号。
5. 若动作为 Reduce(k)，则取编号 k 的产生式 A -> β，记录步骤，弹出 |β| 个符号和状态。
6. 归约弹栈后，取新的状态栈栈顶 t，查询 GOTO[t, A]。
7. 若 GOTO[t, A] 存在，则将 A 压入符号栈，将目标状态压入状态栈；归约动作不消耗当前输入符号。
8. 若动作为 Accept，则记录 acc 步骤并返回分析成功。
9. 若 ACTION 为空，则记录当前状态和输入符号，返回语法错误。

步骤记录在执行动作前生成，因此 steps.txt 中每一行展示的是当前栈和剩余输入下即将执行的动作。这种记录方式便于检查每一步查表依据。

> 【代码占位：syntax_analysis_slr/src/parser.h 中 ParseDiagnostic、ParseStep、ParseResult 和 Parser 的定义】

> 【代码占位：syntax_analysis_slr/src/parser.cpp 中 Parser::parse 的移进、归约、接受和错误处理实现】

### 6. 结果输出

实验输出由 ResultWriter 统一完成。程序会先创建输出目录，然后输出以下文件：

```text
result.txt
error.txt
follow_sets.txt
lr0_item_sets.txt
slr_table.txt
steps.txt
```

各文件含义如下：

1. result.txt：记录每条表达式的最终分析结论，标明表达式正确或错误。
2. error.txt：记录错误表达式的错误类型、当前状态、当前输入词素、终结符、token 文件行号和错误说明。如果没有错误，则输出“无错误”。
3. follow_sets.txt：记录每个非终结符的 FIRST 集和 FOLLOW 集。
4. lr0_item_sets.txt：记录 LR(0) 项目集规范族和状态转换关系。
5. slr_table.txt：以表格形式输出 ACTION/GOTO 分析表，并输出冲突检查结果。
6. steps.txt：记录每条表达式的详细分析过程，包括步骤编号、状态栈、符号栈、剩余输入和动作。

如果 SLR(1) 表构造存在冲突，程序会输出静态构造结果和冲突信息，但不会继续进行输入表达式分析。如果词法分析错误文件非空，程序也不会继续进行 SLR(1) 语法分析，而是将词法错误写入本实验的错误文件中。

> 【代码占位：syntax_analysis_slr/src/result_writer.h 中 ResultWriter 的定义】

> 【代码占位：syntax_analysis_slr/src/result_writer.cpp 中 writeStaticOutputs、writeParseOutputs、writeDiagnostic 和表格输出函数的实现】

## 五、运行与测试

### 1. 测试输入文件

本实验程序默认读取词法分析阶段生成的 token 文件：

```text
lexical_analysis/output/result.txt
```

同时读取词法分析错误文件：

```text
lexical_analysis/output/error.txt
```

如果词法错误文件为空，说明词法分析通过，SLR(1) 语法分析可以继续执行。当前测试输入中包含多条以分号分隔的表达式，既包括正确表达式，也包括存在括号不匹配、运算符连续、表达式缺少操作数等问题的错误表达式。

测试输入覆盖的表达式包括：

```text
(i+i)*i;
i+i)*i;
i;
i+i;
i*i+i;
((i+i)*i);
i+*i;
(i+i;
+i;
i*;
1+2*3;
```

其中 1+2*3 用于验证整数常量能否被归一化为表达式文法中的终结符 i，并按照同一套 SLR(1) 分析表完成分析。

### 2. 静态构造结果

程序运行后，在 syntax_analysis_slr/output/ 目录下生成六个文件：

```text
result.txt
error.txt
follow_sets.txt
lr0_item_sets.txt
slr_table.txt
steps.txt
```

follow_sets.txt 中输出的 FOLLOW 集为：

```text
FOLLOW(S) = { # }
FOLLOW(E) = { +, ), # }
FOLLOW(T) = { +, *, ), # }
FOLLOW(F) = { +, *, ), # }
```

lr0_item_sets.txt 中输出 13 个项目集 I0 至 I12，并输出 23 条状态转换。关键转换包括：

```text
I0 -- S --> I1
I0 -- E --> I2
I0 -- T --> I3
I0 -- F --> I4
I0 -- ( --> I5
I0 -- i --> I6
I2 -- + --> I7
I3 -- * --> I8
I9 -- ) --> I12
I10 -- * --> I8
```

slr_table.txt 中输出 ACTION/GOTO 表。表中重要动作包括：

```text
ACTION[I1, #] = acc
ACTION[I3, *] = s8
ACTION[I3, +] = r3
ACTION[I10, *] = s8
ACTION[I10, #] = r2
```

这些表项说明程序能够在 T 后遇到 * 时继续移进，而在遇到 +、) 或 # 时进行归约，从而正确处理乘法优先于加法的表达式结构。冲突检查结果为：

```text
移进/归约冲突：无
归约/归约冲突：无
```

### 3. 结果文件

result.txt 用于记录每条表达式的最终分析结果。当前测试中，程序共分析 11 条表达式，其中 6 条正确、5 条错误。

正确表达式包括：

```text
(i+i)*i
i
i+i
i*i+i
((i+i)*i)
1+2*3
```

错误表达式包括：

```text
i+i)*i
i+*i
(i+i
+i
i*
```

这些结果表明程序能够识别单个操作数、普通加法表达式、乘法表达式、加乘混合表达式、括号表达式和整数常量表达式；同时也能够发现多余右括号、连续运算符、缺少右括号、开头运算符和结尾缺少操作数等错误。

### 4. 步骤文件

steps.txt 用于记录每条表达式的完整分析过程。每一行包含：

```text
步骤    状态栈    符号栈    剩余输入    动作
```

例如，对于表达式 (i+i)*i#，程序首先在状态 I0 遇到 (，执行 s5 移进；随后在 I5 遇到 i，执行 s6 移进；再根据状态 I6 和输入 + 执行 r7，将 i 归约为 F。后续程序依次执行：

```text
F -> i
T -> F
E -> T
```

当括号内的 E + T 被识别完成后，程序按：

```text
E -> E + T
F -> ( E )
T -> F
```

继续归约。遇到乘号时，状态 I3 在 * 列为 s8，因此程序移进 * 并识别右侧因子。最后依次归约：

```text
T -> T * F
E -> T
S -> E
```

状态栈到达 I1 且当前输入为 # 时，ACTION[I1, #] = acc，分析成功。

步骤文件的作用不仅是给出最终结论，还可以展示程序每一步移进和归约的依据，便于检查 LR(0) 项目集、FOLLOW 集和 SLR(1) 分析表是否配合正确。

### 5. 错误文件

error.txt 用于记录错误表达式的具体错误原因。当前测试中，错误主要表现为 ACTION 表项为空：

1. i+i)*i：表达式已经归约到状态 I2，但当前输入为多余的 )，ACTION[I2, )] 为空。
2. i+*i：状态 I7 表示已经读入 E +，此时应该继续读入 T 的起始符号 i 或 (，但输入为 *，ACTION[I7, *] 为空。
3. (i+i：状态 I9 表示已经识别左括号内的 E，后续应读入 ) 或 +，但输入已经到达 #，ACTION[I9, #] 为空。
4. +i：初始状态 I0 不能以 + 开始表达式，ACTION[I0, +] 为空。
5. i*：状态 I8 表示已经读入 T *，此时应该继续读入 F 的起始符号 i 或 (，但输入到达 #，ACTION[I8, #] 为空。

错误文件同时输出错误 token 在 lexical_analysis/output/result.txt 中的来源行号。该行号是 token 文件行号，不是源程序原始行列号。

## 六、实验总结

通过本实验，我完成了一个基于 SLR(1) 分析方法的表达式语法分析程序。程序能够根据固定表达式文法自动计算 FIRST/FOLLOW 集，自动构造 LR(0) 项目集规范族，自动生成 SLR(1) ACTION/GOTO 分析表，并利用该分析表对多条输入表达式进行移进、归约、接受和错误判断。

本实验进一步说明了 LR 分析方法与算符优先分析方法的差异。算符优先分析通过终结符之间的优先关系决定移进和归约，而 SLR(1) 分析通过状态和输入符号查 ACTION 表决定下一步动作。SLR(1) 归约时保留具体产生式编号和左部非终结符，因此能够更清晰地展示表达式从 F、T、E 到 S 的归约过程。

本实验也体现了 FOLLOW 集在 SLR(1) 分析中的作用。LR(0) 项目集只能说明某个状态中存在可归约项目，但不能单独判断当前输入是否应该归约。将归约动作限制在产生式左部的 FOLLOW 集中，可以在本实验表达式文法中避免乘号位置的移进/归约冲突。

### 问题 1：FOLLOW 集对归约动作的限制容易被忽略

#### 1. 问题描述

在构造 SLR(1) 表时，如果只看到项目 E -> T ·，就容易直接在所有终结符列上填写 r3。这样状态 I3 在 * 列既需要根据 T -> T · * F 移进，又会根据 E -> T · 归约，从而产生移进/归约冲突。

#### 2. 问题分析

SLR(1) 与 LR(0) 的重要区别就在于归约动作不是填入所有终结符列，而是只填入 FOLLOW(A) 中的终结符列。对于 E -> T ·，归约左部是 E，FOLLOW(E) 为：

```text
{ +, ), # }
```

因此，E -> T 的归约动作只能填入 +、)、# 三列，不能填入 * 列。状态 I3 在 * 列应保留 T -> T · * F 对应的移进动作 s8。

#### 3. 解决方案

程序在 SLRTableBuilder 中处理完整项目时，会先取得归约产生式左部的 FOLLOW 集，再只对该集合中的终结符写入 Reduce 动作。这样 I3 和 I10 都能在 * 列继续移进，在 +、) 和 # 列执行归约，从而正确体现表达式文法中 * 的优先级高于 +。

### 问题 2：SLR(1) 归约不能使用统一占位符

#### 1. 问题描述

算符优先实验中，归约时可以把 i、N+N、N*N、(N) 都归约为统一占位符 N。但在 SLR(1) 实验中，如果继续使用统一占位符，就无法根据归约结果查询 GOTO 表。

#### 2. 问题分析

SLR(1) 分析表中的 GOTO 表以非终结符为列。归约 E -> T 后需要查询 GOTO[state, E]，归约 T -> F 后需要查询 GOTO[state, T]，归约 F -> i 后需要查询 GOTO[state, F]。这些非终结符对应的后继状态通常不同。

如果将它们都折叠为 N，程序既不能找到正确的 GOTO 表项，也不能输出准确的产生式归约步骤。

#### 3. 解决方案

程序的 Reduce 动作保存 production_id。归约时，Parser 根据 production_id 取出完整产生式，按照产生式右部长度弹栈，并将产生式左部非终结符压回符号栈。随后使用该左部非终结符查询 GOTO 表。这样分析过程可以严格按照 SLR(1) 总控程序执行。

### 尚未解决或可以改进的问题

当前程序可以完成本实验要求，但仍有一些可以继续改进的地方：

1. 当前文法是固定写在 Grammar 构造函数中的，后续可以考虑从配置文件或文法描述文件中读取产生式。
2. 当前程序默认路径以工程根目录为基准，后续可以增加命令行参数，或统一 CMake 运行目标与 main.cpp 中的相对路径约定。
3. 当前错误位置记录的是 token 在 lexical_analysis/output/result.txt 中的行号，不是源程序中的原始行列号。如果词法分析阶段保留源代码位置，语法分析阶段的错误定位可以更加直观。
4. 当前程序主要面向 +、* 和括号表达式。若后续扩展减法、除法、一元运算符或更多表达式类型，需要同步扩展文法、终结符映射、FIRST/FOLLOW 集计算、LR(0) 自动机构造和 SLR(1) 表输出。
