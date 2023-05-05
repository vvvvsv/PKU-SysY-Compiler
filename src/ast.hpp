#pragma once
#include <memory>
#include <string>

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;
  // 输出 AST 结构 到 stdout
  virtual void Dump() const = 0;
  // 输出 KoopaIR 到 stdout
  virtual void KoopaIR() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;
  void Dump() const override;
  void KoopaIR() const override;
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  void Dump() const override;
  void KoopaIR() const override;
};

class FuncTypeAST : public BaseAST {
 public:
  void Dump() const override;
  void KoopaIR() const override;
};

class BlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;
  void Dump() const override;
  void KoopaIR() const override;
};

class StmtAST : public BaseAST {
 public:
  std::int32_t number;
  void Dump() const override;
  void KoopaIR() const override;
};