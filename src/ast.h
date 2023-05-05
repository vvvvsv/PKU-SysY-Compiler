#ifndef AST_H_
#define AST_H_

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;
};

class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  std::unique_ptr<BaseAST> func_def;
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
};


#endif  // AST_H_