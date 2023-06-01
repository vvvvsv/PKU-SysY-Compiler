#include <unordered_map>
#include <vector>
#include <cassert>
#include "symtable.hpp"

// 作用域形成了一个树形结构, 而每个时刻需要保存的数个作用域形成了树上从根到某个节点的
// 一条链. 使用一个 std::vector 来维护这个链, 形成一个栈

// 作用域计数, 已经使用了了多少个符号表
static int symbol_table_cnt = 0;

// 作用域栈, 内容物为 std::pair<符号表标号, 符号表>
typedef std::unordered_map<std::string, std::shared_ptr<symbol_value> > symbol_table_t;
typedef std::vector<std::pair<int, symbol_table_t*> > symbol_table_stack_t;
static symbol_table_stack_t symbol_table_stack;

// 进入新的作用域
void enter_code_block()
{
  symbol_table_t* ptr = new symbol_table_t();
  symbol_table_stack.push_back(std::make_pair(symbol_table_cnt, ptr));
  symbol_table_cnt++;
}

// 离开当前作用域
void exit_code_block()
{
  delete symbol_table_stack.back().second;
  symbol_table_stack.pop_back();
}

// 返回当前作用域的标号, 格式形如 "SYM_TABLE_233"
std::string current_code_block()
{
  return "SYM_TABLE_" + std::to_string(symbol_table_stack.back().first) + "_";
}

// 在符号表中寻找符号, 返回其所在符号表的 标号 和其本身的 iterator
static std::pair<int, symbol_table_t::iterator> find_iter(const std::string &symbol)
{
  for(auto rit = symbol_table_stack.rbegin(); rit != symbol_table_stack.rend(); ++rit)
  {
    auto it = rit->second->find(symbol);
    if(it == rit->second->end()) continue;
    return std::make_pair(rit->first, it);
  }
  // 没找到
  return std::make_pair(-1, symbol_table_stack.back().second->end());
}

// 插入符号定义
void insert_symbol(const std::string &symbol, symbol_type type, int value)
{
  int symtid;
  symbol_table_t::iterator it;
  std::tie(symtid, it) = find_iter(symbol);

  // 插入该符号
  auto symval = new symbol_value();
  symval->type = type;
  symval->value = value;
  (*(symbol_table_stack.back().second))[symbol] = std::shared_ptr<symbol_value>(symval);
  return;
}

// 确认符号定义是否存在, 若存在返回1, 否则返回0
int exist_symbol(const std::string &symbol)
{
  int symtid;
  symbol_table_t::iterator it;
  std::tie(symtid, it) = find_iter(symbol);
  return (symtid != -1);
}

// 查询符号定义, 返回该符号所在符号表表号和指向这个符号的值的指针.
// 若符号不存在, 返回的表号为-1, symbol_type为UND
std::pair<std::string, std::shared_ptr<const symbol_value>> query_symbol(const std::string &symbol)
{
  int symtid;
  symbol_table_t::iterator it;
  std::tie(symtid, it) = find_iter(symbol);

  std::string str = "SYM_TABLE_" + std::to_string(symtid) + "_";

  // 若符号不存在
  if(symtid == -1)
  {
    auto symval = new symbol_value();
    symval->type = SYM_TYPE_UND;
    symval->value = -1;
    return std::make_pair(str, std::shared_ptr<const symbol_value>(symval));
  }

  // 符号存在, 返回其value
  return std::make_pair(str, std::shared_ptr<const symbol_value>(it->second));
}