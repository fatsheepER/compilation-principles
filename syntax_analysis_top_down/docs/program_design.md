# 语法分析程序设计

本文档根据 `task.md`、`flowchart.md` 和 `ll1_analysis_table.md`，整理预测分析语法分析器的程序结构设计。当前阶段先确定模块边界、文件职责和整体执行流程，后续实现代码时以本文档为依据。

## 1. 设计目标

本实验要求使用 LL(1) 预测分析方法，对算术表达式进行语法分析，判断表达式是否符合给定文法。

原始文法为：

```text
E → E + T | T
T → T * F | F
F → ( E ) | i
```

消除左递归后，程序实际使用的 LL(1) 文法为：

```text
E  → T E'
E' → + T E' | ε
T  → F T'
T' → * F T' | ε
F  → ( E ) | i
```

语法分析阶段不直接扫描原始表达式字符串，而是读取词法分析阶段产生的 token 文件，并将词法 token 归一化为预测分析表中的终结符。

## 2. 程序整体组成

语法分析程序建议拆分为两部分：

1. `syntax_analysis_top_down_lib`：语法分析库，负责 token 读取、终结符归一化、预测分析、表达式求值和结果生成。
2. `syntax_analysis_top_down`：可执行程序，负责命令行入口、路径组织、调用 library 和打印运行摘要。

这样的划分可以让核心语法分析逻辑与程序入口分离，后续如果需要测试 parser、输出分析过程或复用语法分析功能，只需要调用 library。

## 3. 建议目录结构

```text
syntax_analysis_top_down/
  docs/
    task.md
    flowchart.md
    ll1_analysis_table.md
    program_design.md

  src/
    main.cpp

    token_reader.h
    token_reader.cpp

    grammar.h
    grammar.cpp

    parser.h
    parser.cpp

    evaluator.h
    evaluator.cpp

    diagnostic.h

    result_writer.h
    result_writer.cpp
```

## 4. Library 组成

### 4.1 token_reader.h / token_reader.cpp

该模块负责与词法分析阶段对接。

主要职责：

1. 读取 `../lexical_analysis/output/error.txt`。
2. 如果词法错误文件非空，说明词法分析未通过，语法分析阶段应报告错误并停止预测分析。
3. 读取 `../lexical_analysis/output/result.txt`。
4. 解析词法分析结果中的 token 行。

词法分析结果格式如下：

```text
(lexeme, token_code)
```

例如：

```text
(1, 1)
(+, 200)
(2, 1)
(;, 301)
```

5. 按 `SEP_SEMICOLON = 301` 将 token 流切分为多条表达式。
6. 将每条表达式的 token 归一化为语法分析终结符。

归一化规则如下：

| 词法 token | 种别码 | 语法终结符 |
| --- | --- | --- |
| `TK_IDENTIFIER`，且 lexeme 为 `i` | 0 | `i` |
| `TK_INT_LITERAL` | 1 | `i` |
| `OP_PLUS` | 200 | `+` |
| `OP_MUL` | 202 | `*` |
| `SEP_LPAREN` | 302 | `(` |
| `SEP_RPAREN` | 303 | `)` |
| `SEP_SEMICOLON` | 301 | `#` |

除上表外的 token，例如其他标识符、`-`、`/`、关键字、赋值号、关系运算符、字符常量、字符串常量等，均不属于本实验表达式文法，应作为语法错误处理。

### 4.2 grammar.h / grammar.cpp

该模块负责描述 LL(1) 文法和预测分析表。

主要内容：

1. 终结符集合：

```text
i, +, *, (, ), #
```

2. 非终结符集合：

```text
E, E', T, T', F
```

3. 产生式集合：

| 编号 | 产生式 |
| --- | --- |
| 1 | `E → T E'` |
| 2 | `E' → + T E'` |
| 3 | `E' → ε` |
| 4 | `T → F T'` |
| 5 | `T' → * F T'` |
| 6 | `T' → ε` |
| 7 | `F → ( E )` |
| 8 | `F → i` |

4. 预测分析表 `M`。

预测分析表直接依据 `ll1_analysis_table.md` 实现。parser 不应在算法内部散落大量特殊判断，而应统一通过该模块查询表项。

### 4.3 parser.h / parser.cpp

该模块负责执行 LL(1) 预测分析算法。

核心算法与 `flowchart.md` 中教材图 4.4 的流程一致：

1. 初始化分析栈，将 `#` 和开始符号 `E` 入栈。
2. 读取当前输入符号 `a`。
3. 弹出栈顶符号 `X`。
4. 如果 `X` 是终结符：
   - 若 `X == a`，则输入指针前进。
   - 否则报错。
5. 如果 `X == #`：
   - 若 `a == #`，则分析成功。
   - 否则报错。
6. 如果 `X` 是非终结符：
   - 查询预测分析表 `M[X, a]`。
   - 如果存在产生式 `X → y1 y2 ... yn`，则将右部符号按 `yn ... y2 y1` 逆序压栈。
   - 如果产生式右部为 `ε`，则不压栈。
   - 如果表项为空，则报语法错误。

parser 应输出结构化分析结果，至少包括：

```text
表达式编号
是否分析成功
错误位置
错误原因
可选的分析步骤
```

### 4.4 diagnostic.h

该文件定义语法分析阶段使用的错误信息结构。

建议包含字段：

```text
expression_index
token_index
lexeme
terminal
message
```

建议覆盖以下错误类型：

1. 词法分析未通过。
2. token 文件格式错误。
3. 出现不属于表达式文法的 token。
4. 终结符匹配失败。
5. 预测分析表无入口。
6. 表达式缺少分号。
7. 空表达式。

### 4.5 evaluator.h / evaluator.cpp

该模块负责在表达式通过语法分析后，尝试计算表达式的具体数值。

求值模块不参与 LL(1) 预测分析，也不改变预测分析表。语法分析阶段仍然把 `TK_INT_LITERAL` 和合法标识符 `i` 统一归一化为文法终结符 `i`；求值阶段则使用 `InputToken.lexeme` 中保留的原始词素区分整数常量和标识符。

主要职责：

1. 接收单条表达式的 `std::vector<InputToken>`。
2. 按表达式文法的优先级计算整数表达式：
   - `E` 处理加法。
   - `T` 处理乘法。
   - `F` 处理无符号整数和括号表达式。
3. 当操作数是无符号整数时，将 `lexeme` 转换为 `long long` 并参与计算。
4. 当表达式中出现标识符 `i` 时，不计算具体值，返回“无法计算，表达式含有标识符 i”。
5. 当整数超出 `long long` 范围，或求值过程出现异常状态时，返回求值失败信息。

求值结果使用结构化结果表示：

```text
available: 是否得到有效计算值
value: 计算结果
message: 无法计算或求值失败原因
```

其中 `available == false` 可能表示两类情况：

1. 表达式语法正确，但包含标识符 `i`，因此没有具体数值。
2. 求值阶段发现异常情况，例如整数范围溢出或求值过程没有停在结束符 `#`。

因此调用方和输出模块应直接保留 `EvaluationResult.message`，不要把所有 `available == false` 都当作语法错误。

### 4.6 result_writer.h / result_writer.cpp

该模块负责输出语法分析结果。

建议输出目录为：

```text
syntax_analysis_top_down/output/
```

建议输出文件：

```text
output/result.txt
output/error.txt
```

`result.txt` 用于记录每条表达式的最终判断结果，例如：

```text
表达式 1: 正确
计算结果: 3
表达式 2: 错误
```

对于语法正确但包含标识符 `i` 的表达式，`result.txt` 应保留语法判断结果，同时说明无法计算具体值，例如：

```text
表达式 3: 正确
计算结果: 无法计算，表达式含有标识符 i
```

`error.txt` 用于记录详细错误信息，例如：

```text
表达式 2: 在符号 "*" 处出错，栈顶非终结符 F，预测分析表 M[F, *] 为空
```

如果后续实验报告需要展示预测分析过程，可以额外输出：

```text
output/steps.txt
```

其中记录每一步的分析栈、剩余输入和所用产生式。

## 5. Executable 组成

`src/main.cpp` 应保持简单，只负责程序入口和流程编排。

主要职责：

1. 确定默认输入路径：

```text
../lexical_analysis/output/result.txt
../lexical_analysis/output/error.txt
```

当前 CMake 自定义目标 `run_syntax_analysis_top_down` 的工作目录是：

```text
syntax_analysis_top_down
```

因此从语法分析程序访问词法分析输出时，应使用 `../lexical_analysis/output/...`。

2. 确定输出路径：

```text
output/result.txt
output/error.txt
```

3. 调用 library 完成语法分析。
4. 对语法分析正确的表达式调用 `ExpressionEvaluator` 尝试求值。
5. 捕获异常并输出错误摘要。
6. 在命令行打印运行结果摘要，例如：

```text
语法分析完毕.
共分析 3 条表达式.
正确 2 条, 错误 1 条.
```

`main.cpp` 不应直接实现预测分析算法，也不应直接维护预测分析表。

## 6. 整体执行流程

```text
启动 syntax_analysis_top_down
        |
        v
检查 ../lexical_analysis/output/error.txt
        |
        +-- 非空 -> 输出“词法分析未通过”，结束
        |
        v
读取 ../lexical_analysis/output/result.txt
        |
        v
解析 token 行：(lexeme, token_code)
        |
        v
按 SEP_SEMICOLON 切分多条表达式
        |
        v
把每条表达式归一化为 i + * ( ) # 序列
        |
        v
对每条表达式执行 LL(1) 预测分析
        |
        v
根据分析栈和预测分析表判断正确 / 错误
        |
        v
对语法正确的表达式尝试求值
        |
        +-- 纯整数表达式 -> 得到计算结果
        |
        +-- 含 i 的表达式 -> 保留“正确”，但说明无法计算具体值
        |
        v
写 output/result.txt 和 output/error.txt
        |
        v
命令行打印分析摘要
```

## 7. 后续实现顺序建议

建议后续按以下顺序实现：

1. `grammar`：先把 LL(1) 文法、产生式和预测分析表固定下来。
2. `parser`：实现教材图 4.4 对应的栈驱动预测分析算法。
3. `token_reader`：读取词法分析输出，并完成 token 到语法终结符的归一化。
4. `evaluator`：在表达式语法正确后，根据 `InputToken.lexeme` 尝试计算纯整数表达式。
5. `result_writer`：写入 `result.txt`、`error.txt` 和 `steps.txt`，并在正确表达式后输出计算结果或无法计算原因。
6. `main.cpp`：组装完整流程。
7. 更新 `CMakeLists.txt`：新增 `syntax_analysis_top_down_lib`，并让 executable 链接该 library。

这样可以先保证核心预测分析算法正确，再处理文件输入输出和工程集成。
