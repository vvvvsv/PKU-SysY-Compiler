#include <iostream>
#include <cassert>
#include <unordered_map>
#include <algorithm>
#include "ast.hpp"

// 计数 koopa 语句的返回值 %xxx
static int koopacnt = 0;
// 计数 if 语句, 用于设置 entry
static int ifcnt = 0;
// 计数 while 语句, 用于设置 entry
static int whilecnt = 0;
// 当前 while 语句的标号
static int whilecur = 0;
// 保存 while 树上的 parent 关系
static std::unordered_map<int, int> whilepar;
// 当前 entry 是否已经 ret/br/jump, 若为 1 的话不应再生成任何语句
static int entry_end = 0;
// 当前是否在声明全局变量
static int declaring_global_var = 0;

/************************CompUnit*************************/

// CompUnit ::= CompUnitItemList;
// CompUnitItemList ::= CompUnitItem | CompUnitItemList CompUnitItem;
void CompUnitAST::KoopaIR() const {
  enter_code_block();
  // 声明库函数
  std::cout << "decl @getint(): i32\n" \
               "decl @getch(): i32\n" \
               "decl @getarray(*i32): i32\n" \
               "decl @putint(i32)\n" \
               "decl @putch(i32)\n" \
               "decl @putarray(i32, *i32)\n" \
               "decl @starttime()\n" \
               "decl @stoptime()\n" << std::endl;
  insert_symbol("getint", SYM_TYPE_FUNCINT, 0);
  insert_symbol("getch", SYM_TYPE_FUNCINT, 0);
  insert_symbol("getarray", SYM_TYPE_FUNCINT, 0);
  insert_symbol("putint", SYM_TYPE_FUNCVOID, 0);
  insert_symbol("putch", SYM_TYPE_FUNCVOID, 0);
  insert_symbol("putarray", SYM_TYPE_FUNCVOID, 0);
  insert_symbol("starttime", SYM_TYPE_FUNCVOID, 0);
  insert_symbol("stoptime", SYM_TYPE_FUNCVOID, 0);

  // 访问所有 CompUnitItem
  for(auto& comp_unit_item: *comp_unit_item_list) {
    comp_unit_item->KoopaIR();
  }
  exit_code_block();
}

// CompUnitItem ::= Decl | FuncDef;
void CompUnitItemAST::KoopaIR() const {
  if(type==1) {
    declaring_global_var = 1;
    decl1_funcdef2->KoopaIR();
    declaring_global_var = 0;
  }
  else if(type==2) {
    decl1_funcdef2->KoopaIR();
  }
}

/**************************Decl***************************/

// Decl ::= ConstDecl | VarDecl;
void DeclAST::KoopaIR() const {
  const_decl1_var_decl2->KoopaIR();
}

// ConstDecl ::= "const" TYPE ConstDefList ";";
// ConstDefList ::= ConstDef | ConstDefList "," ConstDef;
void ConstDeclAST::KoopaIR() const {
  for(auto& const_def: *const_def_list)
    const_def->KoopaIR();
}

// 打印一个 Aggregate
// mode == 'A' 为 Aggregate 模式, 如 {{1, 2}, {3, 4}} 等
// mode == 'S' 为 Store 模式, 每个元素分别 store
// mode == 'K' 为 KoopaIR Symbol 模式, 与 Store 模式类似, 但 agg 中都为 KoopaIR Symbol 编号 (%233)
static void print_aggregate(const std::string& ident, std::vector<int>* agg, int pos,
  std::vector<int>* len, std::vector<int>* mul_len, int cur, char mode) {
  // std::cout <<"--" << pos << std::endl;
  if (mode == 'A') {
    if(cur == len->size())
      std::cout << (*agg)[pos];
    else {
      std::cout << "{";
      int size = (*mul_len)[cur];
      size /= (*len)[cur];
      for (int i=0; i < (*len)[cur] ;i++) {
        print_aggregate(ident, agg, pos + i*size, len, mul_len, cur+1, mode);
        if(i != (*len)[cur]-1)
          std::cout << ", ";
      }
      std::cout << "}";
    }
  }
  else if (mode == 'S' || mode == 'K') {
    if(cur == len->size()) {
      if (mode == 'S')
        std::cout << "  store " << (*agg)[pos] << ", %" << koopacnt-1 << std::endl;
      else if (mode == 'K') {
        int tmp = (*agg)[pos];
        if (tmp == -1) // KoopaIR Symbol 编号为 -1 表示是后补的 0, 这样能省几条指令
          std::cout << "  store " << 0 << ", %" << koopacnt-1 << std::endl;
        else
          std::cout << "  store %" << tmp << ", %" << koopacnt-1 << std::endl;
      }
    }
    else {
      int size = (*mul_len)[cur];
      size /= (*len)[cur];
      int parent_ptr = koopacnt-1;
      for (int i=0; i < (*len)[cur] ;i++) {
        std::cout << "  %" << koopacnt << " = getelemptr ";
        if (cur == 0)
          std::cout << "@" << query_symbol(ident).first << ident;
        else
          std::cout << "%" << parent_ptr;
        std::cout << ", " << i << std::endl;
        koopacnt++;
        print_aggregate(ident, agg, pos + i*size, len, mul_len, cur+1, mode);
      }
    }
  }
}

// ConstDef ::= IDENT ConstIndexList "=" ConstInitVal;
// ConstIndexList ::=  | ConstIndexList "[" ConstExp "]";
void ConstDefAST::KoopaIR() const {
  if (const_index_list->empty()) {
    // 单常量
    insert_symbol(ident, SYM_TYPE_CONST,
      dynamic_cast<ConstInitValAST*>(const_init_val.get())->Calc());
  }
  else {
    // 常量数组
    insert_symbol(ident, SYM_TYPE_CONSTARRAY, const_index_list->size());

    // int arr[2][3] -> global @arr = alloc [[i32, 3], 2], {{1, 1, 4}, {5, 1, 4}}
    //                | @arr = alloc [[i32, 3], 2] \n 之后是store初始化
    if (declaring_global_var)
      std::cout << "global ";
    else
      std::cout << "  ";

    // @arr = alloc [[i32, 3], 2]
    std::cout << "@" << query_symbol(ident).first << ident << " = alloc";
    for (int i = 0; i < const_index_list->size(); i++) {
      std::cout << "[";
    }
    std::cout << "i32, ";

    // arr[2][3][4] -> len = {2, 3, 4}, mul_len = {4*3*2, 4*3, 4}
    auto mul_len = new std::vector<int>();
    auto len = new std::vector<int>();
    for (int i = const_index_list->size() - 1; i >= 0; i--) {
      const auto& const_exp = (*const_index_list)[i];
      int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();
      len->push_back(tmp);
      if(mul_len->empty())
        mul_len->push_back(tmp);
      else
        mul_len->push_back(mul_len->back() * tmp);
      std::cout << tmp << "], ";
    }
    std::cout.seekp(-2, std::cout.end);
    // 现在 mul_len = {4, 4*3, 4*3*2}, 需要 reverse 一下
    std::reverse(mul_len->begin(), mul_len->end());
    // 现在 len = {4, 3, 2}, 需要 reverse 一下
    std::reverse(len->begin(), len->end());

    // 初始化部分
    std::vector<int> agg = dynamic_cast<ConstInitValAST*>
      (const_init_val.get())->Aggregate(mul_len->begin(), mul_len->end());

    if (declaring_global_var) {
      // 全局常量数组, 用 aggregate 初始化
      std::cout << ", ";
      print_aggregate(ident, &agg, 0, len, mul_len, 0, 'A');
      std::cout << std::endl;
    }
    else {
      // 局部常量数组, 用store指令初始化
      std::cout << std::endl;
      print_aggregate(ident, &agg, 0, len, mul_len, 0, 'S');
    }
    delete mul_len;
    delete len;
  }
}

// ConstInitVal ::= ConstExp | ConstArrayInitVal;
// ConstArrayInitVal ::= "{" "}" | "{" ConstInitValList "}";
// ConstInitValList ::= ConstInitVal | ConstInitValList "," ConstInitVal;
void ConstInitValAST::KoopaIR() const {
  assert(0);
  return;
}

int ConstInitValAST::Calc() const {
  assert(type == 1);
  return dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();
}

std::vector<int> ConstInitValAST::Aggregate(std::vector<int>::iterator mul_len_begin,
  std::vector<int>::iterator mul_len_end) const {
  std::vector<int> agg;
  for(auto& const_init_val : *const_init_val_list) {
    auto child = dynamic_cast<ConstInitValAST*>(const_init_val.get());
    if (child->type == 1) {
      agg.push_back(child->Calc());
    }
    else if (child->type == 2) {
      int flag = 0;
      auto it = mul_len_begin;
      ++it;
      for (; it !=  mul_len_end; ++it) {
        if (agg.size() % (*it) == 0) {
          auto child_agg = child->Aggregate(it, mul_len_end);
          agg.insert(agg.end(), child_agg.begin(), child_agg.end());
          flag = 1;
          break;
        }
      }
      if (!flag)
        assert(0);
    }
  }
  agg.insert(agg.end(), (*mul_len_begin) - agg.size(), 0);
  return agg;
}

// VarDecl ::= TYPE VarDefList ";";
// VarDefList ::= VarDef | VarDefList "," VarDef;
void VarDeclAST::KoopaIR() const {
  for(auto& var_def : *var_def_list)
    var_def->KoopaIR();
}

// VarDef ::= IDENT ConstIndexList | IDENT ConstIndexList "=" InitVal;
// ConstIndexList ::=  | ConstIndexList "[" ConstExp "]";
void VarDefAST::KoopaIR() const {
  if(const_index_list->empty()) {
    // 单变量
    insert_symbol(ident, SYM_TYPE_VAR, 0);
    if(declaring_global_var) { // 全局变量
      if(type==1) {
        // global @var = alloc i32, zeroinit
        std::cout << "global @" << current_code_block() << ident;
        std::cout << " = alloc i32, zeroinit" << std::endl;
      }
      else if(type==2) {
        // global @var = alloc i32, 233
        std::cout << "global @" << current_code_block() << ident;
        std::cout << " = alloc i32, ";
        std::cout << dynamic_cast<InitValAST*>(init_val.get())->Calc() << std::endl;
      }
      std::cout << std::endl;
    }
    else { // 局部变量
      // 先 alloc 一段内存
      // @x = alloc i32
      std::cout << "  @" << current_code_block() << ident << " = alloc i32" << std::endl;

      if(type==2) {
        init_val->KoopaIR();
        // 存入 InitVal
        // store %1, @x
        std::cout << "  store %" << koopacnt-1 << ", @";
        std::cout << query_symbol(ident).first << ident << std::endl;
      }
    }
  }
  else {
    // 变量数组
    insert_symbol(ident, SYM_TYPE_ARRAY, const_index_list->size());
    // int arr[2][3] -> global @arr = alloc [[i32, 3], 2]
    //                | @arr = alloc [[i32, 3], 2]
    if (declaring_global_var)
      std::cout << "global ";
    else
      std::cout << "  ";

    // @arr = alloc [[i32, 3], 2]
    std::cout << "@" << query_symbol(ident).first << ident << " = alloc";
    for (int i = 0; i < const_index_list->size(); i++) {
      std::cout << "[";
    }
    std::cout << "i32, ";

    // arr[2][3][4] -> len = {2, 3, 4}, mul_len = {4*3*2, 4*3, 4}
    auto mul_len = new std::vector<int>();
    auto len = new std::vector<int>();
    for (int i = const_index_list->size() - 1; i >= 0; i--) {
      const auto& const_exp = (*const_index_list)[i];
      int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();
      len->push_back(tmp);
      if(mul_len->empty())
        mul_len->push_back(tmp);
      else
        mul_len->push_back(mul_len->back() * tmp);
      std::cout << tmp << "], ";
    }
    std::cout.seekp(-2, std::cout.end);
    // 现在 mul_len = {4, 4*3, 4*3*2}, 需要 reverse 一下
    std::reverse(mul_len->begin(), mul_len->end());
    // 现在 len = {4, 3, 2}, 需要 reverse 一下
    std::reverse(len->begin(), len->end());

    // 初始化部分
    if (declaring_global_var) {
      if (type == 1)
        std::cout << ", zeroinit" << std::endl;
      else if (type==2) {
        std::vector<int> agg = dynamic_cast<InitValAST*>
          (init_val.get())->Aggregate(mul_len->begin(), mul_len->end());
        std::cout << ", ";
        print_aggregate(ident, &agg, 0, len, mul_len, 0, 'A');
        std::cout << std::endl;
      }
    }
    else {
      if (type==1) {
        std::cout << std::endl;
      }
      else if (type==2) {
        std::cout << std::endl;
        std::vector<int> agg = dynamic_cast<InitValAST*>
          (init_val.get())->Aggregate(mul_len->begin(), mul_len->end());
        print_aggregate(ident, &agg, 0, len, mul_len, 0, 'K');
      }
    }
    delete mul_len;
    delete len;
  }
}

// InitVal ::= Exp | ArrayInitVal;
// ArrayInitVal ::= "{" "}" | "{" InitValList "}";
// InitValList ::= InitVal | InitValList "," InitVal;
void InitValAST::KoopaIR() const {
  assert(type == 1);
  exp->KoopaIR();
}

int InitValAST::Calc() const {
  assert(type == 1);
  return dynamic_cast<ExpBaseAST*>(exp.get())->Calc();
}

std::vector<int> InitValAST::Aggregate(std::vector<int>::iterator mul_len_begin,
  std::vector<int>::iterator mul_len_end) const {
  if(declaring_global_var) {
    // 全局数组变量的初始化列表中只能出现常量表达式, 返回的 agg 即为各项的值
    std::vector<int> agg;
    for(auto& init_val : *init_val_list) {
      auto child = dynamic_cast<InitValAST*>(init_val.get());
      if (child->type == 1) {
        agg.push_back(child->Calc());
      }
      else if (child->type == 2) {
        int flag = 0;
        auto it = mul_len_begin;
        ++it;
        for (; it !=  mul_len_end; ++it) {
          if (agg.size() % (*it) == 0) {
            auto child_agg = child->Aggregate(it, mul_len_end);
            agg.insert(agg.end(), child_agg.begin(), child_agg.end());
            flag = 1;
            break;
          }
        }
        if (!flag)
          assert(0);
      }
    }
    agg.insert(agg.end(), (*mul_len_begin) - agg.size(), 0);
    return agg;
  }
  else {
    // 局部数组变量的初始化列表中可以出现任何表达式, 返回的 agg 为各项的 KoopaIR Symbol 编号
    // 若编号为 -1, 则对应的值为 0
    std::vector<int> agg;
    for(auto& init_val : *init_val_list) {
      auto child = dynamic_cast<InitValAST*>(init_val.get());
      if (child->type == 1) {
        child->KoopaIR();
        agg.push_back(koopacnt-1);
      }
      else if (child->type == 2) {
        int flag = 0;
        auto it = mul_len_begin;
        ++it;
        for (; it !=  mul_len_end; ++it) {
          if (agg.size() % (*it) == 0) {
            auto child_agg = child->Aggregate(it, mul_len_end);
            agg.insert(agg.end(), child_agg.begin(), child_agg.end());
            flag = 1;
            break;
          }
        }
        if (!flag)
          assert(0);
      }
    }
    agg.insert(agg.end(), (*mul_len_begin) - agg.size(), -1);
    return agg;
  }
}

/**************************Func***************************/

// FuncDef ::= TYPE IDENT "(" FuncFParams ")" Block;
// FuncFParams ::=  | FuncFParamList;
// FuncFParamList ::= FuncFParam | FuncFParamList "," FuncFParam;
void FuncDefAST::KoopaIR() const {
  // 插入符号
  if (func_type == "void") {
    insert_symbol(ident, SYM_TYPE_FUNCVOID, 0);
  }
  else if (func_type == "int") {
    insert_symbol(ident, SYM_TYPE_FUNCINT, 0);
  }
  enter_code_block();

  // fun @func(@x: i32): i32 {}
  std::cout << "fun @" << ident << "(";
  for(auto& func_f_param: *func_f_param_list) {
    func_f_param->KoopaIR();
    std::cout << ", ";
  }
  // 退格擦除最后一个逗号
  if(!func_f_param_list->empty())
    std::cout.seekp(-2, std::cout.end);
  std::cout << ")";
  if (func_type == "int") {
    std::cout << ": i32";
  }

  std::cout << " {" << std::endl;
  std::cout << "%entry:" << std::endl;
  entry_end = 0;

  for(auto& func_f_param: *func_f_param_list) {
    // 为参数再分配一份内存
    // @SYM_TABLE_233_x = alloc i32
    // store @x, @SYM_TABLE_233_x
    dynamic_cast<FuncFParamAST*>(func_f_param.get())->Alloc();
  }

  block->KoopaIR();
  // 若函数还未返回, 补一个ret
  // 无返回值补 ret
  if (!entry_end) {
    if (func_type == "int")
      std::cout << "  ret 0" << std::endl;
    else if (func_type == "void")
      std::cout << "  ret" << std::endl;
    else
      assert(0);
  }
  std::cout << "}" << std::endl << std::endl;
  exit_code_block();
}

std::string FuncFParamAST::ParamType() const {
  assert(type==2);
  std::string str = "*";
  for (int i = 0; i < const_index_list->size(); i++) {
    str += "[";
  }
  str += "i32, ";
  for (int i = const_index_list->size() - 1; i >= 0; i--) {
    const auto& const_exp = (*const_index_list)[i];
    int tmp = dynamic_cast<ExpBaseAST*>(const_exp.get())->Calc();
    str += std::to_string(tmp) + "], ";
  }
  str.pop_back();
  str.pop_back();
  return str;
}

// FuncFParam ::= TYPE IDENT | TYPE IDENT "[" "]" ConstIndexList;
void FuncFParamAST::KoopaIR() const {
  if(type==1) {
    std::cout << "@" << ident << ": i32";
  }
  else if(type==2) {
    // 数组参数
    std::cout << "@" << ident << ": " << ParamType();
  }
}

void FuncFParamAST::Alloc() const {
  if(type==1) {
    // @SYM_TABLE_233_x = alloc i32
    std::cout << "  @" << current_code_block() << ident << " = alloc i32" << std::endl;
    insert_symbol(ident, SYM_TYPE_VAR, 0);
    // store @x, @SYM_TABLE_233_x
    std::cout << "  store @" << ident << ", @";
    std::cout << query_symbol(ident).first << ident << std::endl;
  }
  else if(type==2) {
    // 数组参数
    insert_symbol(ident, SYM_TYPE_PTR, const_index_list->size()+1);
    std::cout << "  @" << current_code_block() << ident << " = alloc " << ParamType() << std::endl;
    // store @x, @SYM_TABLE_233_x
    std::cout << "  store @" << ident << ", @";
    std::cout << query_symbol(ident).first << ident << std::endl;
  }
}

/**************************Block***************************/

// Block ::= "{" BlockItemList "}";
// BlockItemList ::=  | BlockItemList BlockItem;
void BlockAST::KoopaIR() const {
  enter_code_block();
  for(auto& block_item: *block_item_list)
  {
    if(entry_end) break;
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
  int exp_koopacnt = koopacnt-1;
  auto lvalptr = dynamic_cast<LValAST*>(lval.get());
  auto symval = query_symbol(lvalptr->ident);

  // 存入刚刚计算出的值

  if (symval.second->type == SYM_TYPE_VAR) {
    // LVal 为一个变量
    // store %1, @x
    std::cout << "  store %" << exp_koopacnt << ", @";
    std::cout << symval.first << lvalptr->ident << std::endl;
  }
  else if (symval.second->type == SYM_TYPE_ARRAY) {
    // LVal 为一个数组项
    // 依次 getelemptr
    for (int i = 0; i < lvalptr->index_list->size(); i++) {
      int lastptr_koopacnt = koopacnt-1;
      const auto& exp_index = (*(lvalptr->index_list))[i];
      dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
      int exp_index_koopacnt = koopacnt-1;

      std::cout << "  %" << koopacnt << " = getelemptr ";
      if (i == 0)
        std::cout << "@" << symval.first << lvalptr->ident;
      else
        std::cout << "%" << lastptr_koopacnt;
      std::cout << ", %" << exp_index_koopacnt << std::endl;
      koopacnt++;
    }
    std::cout << "  store %" << exp_koopacnt << ", %" << koopacnt-1 << std::endl;
  }
  else if (symval.second->type == SYM_TYPE_PTR) {
    // ident 为一个指针
    std::cout << "  %" << koopacnt << " = load @" << symval.first << lvalptr->ident << std::endl;
    koopacnt++;
    for (int i = 0; i<lvalptr->index_list->size(); i++) {
      int lastptr_koopacnt = koopacnt-1;
      const auto& exp_index = (*(lvalptr->index_list))[i];
      dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
      int exp_index_koopacnt = koopacnt-1;
      if(i==0)
        std::cout << "  %" << koopacnt << " = getptr %";
      else
        std::cout << "  %" << koopacnt << " = getelemptr %";
      std::cout << lastptr_koopacnt << ", %" << exp_index_koopacnt << std::endl;
      koopacnt++;
    }
    std::cout << "  store %" << exp_koopacnt << ", %" << koopacnt-1 << std::endl;
  }
  else
    assert(0);
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
  if(entry_end) return;
  int ifcur = ifcnt;
  ifcnt++;
  exp->KoopaIR();

  if(type==1) {
    // br %0, %then, %end
    std::cout << "  br %" << koopacnt-1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_END_" << ifcur << std::endl;

    // %STMTIF_THEN_233: 创建新的entry
    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;
    entry_end = 0;
    stmt_if->KoopaIR();
    if(!entry_end) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_end = 0;
  }
  else if(type==2) {
    // br %0, %then, %else
    std::cout << "  br %" << koopacnt-1 << ", %STMTIF_THEN_" << ifcur;
    std::cout << ", %STMTIF_ELSE_" << ifcur << std::endl;

    // %STMTIF_THEN_233: 创建新的entry
    std::cout << "%STMTIF_THEN_" << ifcur << ":" << std::endl;
    entry_end = 0;
    stmt_if->KoopaIR();
    if(!entry_end) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_ELSE_233: 创建新的entry
    std::cout << "%STMTIF_ELSE_" << ifcur << ":" << std::endl;
    entry_end = 0;
    stmt_else->KoopaIR();
    if(!entry_end) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_end = 0;
  }
}

//        | "while" "(" Exp ")" Stmt
void StmtWhileAST::KoopaIR() const {
  if(entry_end) return;
  int whileold = whilecur;
  whilecur = whilecnt;
  whilecnt++;
  whilepar[whilecur] = whileold;
  //   jump %while_entry
  // %while_entry:
  //   %cond = Exp
  //   br %cond, %while_body, %while_end
  // %while_body:
  //   Stmt
  //   jump %while_entry
  // %while_end:

  // jump %while_entry
  std::cout << "  jump %STMTWHILE_ENTRY_" << whilecur << std::endl;
  // %while_entry:
  std::cout << "%STMTWHILE_ENTRY_" << whilecur << ":" << std::endl;
  entry_end = 0;
  exp->KoopaIR();
  // br %cond, %while_body, %while_end
  std::cout << "  br %" << koopacnt-1 << ", %STMTWHILE_BODY_" << whilecur;
  std::cout << ", %STMTWHILE_END_" << whilecur << std::endl;
  // %while_body:
  std::cout << "%STMTWHILE_BODY_" << whilecur << ":" << std::endl;
  entry_end = 0;
  stmt->KoopaIR();
  if(!entry_end){
    // jump %while_entry
    std::cout << "  jump %STMTWHILE_ENTRY_" << whilecur << std::endl;
  }
  // %while_end:
  std::cout << "%STMTWHILE_END_" << whilecur << ":" << std::endl;
  entry_end = 0;
  whilecur = whilepar[whilecur];
}

//        | "break" ";"
void StmtBreakAST::KoopaIR() const {
  // jump %while_end
  std::cout << "  jump %STMTWHILE_END_" << whilecur << std::endl;
  entry_end = 1;
}

//        | "continue" ";"
void StmtContinueAST::KoopaIR() const {
  // jump %while_entry
  std::cout << "  jump %STMTWHILE_ENTRY_" << whilecur << std::endl;
  entry_end = 1;
}

//        | "return" ";";
//        | "return" Exp ";";
void StmtReturnAST::KoopaIR() const {
  if(type==1) {
    std::cout << "  ret" << std::endl;
    entry_end = 1;
  }
  else if(type==2) {
    exp->KoopaIR();
    // ret %0
    std::cout << "  ret %" << koopacnt-1 << std::endl;
    entry_end = 1;
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

// LVal ::= IDENT IndexList;
// IndexList ::=  | IndexList "[" Exp "]";
// 只有 LVal 出现在表达式中时会调用该 KoopaIR
// 如果 LVal 作为左值出现, 则在父节点 StmtAssign 读取其信息
void LValAST::KoopaIR() const {
  auto val = query_symbol(ident);
  assert(val.second->type != SYM_TYPE_UND);

  if(val.second->type == SYM_TYPE_CONST) {
    assert(index_list->size() == 0);
    // 此处有优化空间
    // %0 = add 0, 233
    std::cout << "  %" << koopacnt << " = add 0, ";
    std::cout<< val.second->value << std::endl;
    koopacnt++;
  }
  else if(val.second->type == SYM_TYPE_VAR) {
    assert(index_list->size() == 0);
    // 从内存读取 LVal
    // %0 = load @x
    std::cout << "  %" << koopacnt << " = load @" << val.first << ident << std::endl;
    koopacnt++;
  }
  else if(val.second->type == SYM_TYPE_CONSTARRAY || val.second->type == SYM_TYPE_ARRAY) {
    // 数组
    if (val.second->value == index_list->size()) {
      // 数组项, 依次 getelemptr
      for (int i = 0; i < index_list->size(); i++) {
        int lastptr_koopacnt = koopacnt-1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int exp_index_koopacnt = koopacnt-1;

        std::cout << "  %" << koopacnt << " = getelemptr ";
        if (i == 0)
          std::cout << "@" << val.first << ident;
        else
          std::cout << "%" << lastptr_koopacnt;
        std::cout << ", %" << exp_index_koopacnt << std::endl;
        koopacnt++;
      }
      std::cout << "  %" << koopacnt << " = load %" << koopacnt-1 << std::endl;
      koopacnt++;
    }
    else {
      // 部分解引用，做一个指针丢给函数
      for (int i = 0; i < index_list->size(); i++) {
        int lastptr_koopacnt = koopacnt-1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int exp_index_koopacnt = koopacnt-1;

        std::cout << "  %" << koopacnt << " = getelemptr ";
        if (i == 0)
          std::cout << "@" << val.first << ident;
        else
          std::cout << "%" << lastptr_koopacnt;
        std::cout << ", %" << exp_index_koopacnt << std::endl;
        koopacnt++;
      }
      int lastptr_koopacnt = koopacnt-1;
      std::cout << "  %" << koopacnt << " = getelemptr ";
      if (index_list->size() == 0)
        std::cout << "@" << val.first << ident;
      else
        std::cout << "%" << lastptr_koopacnt;
      std::cout << ", 0" << std::endl;
      koopacnt++;
    }
  }
  else if(val.second->type == SYM_TYPE_PTR) {
    // 指针
    if(val.second->value == index_list->size()) {
      // 取出对应的项
      std::cout << "  %" << koopacnt << " = load @" << val.first << ident << std::endl;
      koopacnt++;
      for (int i = 0; i < index_list->size(); i++) {
        int lastptr_koopacnt = koopacnt-1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int exp_index_koopacnt = koopacnt-1;
        if(i==0)
          std::cout << "  %" << koopacnt << " = getptr %";
        else
          std::cout << "  %" << koopacnt << " = getelemptr %";
        std::cout << lastptr_koopacnt << ", %" << exp_index_koopacnt << std::endl;
        koopacnt++;
      }
      std::cout << "  %" << koopacnt << " = load %" << koopacnt-1 << std::endl;
      koopacnt++;
    }
    else {
      // 部分解引用，做一个指针丢给函数
      std::cout << "  %" << koopacnt << " = load @" << val.first << ident << std::endl;
      koopacnt++;
      for (int i = 0; i < index_list->size(); i++) {
        int lastptr_koopacnt = koopacnt-1;
        const auto& exp_index = (*index_list)[i];
        dynamic_cast<ExpBaseAST*>(exp_index.get())->KoopaIR();
        int exp_index_koopacnt = koopacnt-1;
        if(i==0)
          std::cout << "  %" << koopacnt << " = getptr %";
        else
          std::cout << "  %" << koopacnt << " = getelemptr %";
        std::cout << lastptr_koopacnt << ", %" << exp_index_koopacnt << std::endl;
        koopacnt++;
      }
      int lastptr_koopacnt = koopacnt-1;
      if(index_list->size() == 0)
        std::cout << "  %" << koopacnt << " = getptr %";
      else
        std::cout << "  %" << koopacnt << " = getelemptr %";
      std::cout << lastptr_koopacnt << ", 0" << std::endl;
      koopacnt++;
    }
  }
  return;
}

int LValAST::Calc() const {
  std::cerr<<ident<<std::endl;
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

// UnaryExp ::= PrimaryExp | FuncExp | UnaryOp UnaryExp;
// UnaryOp ::= "+" | "-" | "!"
void UnaryExpAST::KoopaIR() const {
  if(type==1) {
    primaryexp1_funcexp2_unaryexp3->KoopaIR();
  }
  else if(type==2) {
    primaryexp1_funcexp2_unaryexp3->KoopaIR();
  }
  else if(type==3) {
    primaryexp1_funcexp2_unaryexp3->KoopaIR();
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

// FuncExp ::= IDENT "(" FuncRParams ")";
// FuncRParams ::=  | FuncRParamList;
// FuncRParamList ::= Exp | FuncRParamList "," Exp;
void FuncExpAST::KoopaIR() const {
  auto func = query_symbol(ident);
  // 必须为全局符号
  assert(func.first == "SYM_TABLE_0_");
  // 必须是函数
  assert(func.second->type == SYM_TYPE_FUNCVOID || func.second->type == SYM_TYPE_FUNCINT);

  // 计算所有的参数
  auto vec = new std::vector<int>();
  for(auto& exp: *func_r_param_list) {
    exp->KoopaIR();
    vec->push_back(koopacnt-1);
  }

  // 如果是 int 函数, 把返回值保存下来
  if(func.second->type == SYM_TYPE_FUNCINT)
    std::cout << "  %" << koopacnt << " = ";
  else if(func.second->type == SYM_TYPE_FUNCVOID)
    std::cout << "  ";

  // call @half(%1, %2)
  std::cout << "call @" << ident << "(";
  for(int i: *vec) {
    std::cout << "%" << i << ", ";
  }

  // 退格擦除最后一个逗号
  if(!vec->empty())
    std::cout.seekp(-2, std::cout.end);
  std::cout << ")" << std::endl;
  delete vec;
  if(func.second->type == SYM_TYPE_FUNCINT)
    koopacnt++;
}

int UnaryExpAST::Calc() const {
  if(type==1) {
    return dynamic_cast<ExpBaseAST*>(primaryexp1_funcexp2_unaryexp3.get())->Calc();
  }
  else if(type==3) {
    int tmp = dynamic_cast<ExpBaseAST*>(primaryexp1_funcexp2_unaryexp3.get())->Calc();
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
  // 如果 type == 2 则不能计算
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
    entry_end = 0;
    // && 左侧 LAndExp 为 1, 答案为 EqExp 的值
    eqexp->KoopaIR();
    // %2 = ne %0, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt-1 << ", 0" << std::endl;
    koopacnt++;
    std::cout << "  store %" << koopacnt-1 << ", @";
    std::cout << "STMTIF_LAND_RESULT_" << ifcur << std::endl;

    if(!entry_end) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_ELSE_233: 创建新的entry
    std::cout << "%STMTIF_ELSE_" << ifcur << ":" << std::endl;
    entry_end = 0;
    // && 左侧 LAndExp 为 0, 答案为 0
    std::cout << "  store 0, @";
    std::cout << "STMTIF_LAND_RESULT_" << ifcur << std::endl;

    if(!entry_end) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_end = 0;
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
    entry_end = 0;
    // || 左侧 LOrExp 为 1, 答案为 1, 即左侧 LOrExp 的值
    std::cout << "  store 1, @";
    std::cout << "STMTIF_LOR_RESULT_" << ifcur << std::endl;

    if(!entry_end) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_ELSE_233: 创建新的entry
    std::cout << "%STMTIF_ELSE_" << ifcur << ":" << std::endl;
    entry_end = 0;
    // || 左侧 LOrExp 为 0, 答案为 LAndExp 的值
    landexp->KoopaIR();
    // %2 = ne %0, 0
    std::cout << "  %" << koopacnt << " = ne %";
    std::cout << koopacnt-1 << ", 0" << std::endl;
    koopacnt++;
    std::cout << "  store %" << koopacnt-1 << ", @";
    std::cout << "STMTIF_LOR_RESULT_" << ifcur << std::endl;

    if(!entry_end) {
      // jump %STMTIF_END_233
      std::cout << "  jump %STMTIF_END_" << ifcur << std::endl;
    }

    // %STMTIF_END_233: 创建新的entry
    std::cout << "%STMTIF_END_" << ifcur << ":" << std::endl;
    entry_end = 0;
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