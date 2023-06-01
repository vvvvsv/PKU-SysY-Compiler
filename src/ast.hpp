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

// CompUnit ::= FuncDef;
class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;
  void KoopaIR() const override;
};

/**************************Decl***************************/

// Decl ::= ConstDecl | VarDecl;
class DeclAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> const_decl1_var_decl2;
  void KoopaIR() const override;
};

// ConstDecl ::= "const" BType ConstDefList ";";
// ConstDefList ::= ConstDef | ConstDefList "," ConstDef;
class ConstDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> b_type;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > const_def_list;
  void KoopaIR() const override;
};

// BType ::= "int";
class BTypeAST : public BaseAST {
 public:
  void KoopaIR() const override;
};

// ConstDef ::= IDENT "=" ConstInitVal;
class ConstDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> const_init_val;
  void KoopaIR() const override;
};

// ConstInitVal ::= ConstExp;
class ConstInitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_exp;
  void KoopaIR() const override;
  int Calc() const;
};

// VarDecl ::= BType VarDefList ";";
// VarDefList ::= VarDef | VarDefList "," VarDef;
class VarDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> b_type;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST> > > var_def_list;
  void KoopaIR() const override;
};

// VarDef ::= IDENT | IDENT "=" InitVal;
class VarDefAST : public BaseAST {
 public:
  int type;
  std::string ident;
  std::unique_ptr<BaseAST> init_val;
  void KoopaIR() const override;
};

// InitVal ::= Exp;
class InitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  void KoopaIR() const override;
};

/**************************Func***************************/

// FuncDef ::= FuncType IDENT "(" ")" Block;
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  void KoopaIR() const override;
};

// FuncType ::= "int";
class FuncTypeAST : public BaseAST {
 public:
  void KoopaIR() const override;
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
//        | Exp ";"
//        | ";"
//        | Block
//        | "return" Exp ";";
//        | "return" ";";
class StmtAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> lval1_block4;
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

// LVal ::= IDENT;
class LValAST : public ExpBaseAST {
 public:
  std::string ident;
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

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp;
// UnaryOp ::= "+" | "-" | "!"
class UnaryExpAST : public ExpBaseAST {
 public:
  int type;
  char unaryop;
  std::unique_ptr<BaseAST> primaryexp1_unaryexp2;
  void KoopaIR() const override;
  int Calc() const override;
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