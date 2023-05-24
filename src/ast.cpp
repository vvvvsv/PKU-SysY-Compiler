#include <iostream>
#include <cassert>
#include "ast.hpp"

static int koopacnt = 0;

/************************CompUnit*************************/

// CompUnit ::= FuncDef
void CompUnitAST::KoopaIR() const {
  func_def->KoopaIR();
}

int CompUnitAST::Calc() const {
  assert(0);
  return 0;
}

/**************************Decl***************************/

// Decl ::= ConstDecl | VarDecl;
void DeclAST::KoopaIR() const {
  const_decl->KoopaIR();
}

int DeclAST::Calc() const {
  assert(0);
  return 0;
}

// ConstDecl ::= "const" BType ConstDefList ";";
// ConstDefList ::= ConstDef | ConstDefList "," ConstDef;
void ConstDeclAST::KoopaIR() const {
  for(auto& const_def: *const_def_list)
    const_def->KoopaIR();
}

int ConstDeclAST::Calc() const {
  assert(0);
  return 0;
}

// BType ::= "int";
void BTypeAST::KoopaIR() const {
  assert(0);
  return;
}

int BTypeAST::Calc() const {
  assert(0);
  return 0;
}

// ConstDef ::= IDENT "=" ConstInitVal;
void ConstDefAST::KoopaIR() const {
  insert_symbol(ident, SYM_TYPE_CONST, const_init_val->Calc());
}

int ConstDefAST::Calc() const {
  assert(0);
  return 0;
}

// ConstInitVal ::= ConstExp;
void ConstInitValAST::KoopaIR() const {
  assert(0);
  return;
}

int ConstInitValAST::Calc() const {
  return const_exp->Calc();
}

// VarDecl ::= BType VarDefList ";";
// VarDefList ::= VarDef | VarDefList "," VarDef;
// VarDef ::= IDENT | IDENT "=" InitVal;
// InitVal ::= Exp;

/**************************Func***************************/

// FuncDef ::= FuncType IDENT "(" ")" Block;
void FuncDefAST::KoopaIR() const {
  std::cout << "fun @" << ident << "(): ";
  func_type->KoopaIR();
  std::cout << " {" << std::endl;
  block->KoopaIR();
  std::cout << std::endl << "}" << std::endl;
}

int FuncDefAST::Calc() const {
  assert(0);
  return 0;
}

// FuncType ::= "int";
void FuncTypeAST::KoopaIR() const {
  std::cout << "i32";
}

int FuncTypeAST::Calc() const {
  assert(0);
  return 0;
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

int BlockAST::Calc() const {
  assert(0);
  return 0;
}

// BlockItem ::= Decl | Stmt;
void BlockItemAST::KoopaIR() const {
  decl1_stmt2->KoopaIR();
}

int BlockItemAST::Calc() const {
  assert(0);
  return 0;
}

// Stmt ::= LVal "=" Exp ";"
//        | "return" Exp ";";

// Stmt ::= "return" Exp ";";
void StmtAST::KoopaIR() const {
  exp->KoopaIR();
  std::cout << "  ret %" << koopacnt-1;
}

int StmtAST::Calc() const {
  assert(0);
  return 0;
}

/***************************Exp***************************/

// Exp ::= LOrExp;
void ExpAST::KoopaIR() const {
  lorexp->KoopaIR();
}

int ExpAST::Calc() const {
  return lorexp->Calc();
}

// LVal ::= IDENT;
void LValAST::KoopaIR() const {
  auto val = query_symbol(ident);
  assert(val->type != SYM_TYPE_UND);

  if(val->type == SYM_TYPE_CONST)
  {
    // 此处有优化空间
    std::cout << "  %" << koopacnt << " = add 0, ";
    std::cout<< val->value << std::endl;
    koopacnt++;
  }
  else if(val->type == SYM_TYPE_VAR)
  {
    assert(0);
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
    std::cout << "  %" << koopacnt << " = add 0, ";
    std::cout<< number << std::endl;
    koopacnt++;
  }
}

int PrimaryExpAST::Calc() const {
  if(type==1) {
    return exp1_lval2->Calc();
  }
  else if(type==2) {
    return exp1_lval2->Calc();
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
      std::cout << "  %" << koopacnt << " = sub 0, %";
      std::cout << koopacnt-1 <<std::endl;
      koopacnt++;
    }
    else if(unaryop=='!') {
      std::cout << "  %" << koopacnt << " = eq 0, %";
      std::cout << koopacnt-1 <<std::endl;
      koopacnt++;
    }
  }
}

int UnaryExpAST::Calc() const {
  if(type==1) {
    return primaryexp1_unaryexp2->Calc();
  }
  else if(type==2) {
    int tmp = primaryexp1_unaryexp2->Calc();
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
      std::cout << "  %" << koopacnt << " = mul %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(mulop=='/') {
      std::cout << "  %" << koopacnt << " = div %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(mulop=='%') {
      std::cout << "  %" << koopacnt << " = mod %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
  }
}

int MulExpAST::Calc() const {
  if(type==1) {
    return unaryexp->Calc();
  }
  else if(type==2) {
    int left = mulexp->Calc();
    int right = unaryexp->Calc();
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
      std::cout << "  %" << koopacnt << " = add %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(addop=='-') {
      std::cout << "  %" << koopacnt << " = sub %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
  }
}

int AddExpAST::Calc() const {
  if(type==1) {
    return mulexp->Calc();
  }
  else if(type==2) {
    int left = addexp->Calc();
    int right = mulexp->Calc();
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
      std::cout << "  %" << koopacnt << " = lt %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(relop==">") {
      std::cout << "  %" << koopacnt << " = gt %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(relop=="<=") {
      std::cout << "  %" << koopacnt << " = le %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(relop==">=") {
      std::cout << "  %" << koopacnt << " = ge %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
  }
}

int RelExpAST::Calc() const {
  if(type==1) {
    return addexp->Calc();
  }
  else if(type==2) {
    int left = relexp->Calc();
    int right = addexp->Calc();
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
      std::cout << "  %" << koopacnt << " = eq %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
    else if(eqop=="!=") {
      std::cout << "  %" << koopacnt << " = ne %";
      std::cout << left << ", %" << right << std::endl;
      koopacnt++;
    }
  }
}

int EqExpAST::Calc() const {
  if(type==1) {
    return relexp->Calc();
  }
  else if(type==2) {
    int left = eqexp->Calc();
    int right = relexp->Calc();
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
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << left << ", 0" << std::endl;
    left = koopacnt;
    koopacnt++;
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << right << ", 0" << std::endl;
    right = koopacnt;
    koopacnt++;
    std::cout << "  %" << koopacnt << " = and %";
    std::cout << left << ", %" << right << std::endl;
    koopacnt++;
  }
}

int LAndExpAST::Calc() const {
  if(type==1) {
    return eqexp->Calc();
  }
  else if(type==2) {
    int left = landexp->Calc();
    int right = eqexp->Calc();
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
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << left << ", 0" << std::endl;
    left = koopacnt;
    koopacnt++;
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << right << ", 0" << std::endl;
    right = koopacnt;
    koopacnt++;
    std::cout << "  %" << koopacnt << " = or %";
    std::cout << left << ", %" << right << std::endl;
    koopacnt++;
  }
}

int LOrExpAST::Calc() const {
  if(type==1) {
    return landexp->Calc();
  }
  else if(type==2) {
    int left = lorexp->Calc();
    int right = landexp->Calc();
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
  return exp->Calc();
}