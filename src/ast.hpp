#pragma once
#include <memory>
#include <string>

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;
  // 输出 KoopaIR 到 stdout
  virtual void KoopaIR() const = 0;
};

// CompUnit ::= FuncDef;
class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;
  void KoopaIR() const override;
};

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

// Block ::= "{" Stmt "}";
class BlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;
  void KoopaIR() const override;
};

// Stmt ::= "return" Exp ";";
class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  void KoopaIR() const override;
};

// Exp ::= LOrExp;
class ExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lorexp;
  void KoopaIR() const override;
};

// PrimaryExp ::= "(" Exp ")" | Number;
class PrimaryExpAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> exp;
  std::int32_t number;
  void KoopaIR() const override;
};

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp;
// UnaryOp ::= "+" | "-" | "!"
class UnaryExpAST : public BaseAST {
 public:
  int type;
  char unaryop;
  // primaryexp1_unaryexp2 表示在 type 为 1 时为 PrimaryExp
  // 在 type 为 2 时 为 UnaryExp
  std::unique_ptr<BaseAST> primaryexp1_unaryexp2;
  void KoopaIR() const override;
};

// MulExp ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp ::= "*" | "/" | "%"
class MulExpAST : public BaseAST {
 public:
  int type;
  char mulop;
  std::unique_ptr<BaseAST> mulexp;
  std::unique_ptr<BaseAST> unaryexp;
  void KoopaIR() const override;
};

// AddExp ::= MulExp | AddExp AddOp MulExp;
// AddOp ::= "+" | "-"
class AddExpAST : public BaseAST {
 public:
  int type;
  char addop;
  std::unique_ptr<BaseAST> addexp;
  std::unique_ptr<BaseAST> mulexp;
  void KoopaIR() const override;
};

// RelExp ::= AddExp | RelExp RelOp AddExp;
// RelOp ::= "<" | ">" | "<=" | ">="
class RelExpAST : public BaseAST {
 public:
  int type;
  std::string relop;
  std::unique_ptr<BaseAST> relexp;
  std::unique_ptr<BaseAST> addexp;
  void KoopaIR() const override;
};

// EqExp ::= RelExp | EqExp EqOp RelExp;
// EqOp ::= "==" | "!="
class EqExpAST : public BaseAST {
 public:
  int type;
  std::string eqop;
  std::unique_ptr<BaseAST> eqexp;
  std::unique_ptr<BaseAST> relexp;
  void KoopaIR() const override;
};

// LAndExp ::= EqExp | LAndExp "&&" EqExp;
class LAndExpAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> landexp;
  std::unique_ptr<BaseAST> eqexp;
  void KoopaIR() const override;
};

// LOrExp  ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpAST : public BaseAST {
 public:
  int type;
  std::unique_ptr<BaseAST> lorexp;
  std::unique_ptr<BaseAST> landexp;
  void KoopaIR() const override;
};