# PKU 编译 lab 2023 - SysY 编译器教程

本 repo 为 2023 年春季学期北京大学编译原理课程的 Lab。该 Lab 设计相当完善，按照 MaxXing 学长写的 [在线文档](https://pku-minic.github.io/online-doc/#/)，进行一些适量的思考，写一些适量的代码就可以完成这个 Lab。卡住的时候可以考虑看看 [这篇教程](https://www.cnblogs.com/zhangleo/p/15963442.html)。


## 关于配环境

按教程配好环境就 OK 了。对于使用 Windows 系统的同学，DockerDesktop 的软件本体必须装在 C 盘（系统盘），但是镜像和容器在安装后可以移动到 D 盘，详见 [How can I change the location of docker images when using Docker Desktop on WSL2 with Windows 10 Home?](https://stackoverflow.com/questions/62441307/how-can-i-change-the-location-of-docker-images-when-using-docker-desktop-on-wsl2/63752264#63752264)

后面部分的教程懒得写了，查看 commits 记录就能大概看到每个 lv 是怎么写的。可以大概看看：

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：

1. 将 SysY 语言编译为 Koopa IR。
2. 将 SysY 语言编译为 RISC-V 汇编。

## 二、编译器设计

### 2.1 主要模块组成

编译器由4个主要模块组成。词法分析器部分负责对 SysY 源程序进行词法分析，得到 token 流。语法分析器部分负责对 token 流进行语法分析，并生成对应 AST。IR 生成部分负责在 AST 上进行语义分析，建立符号表，并生成 Koopa IR。目标代码生成部分负责遍历内存形式的 IR，并生成汇编代码。

### 2.2 主要数据结构

本编译器最核心的数据结构是 AST，其定义在 `src/ast.hpp`。语法分析器生成的 AST 即为整个 SysY 源程序的抽象语法树。

如果将一个 SysY 程序视作一棵树，那么一个 `class CompUnitAST` 的实例就是这棵树的根。该树的每个结点都是一个 `BaseAST` 的派生类的实例。树上所有节点都有 `void KoopaIR()` 方法，用于进行语义分析并生成 Koopa IR；`ConstExp` 及其推导出的其他非终结符对应的节点都有 `int Calc()` 方法，用于在编译器计算出常量表达式的值。因此 `BaseAST` 的派生关系如下：

```
                   BaseAST (含虚函数 KoopaIR)
                     /    \
All ASTs except ExpASTs   ExpBaseAST (含虚函数 Calc)
                             |
                          All ExpASTs
```

例如， `CompUnitAST` 的定义如下，其中 `comp_unit_item_list` 中的每项都是一个指向 `CompUnitItemAST` 实例的智能指针。

```cpp
// CompUnit ::= CompUnitItemList;
// CompUnitItemList ::= CompUnitItem | CompUnitItemList CompUnitItem;
class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > comp_unit_item_list;
  void KoopaIR() const override;
};
// CompUnitItem ::= Decl | FuncDef;
```

在实际代码编写过程中，我设计了基于 `std::vector` 和 `std::unordered_map` 的符号表用来保存 SysY 程序中的符号，具体定义见 `symtable.hpp`。我为 SysY 程序中的每个作用域都创建了一个类型为 `std::unordered_map` 的符号表。在合法的 SysY 程序中，作用域形成了一个树形结构，而每个时刻需要保存的数个作用域形成了树上从根到某个节点的一条链，这条链可以用一个栈来维护。

```cpp
// 进入新的作用域
void enter_code_block()
{
  symbol_table_t* ptr = new symbol_table_t();
  symbol_table_stack.push_back(std::make_pair(symbol_table_cnt, ptr));
  symbol_table_cnt++;
}
// 离开当前作用域
void exit_code_block()
{
  delete symbol_table_stack.back().second;
  symbol_table_stack.pop_back();
}
```

在 `if...else...` 语句方面，由于涉及到二义性问题，所以我借助 bison 中的符号优先级进行了处理。在 LR 分析中，空悬 else 问题表现为移进/归约冲突，在 ` "if" "(" Exp ")" Stmt "if" "(" Exp ")" Stmt . "else" Stmt` 时无法确定进行移进（else 与较近的 if 匹配）还是规约（else 与较远的 if 匹配）。因为 SysY 的语义规定了 else 必须和最近的 if 进行匹配，所以在这里发生冲突时应选择移进。bison 中规则的优先级等于其中最后一个终结符的优先级，所以只要通过设置空终结符并规定优先级即可。具体代码如下：

```yacc
 /*设置ELSE的优先级大于IFX*/
%precedence IFX
%precedence ELSE
 /*...*/
Stmt
  : /*...*/
  | IF '(' Exp ')' Stmt %prec IFX {/*...*/}
  | IF '(' Exp ')' Stmt ELSE Stmt {/*...*/}
```

除此之外，为了代码编写的便利性，前端和后端部分都使用了一些全局变量。

```cpp
/*ast.cpp*/
// 计数 koopa 语句的返回值 %xxx
static int koopacnt = 0;
// 计数 if 语句, 用于设置 entry
static int ifcnt = 0;
// 计数 while 语句, 用于设置 entry
static int whilecnt = 0;
// ...

/*visitraw.cpp*/
// 类型为 koopa_raw_value 的有返回值的语句 -> 该语句相对于 sp 的存储位置
static std::unordered_map<koopa_raw_value_t, int> loc;
// 栈帧长度
static int stack_frame_length = 0;
// 已经使用的栈帧长度
static int stack_frame_used = 0;
// ...
```

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑

在合法的 SysY 程序中，作用域形成了一个树形结构，而每个时刻需要保存的数个作用域是树上从根到某个节点的一条链，这条链可以用一个栈来维护。栈使用 `std::vector` 实现，进入作用域时压栈，离开作用域时出栈。栈中每个元素都是一个 `std::pair<int, symbol_table_t*>`。该pair中，前者是该符号表的编号，用于在 Koopa IR 中区分不同作用域的同名变量；后者是一个指向类型为 `std::unordered_map<std::string, std::shared_ptr<symbol_value> >` 的符号表的指针。符号表的 key 是变量名字符串，value 是一个指向 symbol_value 实例的智能指针。symbol_value 中包含了符号的类型，若类型为常量则还包括符号的值。插入符号时，插入到栈顶的符号表中；查询符号时，从栈顶到栈底逐个查询。

#### 2.3.2 寄存器分配策略

没有进行寄存器分配，把所有局部变量都放在栈上，单条 Koopa IR 指令中需要用的时候再从内存读入寄存器。Koopa IR 指令结束后，如果该指令有返回值，则把返回值写入栈中。（不分配寄存器！）

#### 2.3.3 采用的优化策略

几乎没有进行优化，仅在访问 `sp + imm` 处内存的时候，若 imm 在 12 位以下则使用 addi，否则先用 li 将 imm 读入寄存器，再使用 add 指令。

#### 2.3.4 其它补充设计考虑

在 **Lv9+.1. 你的编译器超强的** 中的 mandelbrot 样例需要使用函数声明的语法，我在SysY语言的基础上加入了该语法：

```ebnf
Decl ::= ConstDecl | VarDecl | FuncDecl;
FuncDecl ::= FuncType IDENT "(" [FuncFParams] ")" ";";
```

## 三、编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和Lv2. 初试目标代码生成

Lv1 和 Lv2 实现了将只有一个 main 函数，main 函数里只有一个 return 语句的 SysY 程序编译到 Koopa IR，或者 RISC-V 代码。块状注释的文法规则为：

```flex
BlockComment  "/*"(.| |\t|\n|\r)*?"*/"
```

Lv1 的 15_misc2 在线测试点貌似是有嵌套作用域，总之写完 Lv5 的作用域就过了。

#### Lv3. 表达式

Lv3 各个表达式的文法按照运算符优先级排列，其AST生成代码、语义分析代码等具有高度同质性，嫌麻烦的话可以写一个生成表达式非终结符相关的 bison 和 cpp 代码的脚本。这部分只要小心对待优先级就不容易产生bug。

在 Lv3 中在线文档中，目标代码生成的思路是每条指令的结果都占用一个临时寄存器，所以我没有做 Lv3 的目标代码生成部分，直接在 lv4 和变量一起做完，这样不用修改寄存器分配策略。关于 `&&` 和 `||` 逻辑运算，因为riscv指令中没有逻辑与或，只有按位与或，所以要进行如下变换：

```
A&&B => (A!=0)&(B!=0)
A||B => (A!=0)|(B!=0)
```

#### Lv4. 常量和变量

在 Lv4 中引入了符号表，这时因为只有一个作用域，所以符号表只有一个 `std::unordered_map`。每个符号在符号表中都有一个类型和一个值，类型是常量或者变量。如果类型为常量，则值为其编译期求出的常量值，也就是调用 `Calc()` 方法的返回值；如果类型为变量，则值为0。

```cpp
typedef std::unordered_map<std::string, std::shared_ptr<symbol_value> > symbol_table_t;
static symbol_table_t symbol_table;

// 在符号表中寻找符号，返回其iterator
static symbol_table_t::iterator find_iter(const std::string &symbol)
{
  return symbol_table.find(symbol);
}
```

在目标代码生成时，同样使用一个 `std::unordered_map`，记录类型为 `koopa_raw_value` 的语句的返回值在栈中相对 sp 的位置。语句结束时将返回值存入栈中，此后别的语句以该语句为参数时再从栈中读入寄存器。

```cpp
// 类型为 koopa_raw_value 的有返回值的语句的存储位置
static std::unordered_map<koopa_raw_value_t, std::string> loc;
//...
void Visit(/*...*/) {
  // ...
  // 将 t0 中的结果存入栈
  loc[value] = std::to_string(stack_frame_used) + "(sp)";
  stack_frame_used += 4;
  std::cout << "  sw t0, " << loc[value] << std::endl;
}
```

Lv4 的 18_multiple_returns2 应该是有多条 return 语句。只要保证 Koopa IR 中每个基本块中有且仅有一个 ret/br/jump 语句，且位于基本块最后，就可以通过这个测试点。

#### Lv5. 语句块和作用域

如 **2.2** 和 **2.3.1** 所述在作用域嵌套树上维护一条链，可以减少一部分编译器运行时申请的内存，并且也更易于维护。

#### Lv6. if语句

对于悬空else问题，龙书中提供的解决方案是

```ebnf
Stmt          ::= MatchedStmt;
                | OpenStmt;
MatchedStmt   ::= "if" "(" Exp ")" MatchedStmt "else" MatchedStmt;
                | OtherStmt;
OpenStmt      ::= "if" "(" Exp ")" Stmt;
                | "if" "(" Exp ")" MatchedStmt "else" OpenStmt;
```

但是这样做有点复杂，语法分析有点难写，所以我采用了 **2.2** 中说明的借助优先级来处理二义性的方法。

中间代码生成时，只需按照如下格式进行生成即可：

```
  br %0, %then, %else
%then:
  //! do something
  jump %end
%else:
  //! do something
  jump %end
%end:
//! do something
```

在生成目标代码时，jump 指令可以直接翻译为 j 指令。对于形如 `br a, addr1, addr0` 的指令要翻译为如下形式：

```riscv
  # load a to t0
  bnez t0, DOUBLE_JUMP_addr1
  j addr0
DOUBLE_JUMP_addr1:
  j addr1
```

这是因为 bnez 这个条件跳转指令的目标地址和当前 PC 的 offset 必须控制在8位以内，而 j 这个无条件跳转指令的目标地址和 PC 的 offset 可以达到11位。使用上述双重跳转的方法，当 br 指令和跳转目标离得很远时，编译器也可以正常工作。

在Lv6 中实现了每个基本块中有且仅有一个 ret/br/jump 语句，且位于基本块最后。在生成 ret 后不再在当前基本块中生成其他代码；如果到了基本块的结尾还没有 ret 语句，就补上一个 ret。

#### Lv7. while语句

中间代码生成时，只需按照如下格式进行生成即可。`continue` 语句可转为 `jump %while_entry`，`break` 语句可转为`jump %while_end`。

```
  jump %while_entry
%while_entry:
  %cond = Exp
  br %cond, %while_body, %while_end
%while_body:
  //! do something
  jump %while_entry
%while_end:
  //! do something
```

while之间可能会产生循环嵌套关系。为了在遇到 `continue` 或 `break` 时能确定当前应该跳到哪个 while 的对应标号，需要使用全局变量来维护循环嵌套树和当前语句在循环嵌套树中的位置。

```cpp
// 当前 while 语句的标号
static int whilecur = 0;
// 保存 while 树上的 parent 关系
static std::unordered_map<int, int> whilepar;
void StmtBreakAST::KoopaIR() const {
  std::cout << "  jump %STMTWHILE_END_" << whilecur << std::endl;
  entry_end = 1;
}
```

#### Lv8. 函数和全局变量

为函数和全局变量生成 Koopa IR 只需使用 `call` 和 `global alloc` 即可，有以下三个细节：

第一：`UnaryExp ::= PrimaryExp | FuncExp | UnaryOp UnaryExp` 中，`FuncExpAST` 应为 `BaseAST` 的派生类而不是 `BaseExpAST` 的派生类，因为它没有 `Calc()` 这个用于在编译期求值的方法。

第二：语义分析在遇到 `Decl` 时不确定是全局定义还是局部定义，因此`ast.cpp`中需要一个全局变量来确定当前SysY程序中是否在声明全局变量。

```cpp
// 当前是否在声明全局变量
static int declaring_global_var = 0;
```

第三：如下文法规则在 `"int" . IDENT blabla` 这个状态存在规约/规约冲突，无法确定将 `"int"` 规约为 `FuncType` 还是 `BType`。

```ebnf
FuncType      ::= "void" | "int";
BType         ::= "int";
VarDecl       ::= BType VarDef {"," VarDef} ";";
FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;
```

我的处理方法是直接把类型变成终结符，即：

```ebnf
VarDecl       ::= TYPE VarDef {"," VarDef} ";";
FuncDef       ::= TYPE IDENT "(" [FuncFParams] ")" Block;
```

其中 `TYPE` 是 lexer 输出的终结符，可以携带 `"void"` 或 `"int"`字符串。

生成目标代码有点复杂，但注意好以下几点就没问题：

1. 按照文档正确生成函数的 prologue 和 epilogue。
2. 前8个参数放置在寄存器中，其他参数放置在栈中，函数开始时要把参数复制到自己的栈帧中方便处理
3. 处理 ret 语句时注意 a0 sp 和 ra 的设置。

因为选择了不分配寄存器的策略，所以也不需要考虑调用者保存寄存器/被调用者保存寄存器的问题。

#### Lv9. 数组

##### 中间代码生成

首先是把 SysY 的初始化列表转换为填好0的初始化列表，这一步我为 `ConstInitValAST` 和 `InitValAST` 添加了 `Aggregate` 方法，该方法返回一个 `std::vector<int>`。

```cpp
  // 计算并补全初始化列表
  // 常量数组或者全局变量数组的初始化列表中只能出现常量表达式，
  // 返回的 vector<int> 中即为补足 0 的各项的值。
  // 局部变量数组的初始化列表中可以出现任何表达式，
  // 返回的 vector<int> 为各项的 KoopaIR 临时符号。
  // 如初始化列表为 {%1, %2, %3}，则返回的vector为 [1,2,3]
  std::vector<int> Aggregate(std::vector<int>::iterator len_begin,
    std::vector<int>::iterator len_end) const;
```

在输出初始化列表时，也有几种可能：跟在 `alloc` 语句后面输出；逐个 `store` 立即数；逐个 `store` KoopaIR 临时符号。我定义了递归函数 `static void print_aggregate(...)` 用于输出。

在根据数组或者数组参数访问数组项时，按照定义依次使用 `getptr` 或 `getelemptr` 即可。

##### 目标代码生成

在后端中，需要存储每个数组、数组参数的维数和各维的长度，用于在访存时计算 offset，这可以用一个 `std::vector<int>` 来存储。Koopa IR 程序中 `getelemptr` 和 `getptr` 语句所生成的指针，一定对应上述数组/数组参数的 `vector` 中的一段，因此只需保存两个 `std::vector<int>::iterator` 即可。

```cpp
// 生成数组/指针的 (global) alloc 语句 -> 该数组/指针的维数.
// [[i32, 2], 3] -> 3, 2; *[[i32, 2], 3] ->  1, 3, 2
static std::unordered_map<koopa_raw_value_t, std::vector<int> > dimvec;
// getelemptr 和 getptr 语句 -> 生成的指针的维数,
// 表示为 dimvec 中的 vector 的某段的 begin 和 end
typedef std::pair<std::vector<int>::iterator, std::vector<int>::iterator> pvitvit;
static std::unordered_map<koopa_raw_value_t, pvitvit> dimlr;
```

这种做法不是很符合 Koopa IR 里的定义，比如无法处理 `***[i32, 2]` 类型的指针。这里只能保证能正确处理我的编译器生成的 Koopa IR 代码，不能保证能正确处理所有合法的Koopa IR 代码。传参和访存时需要一点特殊判断，细节不再赘述。
