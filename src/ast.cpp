#include <iostream>
#include <cassert>
#include "ast.hpp"

static int koopacnt = 0;

/************************CompUnit*************************/

// CompUnit ::= FuncDef
void CompUnitAST::KoopaIR() const {
  func_def->KoopaIR();
}

/**************************Decl***************************/

// Decl ::= ConstDecl | VarDecl;
void DeclAST::KoopaIR() const {
  const_decl1_var_decl2->KoopaIR();
}

// ConstDecl ::= "const" BType ConstDefList ";";
// ConstDefList ::= ConstDef | ConstDefList "," ConstDef;
void ConstDeclAST::KoopaIR() const {
  for(auto& const_def: *const_def_list)
    const_def->KoopaIR();
}

// BType ::= "int";
void BTypeAST::KoopaIR() const {
  assert(0);
  return;
}

// ConstDef ::= IDENT "=" ConstInitVal;
void ConstDefAST::KoopaIR() const {
  insert_symbol(ident, SYM_TYPE_CONST,
    dynamic_cast<ConstInitValAST*>(const_init_val.get())->Calc());
}

// ConstInitVal ::= ConstExp;
void ConstInitValAST::KoopaIR() const {
  assert(0);
  return;
}

int ConstInitValAST::Calc() const {
  return dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();
}

// VarDecl ::= BType VarDefList ";";
// VarDefList ::= VarDef | VarDefList "," VarDef;
void VarDeclAST::KoopaIR() const {
  for(auto& var_def : *var_def_list)
    var_def->KoopaIR();
}

// VarDef ::= IDENT | IDENT "=" InitVal;
void VarDefAST::KoopaIR() const {
  // 先 alloc 一段内存
  // @x = alloc i32
  std::cout << "  @" << ident << " = alloc i32" << std::endl;
  if(type==2) {
    init_val->KoopaIR();
    // 存入 InitVal
    // store %1, @x
    std::cout << "  store %" << koopacnt-1 << ", @" << ident << std::endl;
  }
  insert_symbol(ident, SYM_TYPE_VAR, 0);
}

// InitVal ::= Exp;
void InitValAST::KoopaIR() const {
  exp->KoopaIR();
}

/**************************Func***************************/

// FuncDef ::= FuncType IDENT "(" ")" Block;
void FuncDefAST::KoopaIR() const {
  // fun @main(): i32 {}
  std::cout << "fun @" << ident << "(): ";
  func_type->KoopaIR();
  std::cout << " {" << std::endl;
  block->KoopaIR();
  std::cout << std::endl << "}" << std::endl;
}

// FuncType ::= "int";
void FuncTypeAST::KoopaIR() const {
  std::cout << "i32";
}

/**************************Block***************************/

// Block ::= "{" BlockItemList "}";
// BlockItemList ::=  | BlockItemList BlockItem;
void BlockAST::KoopaIR() const {
  std::cout << "%entry:" << std::endl;
  for(auto& block_item: *block_item_list)
  {
    block_item->KoopaIR();
  }
}

// BlockItem ::= Decl | Stmt;
void BlockItemAST::KoopaIR() const {
  decl1_stmt2->KoopaIR();
}

// Stmt ::= LVal "=" Exp ";"
//        | "return" Exp ";";
void StmtAST::KoopaIR() const {
  if(type==1) {
    exp->KoopaIR();
    // 存入刚刚计算出的值
    // store %1, @x
    std::cout << "  store %" << koopacnt-1 << ", @";
    std::cout << dynamic_cast<LValAST*>(lval.get())->ident << std::endl;
  }
  else if(type==2) {
    exp->KoopaIR();
    // ret %0
    std::cout << "  ret %" << koopacnt-1;
  }
}

/***************************Exp***************************/

// Exp ::= LOrExp;
void ExpAST::KoopaIR() const {
  lorexp->KoopaIR();
}

int ExpAST::Calc() const {
  return dynamic_cast<ExpBaseAST*>(lorexp.get())->Calc();
}

// LVal ::= IDENT;
// 只有 LVal 出现在表达式中时会调用该KoopaIR
// 如果 LVal 作为左值出现，则在父节点读取其ident
void LValAST::KoopaIR() const {
  auto val = query_symbol(ident);
  assert(val->type != SYM_TYPE_UND);

  if(val->type == SYM_TYPE_CONST)
  {
    // 此处有优化空间
    // %0 = add 0, 233
    std::cout << "  %" << koopacnt << " = add 0, ";
    std::cout<< val->value << std::endl;
    koopacnt++;
  }
  else if(val->type == SYM_TYPE_VAR)
  {
    // 从内存读取 LVal
    // %0 = load @x
    std::cout << "  %" << koopacnt << " = load @" << ident << std::endl;
    koopacnt++;
  }
  return;
}

int LValAST::Calc() const {
  auto val = query_symbol(ident);
  assert(val->type == SYM_TYPE_CONST);
  return val->value;
}

// PrimaryExp ::= "(" Exp ")" | LVal | Number;
void PrimaryExpAST::KoopaIR() const {
  if(type==1) {
    exp1_lval2->KoopaIR();
  }
  else if(type==2) {
    exp1_lval2->KoopaIR();
  }
  else if(type==3) {
    // %0 = add 0, 233
    std::cout << "  %" << koopacnt << " = add 0, ";
    std::cout<< number << std::endl;
    koopacnt++;
  }
}

int PrimaryExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(exp1_lval2.get())->Calc();
  }
  else if(type==2) {
    return dynamic_cast<ExpBaseAST*>(exp1_lval2.get())->Calc();
  }
  else if(type==3) {
    return number;
  }
  assert(0);
  return 0;
}

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp;
// UnaryOp ::= "+" | "-" | "!"
void UnaryExpAST::KoopaIR() const {
  if(type==1) {
    primaryexp1_unaryexp2->KoopaIR();
  }
  else if(type==2) {
    primaryexp1_unaryexp2->KoopaIR();
    if(unaryop=='-') {
      // %1 = sub 0, %0
      std::cout << "  %" << koopacnt << " = sub 0, %";
      std::cout << koopacnt-1 <<std::endl;
      koopacnt++;
    }
    else if(unaryop=='!') {
      // %1 = eq 0, %0
      std::cout << "  %" << koopacnt << " = eq 0, %";
      std::cout << koopacnt-1 <<std::endl;
      koopacnt++;
    }
  }
}

int UnaryExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(primaryexp1_unaryexp2.get())->Calc();
  }
  else if(type==2) {
    int tmp = dynamic_cast<ExpBaseAST*>(primaryexp1_unaryexp2.get())->Calc();
    if(unaryop=='+') {
      return tmp;
    }
    else if(unaryop=='-') {
      return -tmp;
    }
    else if(unaryop=='!') {
      return !tmp;
    }
  }
  assert(0);
  return 0;
}

// MulExp ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp ::= "*" | "/" | "%"
void MulExpAST::KoopaIR() const {
  if(type==1) {
    unaryexp->KoopaIR();
  }
  else if(type==2) {
    mulexp->KoopaIR();
    int left = koopacnt-1;
    unaryexp->KoopaIR();
    int right = koopacnt-1;
    if(mulop=='*') {
      // %2 = mul %0, %1
      std::cout << "  %" << koopacnt << " = mul %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(mulop=='/') {
      // %2 = div %0, %1
      std::cout << "  %" << koopacnt << " = div %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(mulop=='%') {
      // %2 = mod %0, %1
      std::cout << "  %" << koopacnt << " = mod %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
  }
}

int MulExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(unaryexp.get())->Calc();
  }
  else if(type==2) {
    int left = dynamic_cast<ExpBaseAST*>(mulexp.get())->Calc();
    int right = dynamic_cast<ExpBaseAST*>(unaryexp.get())->Calc();
    if(mulop=='*') {
      return left * right;
    }
    else if(mulop=='/') {
      return left / right;
    }
    else if(mulop=='%') {
      return left % right;
    }
  }
  assert(0);
  return 0;
}

// AddExp ::= MulExp | AddExp AddOp MulExp;
// AddOp ::= "+" | "-"
void AddExpAST::KoopaIR() const {
  if(type==1) {
    mulexp->KoopaIR();
  }
  else if(type==2) {
    addexp->KoopaIR();
    int left = koopacnt-1;
    mulexp->KoopaIR();
    int right = koopacnt-1;
    if(addop=='+') {
      // %2 = add %0, %1
      std::cout << "  %" << koopacnt << " = add %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(addop=='-') {
      // %2 = sub %0, %1
      std::cout << "  %" << koopacnt << " = sub %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
  }
}

int AddExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(mulexp.get())->Calc();
  }
  else if(type==2) {
    int left = dynamic_cast<ExpBaseAST*>(addexp.get())->Calc();
    int right = dynamic_cast<ExpBaseAST*>(mulexp.get())->Calc();
    if(addop=='+') {
      return left + right;
    }
    else if(addop=='-') {
      return left - right;
    }
  }
  assert(0);
  return 0;
}

// RelExp ::= AddExp | RelExp RelOp AddExp;
// RelOp ::= "<" | ">" | "<=" | ">="
void RelExpAST::KoopaIR() const {
  if(type==1) {
    addexp->KoopaIR();
  }
  else if(type==2) {
    relexp->KoopaIR();
    int left = koopacnt-1;
    addexp->KoopaIR();
    int right = koopacnt-1;
    if(relop=="<") {
      // %2 = lt %0, %1
      std::cout << "  %" << koopacnt << " = lt %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(relop==">") {
      // %2 = gt %0, %1
      std::cout << "  %" << koopacnt << " = gt %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(relop=="<=") {
      // %2 = le %0, %1
      std::cout << "  %" << koopacnt << " = le %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(relop==">=") {
      // %2 = ge %0, %1
      std::cout << "  %" << koopacnt << " = ge %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
  }
}

int RelExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(addexp.get())->Calc();
  }
  else if(type==2) {
    int left = dynamic_cast<ExpBaseAST*>(relexp.get())->Calc();
    int right = dynamic_cast<ExpBaseAST*>(addexp.get())->Calc();
    if(relop=="<") {
      return left < right;
    }
    else if(relop==">") {
      return left > right;
    }
    else if(relop=="<=") {
      return left <= right;
    }
    else if(relop==">=") {
      return left >= right;
    }
  }
  assert(0);
  return 0;
}

// EqExp ::= RelExp | EqExp EqOp RelExp;
// EqOp ::= "==" | "!="
void EqExpAST::KoopaIR() const {
  if(type==1) {
    relexp->KoopaIR();
  }
  else if(type==2) {
    eqexp->KoopaIR();
    int left = koopacnt-1;
    relexp->KoopaIR();
    int right = koopacnt-1;
    if(eqop=="==") {
      // %2 = eq %0, %1
      std::cout << "  %" << koopacnt << " = eq %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(eqop=="!=") {
      // %2 = ne %0, %1
      std::cout << "  %" << koopacnt << " = ne %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
  }
}

int EqExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(relexp.get())->Calc();
  }
  else if(type==2) {
    int left = dynamic_cast<ExpBaseAST*>(eqexp.get())->Calc();
    int right = dynamic_cast<ExpBaseAST*>(relexp.get())->Calc();
    if(eqop=="==") {
      return left == right;
    }
    else if(eqop=="!=") {
      return left != right;
    }
  }
  assert(0);
  return 0;
}

// LAndExp ::= EqExp | LAndExp "&&" EqExp;
void LAndExpAST::KoopaIR() const {
  if(type==1) {
    eqexp->KoopaIR();
  }
  else if(type==2) {
    landexp->KoopaIR();
    int left = koopacnt-1;
    eqexp->KoopaIR();
    int right = koopacnt-1;
    // A&&B <==> (A!=0)&(B!=0)
    // %2 = ne %0, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << left << ", 0" << std::endl;
    left = koopacnt;
    koopacnt++;
    // %3 = ne %1, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << right << ", 0" << std::endl;
    right = koopacnt;
    koopacnt++;
    // %4 = and %2, %3
    std::cout << "  %" << koopacnt << " = and %";
    std::cout << left << ", %" << right << std::endl;
    koopacnt++;
  }
}

int LAndExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(eqexp.get())->Calc();
  }
  else if(type==2) {
    int left = dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();
    int right = dynamic_cast<ExpBaseAST*>(eqexp.get())->Calc();
    return left && right;
  }
  assert(0);
  return 0;
}

// LOrExp  ::= LAndExp | LOrExp "||" LAndExp;
void LOrExpAST::KoopaIR() const {
  if(type==1) {
    landexp->KoopaIR();
  }
  else if(type==2) {
    lorexp->KoopaIR();
    int left = koopacnt-1;
    landexp->KoopaIR();
    int right = koopacnt-1;
    // A||B <==> (A!=0)|(B!=0)
    // %2 = ne %0, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << left << ", 0" << std::endl;
    left = koopacnt;
    koopacnt++;
    // %3 = ne %1, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << right << ", 0" << std::endl;
    right = koopacnt;
    koopacnt++;
    // %4 = or %2, %3
    std::cout << "  %" << koopacnt << " = or %";
    std::cout << left << ", %" << right << std::endl;
    koopacnt++;
  }
}

int LOrExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();
  }
  else if(type==2) {
    int left = dynamic_cast<ExpBaseAST*>(lorexp.get())->Calc();
    int right = dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();
    return left || right;
  }
  assert(0);
  return 0;
}

// ConstExp ::= Exp;
void ConstExpAST::KoopaIR() const {
  assert(0);
  return;
}

int ConstExpAST::Calc() const {
  return dynamic_cast<ExpBaseAST*>(exp.get())->Calc();
}