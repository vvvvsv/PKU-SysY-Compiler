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

// 对 ::= 右侧的每个规则都设计一种 AST, 在 parse 到对应规则时, 构造对应的 AST.
// 例如 UnaryExpAST 中的 primaryexp1_unaryexp2 表示在 type 为 1 时为 PrimaryExp
// 在 type 为 2 时 为 UnaryExp

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

// Exp ::= UnaryExp;
class ExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> unaryexp;
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
class UnaryExpAST : public BaseAST {
 public:
  int type;
  char unaryop;
  std::unique_ptr<BaseAST> primaryexp1_unaryexp2;
  void KoopaIR() const override;
};