# 算符优先语法分析程序设计

本文档根据 `task.md`、`analysis.md`，并参考项目中已经完成的词法分析与自顶向下语法分析程序，整理第三次实验“算符优先语法分析”的程序结构设计。后续实现时应继续沿用现有项目约定：语法分析阶段不直接扫描原始表达式字符串，而是读取 `lexical_analysis/output/result.txt` 与 `lexical_analysis/output/error.txt`，将词法 token 归一化为本实验文法中的终结符后再进行分析。

## 1. 实验目标

本实验实现一个基于算符优先关系表的自下而上表达式语法分析器，完成以下任务：

1. 使用给定表达式文法描述语法结构。
2. 自动计算 `FirstVT` 与 `LastVT` 集。
3. 根据 `FirstVT` / `LastVT` 构造算符优先关系表。
4. 输出 `FirstVT`、`LastVT` 和算符优先关系表。
5. 读取词法分析输出中的表达式 token 序列。
6. 使用移进-归约算法判断每条表达式是否正确。
7. 输出每条表达式的分析结果、错误原因和归约过程。

本实验不要求实现 Floyd 优先函数，也不要求对表达式求值。即使输入 token 中包含整数常量，也只需要在语法层面将其视为文法终结符 `i`。

## 2. 使用文法

本实验使用原始表达式文法：

```text
E -> E + T | T
T -> T * F | F
F -> ( E ) | i
```

终结符集合：

```text
VT = { i, +, *, (, ), # }
```

其中 `#` 是程序额外加入的输入结束符和栈底符号。

非终结符集合：

```text
VN = { E, T, F }
```

注意：自顶向下语法分析实验为了消除左递归使用的是 `E'`、`T'` 等辅助非终结符；算符优先分析应使用这里的原始左递归文法，不需要消除左递归。

## 3. 与既有程序的对接方式

### 3.1 输入来源

算符优先语法分析程序默认读取：

```text
../lexical_analysis/output/result.txt
../lexical_analysis/output/error.txt
```

其中 `result.txt` 的 token 行格式由词法分析程序产生：

```text
(lexeme, token_code)
```

例如：

```text
(i, 0)
(+, 200)
(1, 1)
(;, 301)
```

如果 `../lexical_analysis/output/error.txt` 非空，说明词法分析未通过。此时算符优先语法分析应停止，不继续执行 `FirstVT` 之外的输入分析流程，并在本实验的 `output/error.txt` 中说明“词法分析未通过，算符优先语法分析未执行”。

### 3.2 token 归一化规则

本实验继续沿用自顶向下语法分析中的 token 归一化策略：

| 词法 token | 种别码 | 算符优先终结符 |
| --- | --- | --- |
| `TK_IDENTIFIER`，且 lexeme 为 `i` | 0 | `i` |
| `TK_INT_LITERAL` | 1 | `i` |
| `OP_PLUS` | 200 | `+` |
| `OP_MUL` | 202 | `*` |
| `SEP_LPAREN` | 302 | `(` |
| `SEP_RPAREN` | 303 | `)` |
| `SEP_SEMICOLON` | 301 | 当前表达式结束，内部追加 `#` |

其他 token 均不属于本实验表达式文法，应作为当前表达式的输入错误处理。例如其他标识符、`-`、`/`、关键字、赋值号、关系运算符、字符常量、字符串常量等。

### 3.3 多表达式切分

与自顶向下语法分析相同，`SEP_SEMICOLON` 用于切分多条表达式。每读到一个分号，就结束当前表达式，并在当前表达式 token 序列末尾追加：

```text
#
```

例如源程序中有：

```text
(i+i)*i;
i+i)*i;
```

语法分析阶段应得到两条表达式，分别独立执行算符优先分析。

### 3.4 错误位置含义

当前 `lexical_analysis/output/result.txt` 只保存 `(lexeme, token_code)`，不保存原始源程序行列号。因此语法分析阶段记录的 `source_index` 应继续表示 token 在 `result.txt` 中的行号，而不是原始源文件中的行号。

## 4. 需要复用或抽取的既有结构

自顶向下语法分析中已有一些结构可以继续使用，但不建议直接让算符优先模块依赖 `syntax_analysis_top_down_lib`，因为两个实验的 `Grammar` 与 `Parser` 语义不同。更合理的方式是抽出一个小的公共库。

建议新增公共模块：

```text
syntax_analysis_common/
  src/
    terminal.h
    terminal.cpp
    token_reader.h
    token_reader.cpp
```

公共模块负责：

1. 定义表达式终结符 `Terminal`：

```text
Id, Plus, Mul, LParen, RParen, End
```

2. 提供 `toString(Terminal)`。
3. 定义 `InputToken`：

```text
terminal
lexeme
source_index
```

4. 定义 `TokenReadError`、`ExpressionInput`、`TokenReaderResult`。
5. 提供读取词法输出并按分号切分表达式的 `TokenReader`。

这样自顶向下语法分析和算符优先语法分析都可以共用同一套词法输出读取逻辑，避免重复维护 token 种别码映射。

如果暂时不想重构，也可以先在 `syntax_analysis_operator_precedence/src/` 中复制并改名一份 `token_reader`，保持接口与自顶向下版本一致。后续再抽公共模块即可。但从后续实验维护角度看，抽出 `syntax_analysis_common` 是更稳妥的选择。

## 5. 建议目录结构

```text
syntax_analysis_operator_precedence/
  docs/
    task.md
    analysis.md
    program_design.md

  src/
    main.cpp

    grammar.h
    grammar.cpp

    first_last_vt.h
    first_last_vt.cpp

    precedence_table.h
    precedence_table.cpp

    parser.h
    parser.cpp

    result_writer.h
    result_writer.cpp

    diagnostic.h
```

如果抽出公共模块，则 `token_reader`、`InputToken` 和 `Terminal` 放到 `syntax_analysis_common/src/`。如果暂不抽取公共模块，则还需要在本目录下增加：

```text
    token_reader.h
    token_reader.cpp
```

## 6. Library 与 Executable 划分

建议仍然采用和自顶向下语法分析一致的 library + executable 结构：

1. `syntax_analysis_operator_precedence_lib`
   - 负责文法、`FirstVT` / `LastVT`、优先关系表、算符优先分析、结果数据结构与输出。
2. `syntax_analysis_operator_precedence`
   - 负责程序入口、路径组织、调用 library、打印运行摘要。

后续 `CMakeLists.txt` 可以新增：

```text
add_subdirectory(syntax_analysis_operator_precedence)
```

并在该目录下定义：

```text
syntax_analysis_operator_precedence_lib
syntax_analysis_operator_precedence
run_syntax_analysis_operator_precedence
```

`run_syntax_analysis_operator_precedence` 的 `WORKING_DIRECTORY` 应设置为：

```text
syntax_analysis_operator_precedence
```

这样程序内部访问词法输出时可以继续使用：

```text
../lexical_analysis/output/result.txt
../lexical_analysis/output/error.txt
```

## 7. 模块职责

### 7.1 grammar.h / grammar.cpp

该模块描述算符优先分析使用的原始文法。

建议定义：

```text
enum class NonTerminal { E, T, F };

struct Symbol {
    SymbolKind kind;
    Terminal terminal;
    NonTerminal non_terminal;
};

struct Production {
    int id;
    NonTerminal lhs;
    vector<Symbol> rhs;
};
```

产生式集合：

| 编号 | 产生式 |
| --- | --- |
| 1 | `E -> E + T` |
| 2 | `E -> T` |
| 3 | `T -> T * F` |
| 4 | `T -> F` |
| 5 | `F -> ( E )` |
| 6 | `F -> i` |

`Grammar` 应提供：

1. `productions()`：返回所有产生式。
2. `terminals()`：返回 `i + * ( ) #`。
3. `nonTerminals()`：返回 `E T F`。
4. `startSymbol()`：返回 `E`。
5. `toString(...)`：输出终结符、非终结符、符号和产生式。

### 7.2 first_last_vt.h / first_last_vt.cpp

该模块负责计算 `FirstVT` 与 `LastVT`。

建议输出结构：

```text
struct FirstLastVTResult {
    map<NonTerminal, set<Terminal>> first_vt;
    map<NonTerminal, set<Terminal>> last_vt;
};
```

`FirstVT` 计算规则：

1. 若有 `P -> a ...`，则 `a` 加入 `FirstVT(P)`。
2. 若有 `P -> Q a ...`，则 `a` 加入 `FirstVT(P)`。
3. 若有 `P -> Q ...`，则 `FirstVT(Q)` 传递加入 `FirstVT(P)`。
4. 重复迭代，直到集合不再变化。

`LastVT` 计算规则：

1. 若有 `P -> ... a`，则 `a` 加入 `LastVT(P)`。
2. 若有 `P -> ... a Q`，则 `a` 加入 `LastVT(P)`。
3. 若有 `P -> ... Q`，则 `LastVT(Q)` 传递加入 `LastVT(P)`。
4. 重复迭代，直到集合不再变化。

本实验文法应计算得到：

```text
FirstVT(E) = { +, *, (, i }
FirstVT(T) = { *, (, i }
FirstVT(F) = { (, i }

LastVT(E) = { +, *, ), i }
LastVT(T) = { *, ), i }
LastVT(F) = { ), i }
```

### 7.3 precedence_table.h / precedence_table.cpp

该模块负责根据文法和 `FirstVT` / `LastVT` 构造算符优先关系表。

建议定义：

```text
enum class PrecedenceRelation {
    Less,     // <
    Equal,    // =
    Greater   // >
};

class PrecedenceTable {
public:
    optional<PrecedenceRelation> lookup(Terminal left, Terminal right) const;
    void set(Terminal left, Terminal right, PrecedenceRelation relation);
};
```

构造规则：

1. 若产生式右部出现相邻终结符 `a b`，则置 `a = b`。
2. 若产生式右部出现 `a Q b`，其中 `a`、`b` 是终结符，`Q` 是非终结符，则置 `a = b`。
3. 若产生式右部出现 `a Q`，则对所有 `b in FirstVT(Q)`，置 `a < b`。
4. 若产生式右部出现 `Q a`，则对所有 `b in LastVT(Q)`，置 `b > a`。
5. 对开始符号 `E` 增加 `#` 相关关系：

```text
# < a, a in FirstVT(E)
b > #, b in LastVT(E)
# = #
```

构造表时必须检测冲突。如果同一个单元格已经存在关系，又要写入不同关系，说明该文法在当前规则下不是算符优先文法，应返回构造错误，而不是静默覆盖。

本实验文法的关系表应为：

|     | + | * | ( | ) | i | # |
| --- | --- | --- | --- | --- | --- | --- |
| + | > | < | < | > | < | > |
| * | > | > | < | > | < | > |
| ( | < | < | < | = | < |   |
| ) | > | > |   | > |   | > |
| i | > | > |   | > |   | > |
| # | < | < | < |   | < | = |

空白表示不存在优先关系。分析过程中遇到空白关系时，应判定为语法错误。

### 7.4 parser.h / parser.cpp

该模块实现算符优先移进-归约分析。

建议分析栈元素不要继续保存具体的 `E/T/F`，而是保存终结符或统一的非终结符占位符 `N`：

```text
enum class StackSymbolKind { Terminal, NonTerminalPlaceholder };

struct StackSymbol {
    StackSymbolKind kind;
    Terminal terminal;
};
```

算符优先分析只通过终结符之间的优先关系确定移进和归约边界，归约后的非终结符只需要作为占位符参与后续短语结构匹配。

建议结果结构：

```text
struct OperatorPrecedenceStep {
    int index;
    string stack;
    string remaining_input;
    string relation;
    string action;
};

struct OperatorPrecedenceParseResult {
    bool accepted;
    vector<OperatorPrecedenceStep> steps;
    string error_message;
    size_t error_token_index;
};
```

核心流程：

1. 输入 token 序列末尾应保证有 `#`。
2. 初始化分析栈为 `#`。
3. 每轮找到栈中最靠近栈顶的终结符 `a`。
4. 读取当前输入终结符 `b`。
5. 如果 `a == #`、`b == #`，并且栈形态为 `# N`，分析成功。
6. 查询优先关系 `table[a, b]`。
7. 若关系为空，报语法错误。
8. 若关系为 `<` 或 `=`，移进当前输入符号 `b`。
9. 若关系为 `>`，查找最左素短语并尝试归约。

### 7.5 归约短语查找

归约时不能简单弹出栈顶符号，而要找到最左素短语边界。

查找过程：

1. 从栈顶向左找到第一个终结符，记为 `right_terminal`。
2. 继续向左找到它前面的终结符，记为 `left_terminal`。
3. 查询 `left_terminal` 与 `right_terminal` 的优先关系。
4. 如果关系是 `<`，则边界找到，`left_terminal` 右侧到栈顶就是待归约短语。
5. 如果关系是 `=` 或 `>`，继续向左查找。
6. 如果找不到前一个终结符，或关系为空，则归约失败。

查找时应跳过栈中的 `N`，因为 `N` 不是终结符，不参与优先关系比较。

### 7.6 归约匹配规则

归约短语匹配时建议把所有非终结符统一视为 `N`。本实验只需要支持以下模式：

| 短语模式 | 归约结果 |
| --- | --- |
| `i` | `N` |
| `N + N` | `N` |
| `N * N` | `N` |
| `( N )` | `N` |

这四个模式对应原文法中的：

```text
F -> i
E -> E + T
T -> T * F
F -> ( E )
```

原文法中的单产生式 `E -> T`、`T -> F` 不需要在栈中反复归约为不同名字的非终结符；在算符优先分析器里统一用 `N` 表示“已经归约出的一个表达式成分”即可。

### 7.7 diagnostic.h

建议集中定义错误类型，避免 parser 和 result writer 中散落字符串判断。

错误类型至少包括：

1. 词法分析未通过。
2. token 文件格式错误。
3. token 不属于本实验表达式文法。
4. 表达式缺少结束分号。
5. 空表达式。
6. 当前栈顶终结符与当前输入终结符之间无优先关系。
7. 归约时找不到最左素短语边界。
8. 待归约短语不能匹配任何归约模式。
9. 输入结束后栈不满足 `# N`。

### 7.8 result_writer.h / result_writer.cpp

本实验建议输出目录：

```text
syntax_analysis_operator_precedence/output/
```

建议输出文件：

```text
output/result.txt
output/error.txt
output/first_last_vt.txt
output/precedence_table.txt
output/steps.txt
```

各文件职责：

1. `result.txt`
   - 记录每条表达式最终判断结果。
2. `error.txt`
   - 记录词法错误、输入 token 错误和语法错误详情。
3. `first_last_vt.txt`
   - 输出每个非终结符的 `FirstVT` / `LastVT` 集。
4. `precedence_table.txt`
   - 输出算符优先关系表。
5. `steps.txt`
   - 输出每条表达式的移进、归约和出错过程。

`steps.txt` 建议格式：

```text
表达式 1: 正确
输入符号: ( i + i ) * i #

步骤    分析栈        剩余输入        优先关系    动作
1       #             (i+i)*i#        # < (       移进 (
2       #(            i+i)*i#         ( < i       移进 i
3       #(i           +i)*i#          i > +       归约 i -> N
...
```

## 8. main.cpp 执行流程

`main.cpp` 应保持简单，只负责流程编排。

默认路径：

```text
token_path = "../lexical_analysis/output/result.txt"
lexical_error_path = "../lexical_analysis/output/error.txt"
output_dir = "output"
```

建议支持命令行覆盖：

```text
syntax_analysis_operator_precedence <token_path> <lexical_error_path> <output_dir>
```

整体流程：

```text
启动 syntax_analysis_operator_precedence
        |
        v
初始化 Grammar
        |
        v
计算 FirstVT / LastVT
        |
        v
构造算符优先关系表，检测冲突
        |
        v
写 first_last_vt.txt 和 precedence_table.txt
        |
        v
检查 ../lexical_analysis/output/error.txt
        |
        +-- 非空 -> 写 result/error/steps，结束
        |
        v
读取 ../lexical_analysis/output/result.txt
        |
        v
按分号切分表达式，并归一化为 i + * ( ) # 序列
        |
        v
对每条表达式执行算符优先分析
        |
        v
写 result.txt、error.txt、steps.txt
        |
        v
命令行打印分析摘要
```

## 9. 分析成功与失败条件

### 9.1 成功条件

当满足以下条件时分析成功：

```text
当前输入符号为 #
分析栈为 # N
```

也就是说，输入已经读完，栈底符号上方只剩一个归约后的表达式成分。

### 9.2 失败条件

以下情况应判定为语法错误：

1. 当前表达式存在 token 归一化错误。
2. 当前栈顶终结符与当前输入终结符之间没有优先关系。
3. 归约时无法找到 `<` 边界。
4. 找到的短语无法匹配 `i`、`N + N`、`N * N`、`( N )`。
5. 输入读到 `#` 后，栈不满足 `# N`。
6. 读到右括号、运算符或结束符时无法通过优先关系和归约模式形成合法表达式。

例如：

```text
i+i)*i
```

应在多余的 `)` 或其后的关系判断阶段被判为错误。

## 10. 测试建议

实验要求至少测试：

```text
(i+i)*i
i+i)*i
```

建议额外测试：

```text
i
i+i
i*i+i
((i+i)*i)
i+*i
(i+i
+i
i*
1+2*3
```

其中 `1+2*3` 在语法上应视为正确，因为整数常量会被归一化为终结符 `i`；但本实验不需要输出计算结果。

## 11. 后续实现顺序

建议按以下顺序推进：

1. 先决定是否抽出 `syntax_analysis_common`。
2. 实现或复用 `Terminal`、`InputToken` 和 `TokenReader`。
3. 实现 `grammar`，固定原始左递归表达式文法。
4. 实现 `first_last_vt`，验证集合结果是否与本文档一致。
5. 实现 `precedence_table`，验证关系表是否与本文档一致，并加入冲突检测。
6. 实现 `parser` 的移进-归约过程。
7. 实现 `result_writer` 输出五类文件。
8. 实现 `main.cpp` 编排流程。
9. 增加 `CMakeLists.txt` 和 `run_syntax_analysis_operator_precedence` 目标。
10. 使用词法分析输出对实验要求中的两条表达式做端到端验证。

## 12. 需要提前指出的重构任务

如果后续希望真正“复用自顶向下语法分析里完成的代码”，建议先做一个小重构：

1. 把 `Terminal`、`toString(Terminal)`、`InputToken`、`TokenReader` 和相关读取错误结构从 `syntax_analysis_top_down` 中抽到 `syntax_analysis_common`。
2. 让 `syntax_analysis_top_down_lib` 和 `syntax_analysis_operator_precedence_lib` 同时链接 `syntax_analysis_common`。
3. 两个语法分析实验各自保留自己的 `Grammar`、`Parser`、`ResultWriter`。

不建议直接让算符优先模块包含 `syntax_analysis_top_down/src/grammar.h`，原因是：

1. 自顶向下模块的 `NonTerminal` 包含 `E'`、`T'`，对应的是消除左递归后的 LL(1) 文法。
2. 算符优先模块需要的是原始文法 `E/T/F`。
3. 两者的 parser 算法和输出步骤语义完全不同。

因此，推荐复用“词法输出读取与终结符归一化”这层公共能力，而不是复用 LL(1) 的文法和预测分析表。
