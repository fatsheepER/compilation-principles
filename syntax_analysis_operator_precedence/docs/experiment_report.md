# 算符优先语法分析程序实验报告

## 一、实验目的

1. 根据算符优先分析法，对表达式进行语法分析，使程序能够判断一个表达式是否符合给定表达式文法。
2. 通过实现算符优先分析方法，理解自下而上语法分析中“移进”和“归约”的基本过程，加深对句柄、最左素短语、优先关系表等概念的认识。
3. 在已有词法分析程序输出的基础上完成语法分析阶段，掌握编译前端不同阶段之间通过 token 文件传递信息的基本方法。
4. 通过输出 FirstVT 集、LastVT 集、算符优先关系表、分析步骤和错误信息，验证语法分析程序的中间结果与最终结果是否正确。

## 二、实验原理

### 1. 使用的文法

本实验使用的表达式文法如下：

```text
E -> E + T | T
T -> T * F | F
F -> ( E ) | i
```

其中，E 表示表达式，T 表示项，F 表示因子。终结符集合为：

```text
{ i, +, *, (, ), # }
```

# 为输入结束符，同时也作为分析栈的栈底符号。非终结符集合为：

```text
{ E, T, F }
```

开始符号为 E。

该文法能够描述由标识符或整数常量、加号、乘号以及括号组成的简单算术表达式。文法中 * 的优先级高于 +，括号可以改变默认结合结构。程序实际分析时，会将词法分析输出中的标识符 i 和整数常量统一映射为文法终结符 i。

### 2. 算符优先分析的基本思想

算符优先分析是一种自下而上的语法分析方法。它不从开始符号向下推导输入串，而是从输入符号串出发，通过不断移进输入符号、识别栈顶附近的可归约短语，并将短语归约为非终结符，最终判断输入串是否能够归约为开始符号。

算符优先分析的核心依据是终结符之间的优先关系。对于相邻或被一个非终结符间隔的终结符，可以根据文法确定它们之间的三类关系：

```text
a < b   表示 a 的优先级低于 b，分析时应继续移进 b
a = b   表示 a 和 b 同属某个短语边界，例如括号配对
a > b   表示 a 的优先级高于 b，分析时应尝试归约栈顶短语
```

分析时，程序比较“分析栈中最靠近栈顶的终结符”和“当前输入终结符”的优先关系：

1. 若关系为 < 或 =，则将当前输入符号移进分析栈。
2. 若关系为 >，则从栈顶向左查找最左素短语边界，并尝试将该短语归约为一个非终结符占位符 N。
3. 若不存在优先关系，说明当前输入符号与栈内符号不能构成合法表达式结构，分析出错。
4. 当输入符号为 #，分析栈为 #N 时，说明整个表达式已经成功归约，分析结束。

本实验中，程序没有在归约阶段区分具体归约为 E、T 或 F，而是统一归约为非终结符占位符 N。这是因为算符优先分析的重点是通过终结符优先关系确定移进和归约时机，归约模式只需要判断短语形态是否符合表达式文法即可。

### 3. 求 FirstVT 和 LastVT 集

FirstVT(P) 表示从非终结符 P 推导出的句型中，可能出现在最左端的终结符集合。它用于确定某个终结符后面跟着一个非终结符时，该终结符应当小于哪些可能首先出现的终结符。

求 FirstVT 集的规则如下：

1. 若存在产生式 P -> a ...，其中 a 是终结符，则 a 属于 FirstVT(P)。
2. 若存在产生式 P -> Q a ...，其中 Q 是非终结符、a 是终结符，则 a 属于 FirstVT(P)。
3. 若存在产生式 P -> Q ...，则 FirstVT(Q) 中的所有终结符都属于 FirstVT(P)。

LastVT(P) 表示从非终结符 P 推导出的句型中，可能出现在最右端的终结符集合。它用于确定某个非终结符后面跟着一个终结符时，哪些可能最后出现的终结符应当大于后面的终结符。

求 LastVT 集的规则如下：

1. 若存在产生式 P -> ... a，其中 a 是终结符，则 a 属于 LastVT(P)。
2. 若存在产生式 P -> ... a Q，其中 a 是终结符、Q 是非终结符，则 a 属于 LastVT(P)。
3. 若存在产生式 P -> ... Q，则 LastVT(Q) 中的所有终结符都属于 LastVT(P)。

程序采用迭代方式求解 FirstVT 和 LastVT。首先为每个非终结符建立空集合，然后反复扫描所有产生式，按照上述规则向集合中加入终结符。如果某一轮扫描中有新的元素被加入，则继续下一轮；当所有集合都不再变化时，计算结束。

本实验文法对应的集合为：

```text
FirstVT(E) = { i, +, *, ( }
FirstVT(T) = { i, *, ( }
FirstVT(F) = { i, ( }

LastVT(E) = { i, +, *, ) }
LastVT(T) = { i, *, ) }
LastVT(F) = { i, ) }
```

### 4. 生成算符优先关系表

在得到 FirstVT 和 LastVT 集后，可以根据文法产生式构造算符优先关系表。程序使用以下规则生成关系：

1. 若产生式右部出现相邻终结符 a b，则置 a = b。
2. 若产生式右部出现 a Q b，其中 Q 为非终结符，则置 a = b。
3. 若产生式右部出现 a Q，则对每个 b in FirstVT(Q)，置 a < b。
4. 若产生式右部出现 Q a，则对每个 b in LastVT(Q)，置 b > a。
5. 对开始符号 E，置 # < a，其中 a in FirstVT(E)。
6. 对开始符号 E，置 b > #，其中 b in LastVT(E)。
7. 置 # = #，用于分析成功时判断输入与栈底都到达结束状态。

生成关系时，如果同一对终结符已经存在一种关系，而新规则又试图加入另一种不同关系，则说明文法在该位置存在算符优先关系冲突。程序会记录冲突信息，并停止后续输入表达式分析。本实验所用文法生成的关系表不存在冲突。

## 三、实验完成情况

### 1. 功能完成情况

本实验程序已经完成以下功能：

1. 固定定义实验所需表达式文法，包括非终结符、终结符和六条产生式。
2. 自动计算文法中每个非终结符的 FirstVT 和 LastVT 集。
3. 根据文法和 VT 集自动生成算符优先关系表，并检查关系冲突。
4. 复用公共 TokenReader 模块读取词法分析程序生成的 token 文件。
5. 按分号将 token 文件切分为多条表达式，并为每条表达式补充结束符 #。
6. 将词法 token 映射到本实验文法终结符，包括将整数常量映射为 i。
7. 对每条表达式执行算符优先分析，输出正确或错误结论。
8. 对错误表达式输出错误原因和错误 token 在 token 文件中的来源行号。
9. 输出完整分析步骤，包括分析栈、剩余输入、优先关系和当前动作。
10. 输出 FirstVT / LastVT 集、算符优先关系表、结果文件、错误文件和步骤文件。

程序目前支持的表达式符号范围为：

```text
i, 整数常量, +, *, (, ), ;
```

其中分号 ; 不属于表达式文法本身，而是作为输入文件中多条表达式的分隔符。

### 2. 程序流程图

以下位置预留给已经绘制好的八个流程图：

> 【流程图 1 占位：算符优先语法分析程序总体流程图】

> 【流程图 2 占位：TokenReader 读取词法分析输出并切分表达式流程图】

> 【流程图 3 占位：FirstVT 集计算流程图】

> 【流程图 4 占位：LastVT 集计算流程图】

> 【流程图 5 占位：算符优先关系表生成流程图】

> 【流程图 6 占位：单条表达式算符优先分析流程图】

> 【流程图 7 占位：归约时最左素短语查找流程图】

> 【流程图 8 占位：结果文件、步骤文件和错误文件输出流程图】

### 3. 工程项目组织与各模块说明

本实验位于工程目录 syntax_analysis_operator_precedence/ 下，同时复用了公共模块 syntax_analysis_common/。项目主要结构如下：

```text
syntax_analysis_operator_precedence/
├── CMakeLists.txt
├── docs/
│   ├── task.md
│   ├── analysis.md
│   ├── program_design.md
│   └── experiment_report.md
├── output/
│   ├── result.txt
│   ├── error.txt
│   ├── first_last_vt.txt
│   ├── precedence_table.txt
│   └── steps.txt
└── src/
    ├── grammar.h / grammar.cpp
    ├── first_last_vt.h / first_last_vt.cpp
    ├── precedence_table.h / precedence_table.cpp
    ├── parser.h / parser.cpp
    ├── result_writer.h / result_writer.cpp
    └── main.cpp
```

各模块职责如下：

grammar.h / grammar.cpp：定义实验文法的数据结构，包括 Symbol、Production、Grammar、NonTerminal 等类型，并在 Grammar 构造函数中初始化本实验使用的六条产生式。

first_last_vt.h / first_last_vt.cpp：定义 FirstLastVTResult 和 FirstLastVTCalculator，负责根据文法自动计算各非终结符的 FirstVT 集和 LastVT 集。

precedence_table.h / precedence_table.cpp：定义算符优先关系、优先关系表、冲突信息和关系表构造器。该模块根据产生式、FirstVT 和 LastVT 生成完整的算符优先关系表。

parser.h / parser.cpp：实现算符优先分析算法。该模块维护分析栈，比较栈顶终结符和当前输入符号之间的优先关系，执行移进或归约，并记录每一步分析过程。

result_writer.h / result_writer.cpp：负责将实验结果写入 output/ 目录，包括分析结论、错误信息、FirstVT / LastVT 集、优先关系表和步骤表。

main.cpp：程序入口，负责串联完整流程：初始化文法、计算 VT 集、构造优先表、读取 token 文件、逐条表达式分析并输出结果。

syntax_analysis_common/：公共模块，提供 Terminal、InputToken、TokenReader 等类型和工具函数。本实验只复用 token 读取与终结符抽象，不复用自顶向下语法分析实验中的文法和分析器。

CMake 中定义了三个相关目标：

```text
syntax_analysis_common
syntax_analysis_operator_precedence_lib
syntax_analysis_operator_precedence
```

此外还定义了运行目标：

```text
run_syntax_analysis_operator_precedence
```

该目标运行时的工作目录为 syntax_analysis_operator_precedence/，因此程序默认读取：

```text
../lexical_analysis/output/result.txt
../lexical_analysis/output/error.txt
```

并默认将输出写入：

```text
syntax_analysis_operator_precedence/output/
```

## 四、核心算法和代码

本实验程序复用了第 2 次实验设计的用于词法分析程序输出 token 文件读取与解析的 TokenReader 模块，并使用了基本一样的方法定义了文法 Grammar，因此本文省略相关代码细节介绍，只说明本实验核心算法。

### 1. FirstVT 与 LastVT 集的计算与表示

程序使用 std::map<NonTerminal, std::set<Terminal>> 表示每个非终结符对应的 VT 集。FirstLastVTResult 中分别保存 first_vt 和 last_vt 两张映射表。

计算过程采用不动点迭代思想。初始化时，程序为每个非终结符创建空集合。随后在循环中反复遍历所有产生式：

1. 根据产生式右部开头的符号形式，向 FirstVT 集直接加入终结符。
2. 根据产生式右部末尾的符号形式，向 LastVT 集直接加入终结符。
3. 如果产生式右部第一个符号是非终结符，则将该非终结符的 FirstVT 集传播到左部非终结符。
4. 如果产生式右部最后一个符号是非终结符，则将该非终结符的 LastVT 集传播到左部非终结符。
5. 若本轮有任意集合发生变化，则继续迭代；否则说明所有集合已经稳定，计算结束。

这种方法的优点是实现直接、逻辑清晰，不需要手工为每个非终结符推导集合。只要文法产生式数据结构保持一致，就可以自动计算 VT 集。

> 【代码占位：syntax_analysis_operator_precedence/src/first_last_vt.h 中 FirstLastVTResult 与 FirstLastVTCalculator 的定义】

> 【代码占位：syntax_analysis_operator_precedence/src/first_last_vt.cpp 中 FirstLastVTCalculator::calculate、addFirstVTByProduction、addLastVTByProduction、propagateFirstVT、propagateLastVT 的实现】

### 2. 算符优先关系表的生成与表示

程序使用 PrecedenceRelation 表示三类优先关系：

```text
Less      对应 <
Equal     对应 =
Greater   对应 >
```

优先关系表使用 std::map<std::pair<Terminal, Terminal>, PrecedenceRelation> 保存。键为两个终结符构成的有序对，值为这两个终结符之间的优先关系。如果某一对终结符没有出现在表中，则表示它们之间不存在合法优先关系。

构造优先关系表时，程序遍历每一条产生式的右部，并检查以下几类局部符号模式：

1. a b：两个终结符相邻，加入 a = b。
2. a Q b：两个终结符中间隔着一个非终结符，加入 a = b。
3. a Q：终结符后接非终结符，加入 a < FirstVT(Q) 中每个终结符。
4. Q a：非终结符后接终结符，加入 LastVT(Q) 中每个终结符 > a。

遍历完全部产生式后，程序再补充结束符 # 与开始符号 E 之间的关系：

```text
# < FirstVT(E)
LastVT(E) > #
# = #
```

每次插入关系时，程序都会检查该终结符对是否已经存在关系。如果已经存在相同关系，则保持不变；如果已经存在不同关系，则记录为冲突。这样可以在分析输入表达式之前发现文法或关系表构造中的问题。

> 【代码占位：syntax_analysis_operator_precedence/src/precedence_table.h 中 PrecedenceRelation、PrecedenceTable、PrecedenceTableBuildResult 的定义】

> 【代码占位：syntax_analysis_operator_precedence/src/precedence_table.cpp 中 PrecedenceTableBuilder::build、addRelationsFromProduction、addEndMarkerRelations、addRelation 的实现】

### 3. 算符优先分析算法总控程序设计

单条表达式的分析由 OperatorPrecedenceParser::parse 完成。程序首先将输入 token 序列复制到局部变量中，如果末尾没有结束符 #，则自动补充 #。随后初始化分析栈，栈底为 #。

每一轮分析的主要步骤如下：

1. 记录当前分析栈、剩余输入和步骤编号。
2. 在分析栈中从右向左找到最靠近栈顶的终结符。
3. 如果当前输入符号为 #，且分析栈形态为 #N，则分析成功。
4. 查询栈顶终结符与当前输入终结符之间的优先关系。
5. 若不存在优先关系，则报告语法错误。
6. 若关系为 < 或 =，则执行移进操作，将当前输入符号压入分析栈。
7. 若关系为 >，则执行归约操作，从栈顶向左查找最左素短语边界。
8. 找到待归约短语后，判断该短语是否匹配合法归约模式。
9. 若短语合法，则删除栈中该短语并压入非终结符占位符 N。
10. 若短语不合法，则报告语法错误。

本实验允许的归约模式包括：

```text
i     -> N
N + N -> N
N * N -> N
( N ) -> N
```

在归约时，程序并不需要区分当前短语具体归约为 E、T 还是 F。由于优先关系表已经体现了 +、* 和括号之间的结合关系，归约模式只需要验证短语结构是否属于表达式文法允许的基本形态。

最左素短语的查找过程为：从分析栈中最靠近栈顶的终结符开始，不断向左寻找前一个终结符，并查询二者之间的优先关系。当第一次遇到 < 关系时，说明该 < 右侧到栈顶之间的内容就是当前需要归约的短语。

> 【代码占位：syntax_analysis_operator_precedence/src/parser.h 中 OperatorPrecedenceParseResult、OperatorPrecedenceStep、OperatorPrecedenceParser 的定义】

> 【代码占位：syntax_analysis_operator_precedence/src/parser.cpp 中 OperatorPrecedenceParser::parse 的主循环实现】

> 【代码占位：syntax_analysis_operator_precedence/src/parser.cpp 中 findHandle、canReduceToN、isAcceptStack 的实现】

### 4. 结果输出

实验输出由 OperatorPrecedenceResultWriter 统一完成。程序会先创建输出目录，然后打开五个输出文件：

```text
result.txt
error.txt
first_last_vt.txt
precedence_table.txt
steps.txt
```

各文件含义如下：

1. result.txt：记录每条表达式的最终分析结论，标明表达式正确或错误，并输出对应输入符号串。
2. error.txt：记录错误表达式的错误原因。如果没有错误，则输出“无错误”。
3. first_last_vt.txt：记录每个非终结符的 FirstVT 集和 LastVT 集。
4. precedence_table.txt：以矩阵形式输出算符优先关系表。
5. steps.txt：记录每条表达式的详细分析过程，包括步骤编号、分析栈、剩余输入、优先关系和动作。

如果优先关系表构造失败，程序会在结果文件中说明未执行输入表达式分析，并在错误文件中输出冲突信息。如果词法分析错误文件非空，程序也不会继续进行语法分析，而是将词法错误写入本实验的错误文件中。

> 【代码占位：syntax_analysis_operator_precedence/src/result_writer.h 中结果报告结构体和 OperatorPrecedenceResultWriter 的定义】

> 【代码占位：syntax_analysis_operator_precedence/src/result_writer.cpp 中 write、writeFirstLastVT、writePrecedenceTable、writeSteps 的实现】

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

如果词法错误文件为空，说明词法分析通过，算符优先语法分析可以继续执行。当前测试输入中包含多条以分号分隔的表达式，既包括正确表达式，也包括存在括号不匹配、运算符连续、表达式缺少操作数等问题的错误表达式。

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

其中 1+2*3 用于验证整数常量能否被归一化为表达式文法中的终结符 i，并按照同一套算符优先关系完成分析。

### 2. 测试输出文件

程序运行后，在 syntax_analysis_operator_precedence/output/ 目录下生成五个文件：

```text
result.txt
error.txt
first_last_vt.txt
precedence_table.txt
steps.txt
```

#### 结果文件

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

这些结果表明程序能够识别普通表达式、带括号表达式、乘法优先于加法的表达式，以及整数常量表达式；同时也能够发现多余右括号、缺少右括号、连续运算符、开头运算符和结尾缺少操作数等错误。

#### 步骤文件

steps.txt 用于记录每条表达式的完整分析过程。每一行包含：

```text
步骤    分析栈    剩余输入    优先关系    动作
```

例如，对于表达式 (i+i)*i#，程序会先根据 # < (、( < i 连续移进，再根据 i > + 将 i 归约为 N，随后继续移进 + 和第二个 i。当遇到 + > ) 时，程序将 N+N 归约为 N；当遇到 ( = ) 时移进右括号；随后根据 ) > * 将 (N) 归约为 N。最后，程序将 N*N 归约为 N，分析栈变为 #N，输入为 #，满足成功条件。

步骤文件的作用不仅是给出最终结论，还可以展示程序每一步移进和归约的依据，便于检查优先关系表和归约过程是否正确。

#### 错误文件

error.txt 用于记录错误表达式的具体错误原因。当前测试中，错误主要分为两类：

1. 当前栈顶终结符与输入终结符之间不存在优先关系。例如 i+i)*i 在处理多余的 ) 时，栈顶状态已经归约到 #N，但当前输入仍为 )，程序查询 # 和 ) 之间的关系失败，因此报告错误。
2. 找到了归约边界，但待归约短语不符合合法归约模式。例如 i+*i 中会形成 *N 这样的短语，该短语不能匹配 i、N+N、N*N 或 (N)，因此报告错误。

错误文件同时输出错误 token 在词法 token 文件中的来源行号，便于回到 lexical_analysis/output/result.txt 中定位问题 token。

## 六、实验总结

通过本实验，我完成了一个基于算符优先分析法的表达式语法分析程序。程序能够根据固定表达式文法自动计算 FirstVT 集和 LastVT 集，自动生成算符优先关系表，并利用该关系表对多条输入表达式进行移进、归约和错误判断。

本实验进一步说明了自下而上分析和自顶向下分析的差异。自顶向下分析更关注从开始符号出发选择产生式，而算符优先分析更关注输入串和分析栈中终结符之间的优先关系。只要优先关系表构造正确，分析程序就可以根据 <、=、> 三种关系决定移进或归约，并逐步把输入表达式归约为一个非终结符。

本实验也体现了编译程序分阶段设计的好处。语法分析程序不需要重新扫描源程序，而是直接读取词法分析阶段生成的 token 文件。这样可以让词法分析和语法分析职责分离：词法分析负责识别单词符号，语法分析负责判断 token 序列是否符合表达式文法。

### 问题 1：词法 token 与表达式文法终结符不完全一致

#### 1. 问题描述

词法分析程序输出的是具体 token，例如标识符、整数常量、运算符、分隔符等；而本实验的表达式文法只使用抽象终结符 i、+、*、(、) 和 #。两者并不是完全一一对应的关系。

例如，词法分析输出中的整数常量 1、2、3 在表达式文法中都应该被当作操作数处理，也就是归一化为终结符 i。同时，词法分析程序可能输出本实验文法不支持的 token，例如其他标识符、其他运算符或其他分隔符，这些 token 不能直接进入算符优先分析过程。

#### 2. 问题分析

如果语法分析程序直接使用词法 token 的原始种别码，就会使文法和分析表变得复杂。例如，所有整数常量都要在文法中单独处理，这不符合表达式文法中用 i 表示操作数的抽象方式。

同时，如果不在读取阶段过滤不支持的 token，那么后续算符优先关系表中就找不到这些符号对应的优先关系，错误会延迟到分析阶段才暴露，错误信息也不够明确。

因此，语法分析阶段需要在读取 token 文件时增加一个归一化步骤，把词法 token 转换成本实验文法中的终结符。

#### 3. 解决方案

程序在公共 TokenReader 模块中完成 token 映射：

1. 标识符 i 映射为终结符 i。
2. 整数常量映射为终结符 i。
3. 加号、乘号、左括号、右括号分别映射为 +、*、(、)。
4. 分号 ; 用于切分表达式，并为当前表达式补充结束符 #。
5. 不属于本实验文法范围的 token 会被记录为输入错误，该表达式不再执行算符优先分析。

通过这个设计，语法分析器只需要面对文法层面的终结符，不需要关心词法阶段的所有 token 细节。同时，程序仍然保留 token 文件来源行号，使错误定位可以回到词法分析输出文件。

### 尚未解决或可以改进的问题

当前程序可以完成本实验要求，但仍有一些可以继续改进的地方：

1. 当前文法是固定写在 Grammar 构造函数中的，后续可以考虑从配置文件或文法描述文件中读取产生式。
2. 当前归约时统一使用非终结符占位符 N，没有输出具体归约产生式。如果希望分析步骤更接近教材推导过程，可以在归约时进一步区分 E、T、F。
3. 当前错误位置记录的是 token 在 lexical_analysis/output/result.txt 中的行号，不是源程序中的原始行列号。如果词法分析阶段保留源代码位置，语法分析阶段的错误定位可以更加直观。
4. 当前程序主要面向 +、* 和括号表达式。若后续扩展减法、除法、一元运算符或更多表达式类型，需要同步扩展文法、终结符映射、优先关系表构造和归约模式。
