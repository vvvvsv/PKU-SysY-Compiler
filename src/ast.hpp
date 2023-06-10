#pragma once
#include <memory>
#include <string>
#include <vector>
#include "symtable.hpp"

// 所有 AST 的基类
class BaseAST {
 public:
  // 类中的type表示是第几个生成式, type=1表示最左侧第一个生成式, 依此类推.
  // xxx1_yyy2 表示在 type 为 1 时为 xxx, 在 type 为 2 时为 yyy.
  virtual ~BaseAST() = default;
  // 输出 KoopaIR 到 stdout
  virtual void KoopaIR() const = 0;
};

/************************CompUnit*************************/

// CompUnit ::= CompUnitItemList;
// CompUnitItemList ::= CompUnitItem | CompUnitItemList CompUnitItem;
class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > comp_unit_item_list;
  void KoopaIR() const override;
};

// CompUnitItem ::= Decl | FuncDef;
class CompUnitItemAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> decl1_funcdef2;
  void KoopaIR() const override;
};

/**************************Decl***************************/

// Decl ::= ConstDecl | VarDecl | FuncDecl;
class DeclAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> const_decl1_var_decl2_func_decl3;
  void KoopaIR() const override;
};

// ConstDecl ::= "const" TYPE ConstDefList ";";
// ConstDefList ::= ConstDef | ConstDefList "," ConstDef;
class ConstDeclAST : public BaseAST {
 public:
  std::string b_type;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > const_def_list;
  void KoopaIR() const override;
};


// ConstDef ::= IDENT ConstIndexList "=" ConstInitVal;
// ConstIndexList ::=  | ConstIndexList "[" ConstExp "]";
class ConstDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > const_index_list;
  std::unique_ptr<BaseAST> const_init_val;
  void KoopaIR() const override;
};

// ConstInitVal ::= ConstExp | ConstArrayInitVal;
// ConstArrayInitVal ::= "{" "}" | "{" ConstInitValList "}";
// ConstInitValList ::= ConstInitVal | ConstInitValList "," ConstInitVal;
class ConstInitValAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> const_exp;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > const_init_val_list;
  void KoopaIR() const override;
  int Calc() const;
  // 计算并补全初始化列表
  // 常量初始化列表中只能出现常量表达式, 返回的 vector<int> 中即为补足 0 的各项的值
  std::vector<int> Aggregate(std::vector<int>::iterator len_begin,
    std::vector<int>::iterator len_end) const;
};

// VarDecl ::= TYPE VarDefList ";";
// VarDefList ::= VarDef | VarDefList "," VarDef;
class VarDeclAST : public BaseAST {
 public:
  std::string b_type;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > var_def_list;
  void KoopaIR() const override;
};

// VarDef ::= IDENT ConstIndexList | IDENT ConstIndexList "=" InitVal;
// ConstIndexList ::=  | ConstIndexList "[" ConstExp "]";
class VarDefAST : public BaseAST {
 public:
  int type;
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > const_index_list;
  std::unique_ptr<BaseAST> init_val;
  void KoopaIR() const override;
};

// InitVal ::= Exp | ArrayInitVal;
// ArrayInitVal ::= "{" "}" | "{" InitValList "}";
// InitValList ::= InitVal | InitValList "," InitVal;
class InitValAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > init_val_list;
  void KoopaIR() const override;
  int Calc() const;
  // 计算并补全初始化列表
  // 全局数组变量的初始化列表中只能出现常量表达式,
  // 返回的 vector<int> 中即为补足 0 的各项的值.
  // 局部数组变量的初始化列表中可以出现任何表达式,
  // 返回的 vector<int> 为各项的 KoopaIR Symbol 编号.
  // 若编号为 -1, 则对应的值为 0.
  std::vector<int> Aggregate(std::vector<int>::iterator len_begin,
    std::vector<int>::iterator len_end) const;
};

// FuncDecl ::= TYPE IDENT "(" FuncFParams ")" ";";
// FuncFParams ::=  | FuncFParamList;
// FuncFParamList ::= FuncFParam | FuncFParamList "," FuncFParam;
class FuncDeclAST : public BaseAST {
 public:
  std::string func_type;
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > func_f_param_list;
  void KoopaIR() const override;
};


/**************************Func***************************/

// FuncDef ::= TYPE IDENT "(" FuncFParams ")" Block;
// FuncFParams ::=  | FuncFParamList;
// FuncFParamList ::= FuncFParam | FuncFParamList "," FuncFParam;
class FuncDefAST : public BaseAST {
 public:
  std::string func_type;
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > func_f_param_list;
  std::unique_ptr<BaseAST> block;
  void KoopaIR() const override;
};

// FuncFParam ::= TYPE IDENT | TYPE IDENT "[" "]" ConstIndexList;
class FuncFParamAST : public BaseAST {
  // 该函数在 type==2 时可用, 返回该参数的类型. 例子:
  // int arr[]        -> *i32
  // int arr[][3]     -> *[i32, 3]
  // int arr[][2][3]  -> *[[i32, 3], 2]
  std::string ParamType() const;
 public:
  int type;
  std::string b_type;
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > const_index_list;
  void KoopaIR() const override;
  void Alloc() const;
};

/**************************Block***************************/

// Block ::= "{" BlockItemList "}";
// BlockItemList ::=  | BlockItemList BlockItem;
class BlockAST : public BaseAST {
 public:
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > block_item_list;
  void KoopaIR() const override;
};

// BlockItem ::= Decl | Stmt;
class BlockItemAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> decl1_stmt2;
  void KoopaIR() const override;
};

// Stmt ::= LVal "=" Exp ";"
class StmtAssignAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lval;
  std::unique_ptr<BaseAST> exp;
  void KoopaIR() const override;
};

//        | ";"
//        | Exp ";"
class StmtExpAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> exp;
  void KoopaIR() const override;
};

//        | Block
class StmtBlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> block;
  void KoopaIR() const override;
};

//        | "if" "(" Exp ")" Stmt
//        | "if" "(" Exp ")" Stmt "else" Stmt
// 此处有移进/归约冲突, SysY 的语义规定了 else 必须和最近的 if 进行匹配
// 则此处应选择发生冲突时应选择移进，即选择第二条规则
// 在 sysy.y 中如下实现：
// %precedence IFX
// %precedence ELSE
// IF '(' Exp ')' Stmt %prec IFX
// IF '(' Exp ')' Stmt ELSE Stmt
class StmtIfAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt_if;
  std::unique_ptr<BaseAST> stmt_else;
  void KoopaIR() const override;
};

//        | "while" "(" Exp ")" Stmt
class StmtWhileAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt;
  void KoopaIR() const override;
};

//        | "break" ";"
class StmtBreakAST : public BaseAST {
 public:
  void KoopaIR() const override;
};

//        | "continue" ";"
class StmtContinueAST : public BaseAST {
 public:
  void KoopaIR() const override;
};

//        | "return" ";";
//        | "return" Exp ";";
class StmtReturnAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> exp;
  void KoopaIR() const override;
};

/***************************Exp***************************/

class ExpBaseAST : public BaseAST {
 public:
  virtual int Calc() const = 0;
};

// Exp ::= LOrExp;
class ExpAST : public ExpBaseAST {
 public:
  std::unique_ptr<BaseAST> lorexp;
  void KoopaIR() const override;
  int Calc() const override;
};

// LVal ::= IDENT IndexList;
// IndexList ::=  | IndexList "[" Exp "]";
class LValAST : public ExpBaseAST {
 public:
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > index_list;
  void KoopaIR() const override;
  int Calc() const override;
};

// PrimaryExp ::= "(" Exp ")" | LVal | Number;
class PrimaryExpAST : public ExpBaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> exp1_lval2;
  int number;
  void KoopaIR() const override;
  int Calc() const override;
};

// UnaryExp ::= PrimaryExp | FuncExp | UnaryOp UnaryExp;
// UnaryOp ::= "+" | "-" | "!"
class UnaryExpAST : public ExpBaseAST {
 public:
  int type;
  char unaryop;
  std::unique_ptr<BaseAST> primaryexp1_funcexp2_unaryexp3;
  void KoopaIR() const override;
  int Calc() const override;
};

// FuncExp ::= IDENT "(" FuncRParams ")";
// FuncRParams ::=  | FuncRParamList;
// FuncRParamList ::= Exp | FuncRParamList "," Exp;
// Func exps cannot be calculated at compile time,
// so that FuncExpAST doesn't need a Calc() function.
class FuncExpAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > func_r_param_list;
  void KoopaIR() const override;
};

// MulExp ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp ::= "*" | "/" | "%"
class MulExpAST : public ExpBaseAST {
 public:
  int type;
  char mulop;
  std::unique_ptr<BaseAST> mulexp;
  std::unique_ptr<BaseAST> unaryexp;
  void KoopaIR() const override;
  int Calc() const override;
};

// AddExp ::= MulExp | AddExp AddOp MulExp;
// AddOp ::= "+" | "-"
class AddExpAST : public ExpBaseAST {
 public:
  int type;
  char addop;
  std::unique_ptr<BaseAST> addexp;
  std::unique_ptr<BaseAST> mulexp;
  void KoopaIR() const override;
  int Calc() const override;
};

// RelExp ::= AddExp | RelExp RelOp AddExp;
// RelOp ::= "<" | ">" | "<=" | ">="
class RelExpAST : public ExpBaseAST {
 public:
  int type;
  std::string relop;
  std::unique_ptr<BaseAST> relexp;
  std::unique_ptr<BaseAST> addexp;
  void KoopaIR() const override;
  int Calc() const override;
};

// EqExp ::= RelExp | EqExp EqOp RelExp;
// EqOp ::= "==" | "!="
class EqExpAST : public ExpBaseAST {
 public:
  int type;
  std::string eqop;
  std::unique_ptr<BaseAST> eqexp;
  std::unique_ptr<BaseAST> relexp;
  void KoopaIR() const override;
  int Calc() const override;
};

// LAndExp ::= EqExp | LAndExp "&&" EqExp;
class LAndExpAST : public ExpBaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> landexp;
  std::unique_ptr<BaseAST> eqexp;
  void KoopaIR() const override;
  int Calc() const override;
};

// LOrExp  ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpAST : public ExpBaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> lorexp;
  std::unique_ptr<BaseAST> landexp;
  void KoopaIR() const override;
  int Calc() const override;
};

// ConstExp ::= Exp;
class ConstExpAST : public ExpBaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  void KoopaIR() const override;
  int Calc() const override;
};