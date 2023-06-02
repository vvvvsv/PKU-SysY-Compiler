#include <iostream>
#include <cassert>
#include "ast.hpp"

// 计数 koopa 语句的返回值 %xxx
static int koopacnt = 0;
// 计数 if 语句，用于设置 entry
static int ifcnt = 0;
// 当前 entry 是否已经 ret, 若为 1 的话不应再生成任何语句
static int entry_returned = 0;

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
  std::cout << "  @" << current_code_block() << ident << " = alloc i32" << std::endl;
  insert_symbol(ident, SYM_TYPE_VAR, 0);

  if(type==2) {
    init_val->KoopaIR();
    // 存入 InitVal
    // store %1, @x
    std::cout << "  store %" << koopacnt-1 << ", @";
    std::cout << query_symbol(ident).first << ident << std::endl;
  }
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
  std::cout << "%entry:" << std::endl;
  entry_returned = 0;
  block->KoopaIR();
  // 若函数还未返回, 补一个ret
  // 无返回值补 ret
  if (!entry_returned) {
    const std::string& type = dynamic_cast<FuncTypeAST*>(func_type.get())->type;
    if (type == "i32")
      std::cout << "  ret 0" << std::endl;
    else if (type == "void")
      std::cout << "  ret" << std::endl;
    else
      assert(0);
  }
  std::cout << "}" << std::endl;
}

// FuncType ::= "int";
void FuncTypeAST::KoopaIR() const {
  std::cout << type;
}

/**************************Block***************************/

// Block ::= "{" BlockItemList "}";
// BlockItemList ::=  | BlockItemList BlockItem;
void BlockAST::KoopaIR() const {
  enter_code_block();
  for(auto& block_item: *block_item_list)
  {
    if(entry_returned) break;
    block_item->KoopaIR();
  }
  exit_code_block();
}

// BlockItem ::= Decl | Stmt;
void BlockItemAST::KoopaIR() const {
  decl1_stmt2->KoopaIR();
}

// Stmt ::= LVal "=" Exp ";"
void StmtAssignAST::KoopaIR() const {
  exp->KoopaIR();
  // 存入刚刚计算出的值
  // store %1, @x
  const std::string& ident = dynamic_cast<LValAST*>(lval.get())->ident;
  std::cout << "  store %" << koopacnt-1 << ", @";
  std::cout << query_symbol(ident).first << ident << std::endl;
}

//        | ";"
//        | Exp ";"
void StmtExpAST::KoopaIR() const {
  if(type==1) {
    // do nothing
  }
  else if(type==2) {
    exp->KoopaIR();
  }
}

//        | Block
void StmtBlockAST::KoopaIR() const {
  block->KoopaIR();
}

//        | "if" "(" Exp ")" Stmt
//        | "if" "(" Exp ")" Stmt "else" Stmt
void StmtIfAST::KoopaIR() const {
  if(entry_returned) return;
  int ifcur = ifcnt;
  ifcnt++;
  exp->KoopaIR();

  if(type==1) {
    // br %0, %then, %end
    std::cout << "  br %" << koopacnt-1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_END_" << ifcur << std::endl;

    // %STMTIF_THEN_233: 创建新的entry
    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    stmt_if->KoopaIR();
    if(!entry_returned) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_returned = 0;
  }
  else if(type==2) {
    // br %0, %then, %else
    std::cout << "  br %" << koopacnt-1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_ELSE_" << ifcur << std::endl;

    // %STMTIF_THEN_233: 创建新的entry
    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    stmt_if->KoopaIR();
    if(!entry_returned) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_ELSE_233: 创建新的entry
    std::cout << "%STMTIF_ELSE_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    stmt_else->KoopaIR();
    if(!entry_returned) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_returned = 0;
  }
}

//        | "return" ";";
//        | "return" Exp ";";
void StmtReturnAST::KoopaIR() const {
  if(type==1) {
    std::cout << "  ret" << std::endl;
    entry_returned = 1;
  }
  else if(type==2) {
    exp->KoopaIR();
    // ret %0
    std::cout << "  ret %" << koopacnt-1 << std::endl;
    entry_returned = 1;
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
// 只有 LVal 出现在表达式中时会调用该 KoopaIR
// 如果 LVal 作为左值出现，则在父节点读取其 ident
void LValAST::KoopaIR() const {
  auto val = query_symbol(ident);
  assert(val.second->type != SYM_TYPE_UND);

  if(val.second->type == SYM_TYPE_CONST)
  {
    // 此处有优化空间
    // %0 = add 0, 233
    std::cout << "  %" << koopacnt << " = add 0, ";
    std::cout<< val.second->value << std::endl;
    koopacnt++;
  }
  else if(val.second->type == SYM_TYPE_VAR)
  {
    // 从内存读取 LVal
    // %0 = load @x
    std::cout << "  %" << koopacnt << " = load @" << val.first << ident << std::endl;
    koopacnt++;
  }
  return;
}

int LValAST::Calc() const {
  auto val = query_symbol(ident);
  assert(val.second->type == SYM_TYPE_CONST);
  return val.second->value;
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
    // A&&B <==> (A!=0)&(B!=0)
    landexp->KoopaIR();
    // %2 = ne %0, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt-1 << ", 0" << std::endl;
    koopacnt++;

    // 短路求值, 相当于一个if
    int ifcur = ifcnt;
    ifcnt++;
    // @STMTIF_LAND_RESULT_233 = alloc i32
    std::cout << "  @" << "STMTIF_LAND_RESULT_" << ifcur << " = alloc i32" << std::endl;

    // br %0, %then, %else
    std::cout << "  br %" << koopacnt-1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_ELSE_" << ifcur << std::endl;

    // %STMTIF_THEN_233: 创建新的entry
    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    // && 左侧 LAndExp 为 1, 答案为 EqExp 的值
    eqexp->KoopaIR();
    // %2 = ne %0, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt-1 << ", 0" << std::endl;
    koopacnt++;
    std::cout << "  store %" << koopacnt-1 << ", @";
    std::cout << "STMTIF_LAND_RESULT_" << ifcur << std::endl;

    if(!entry_returned) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_ELSE_233: 创建新的entry
    std::cout << "%STMTIF_ELSE_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    // && 左侧 LAndExp 为 0, 答案为 0
    std::cout << "  store 0, @";
    std::cout << "STMTIF_LAND_RESULT_" << ifcur << std::endl;

    if(!entry_returned) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    std::cout << "  %" << koopacnt << " = load @";
    std::cout << "STMTIF_LAND_RESULT_" << ifcur << std::endl;
    koopacnt++;
  }
}

int LAndExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(eqexp.get())->Calc();
  }
  else if(type==2) {
    int left = dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();
    if(!left) return 0;
    int right = dynamic_cast<ExpBaseAST*>(eqexp.get())->Calc();
    return (right!=0);
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
    // A||B <==> (A!=0)|(B!=0)
    lorexp->KoopaIR();
    // %2 = ne %0, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt-1 << ", 0" << std::endl;
    koopacnt++;

    // 短路求值, 相当于一个if
    int ifcur = ifcnt;
    ifcnt++;
    // @STMTIF_LOR_RESULT_233 = alloc i32
    std::cout << "  @" << "STMTIF_LOR_RESULT_" << ifcur << " = alloc i32" << std::endl;

    // br %0, %then, %else
    std::cout << "  br %" << koopacnt-1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_ELSE_" << ifcur << std::endl;

    // %STMTIF_THEN_233: 创建新的entry
    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    // || 左侧 LOrExp 为 1, 答案为 1, 即左侧 LOrExp 的值
    std::cout << "  store 1, @";
    std::cout << "STMTIF_LOR_RESULT_" << ifcur << std::endl;

    if(!entry_returned) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_ELSE_233: 创建新的entry
    std::cout << "%STMTIF_ELSE_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    // || 左侧 LOrExp 为 0, 答案为 LAndExp 的值
    landexp->KoopaIR();
    // %2 = ne %0, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt-1 << ", 0" << std::endl;
    koopacnt++;
    std::cout << "  store %" << koopacnt-1 << ", @";
    std::cout << "STMTIF_LOR_RESULT_" << ifcur << std::endl;

    if(!entry_returned) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_returned = 0;
    std::cout << "  %" << koopacnt << " = load @";
    std::cout << "STMTIF_LOR_RESULT_" << ifcur << std::endl;
    koopacnt++;
  }
}

int LOrExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();
  }
  else if(type==2) {
    int left = dynamic_cast<ExpBaseAST*>(lorexp.get())->Calc();
    if(left) return 1;
    int right = dynamic_cast<ExpBaseAST*>(landexp.get())->Calc();
    return (right!=0);
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