#include <unordered_map>
#include "symtable.hpp"

typedef std::unordered_map<std::string, std::shared_ptr<symbol_value> > symbol_table_t;
static symbol_table_t symbol_table;

// 在符号表中寻找符号，返回其iterator
static symbol_table_t::iterator find_iter(const std::string &symbol)
{
  return symbol_table.find(symbol);
}

int insert_symbol(const std::string &symbol, symbol_type type, int value)
{
  auto it = find_iter(symbol);

  // 已经存在的符号
  if(it != symbol_table.end())
    return -1;

  // 插入该符号
  auto symval = new symbol_value();
  symval->type = type;
  symval->value = value;
  symbol_table[symbol] = std::shared_ptr<symbol_value>(symval);
  return 0;
}

int exist_symbol(const std::string &symbol)
{
  auto it = find_iter(symbol);
  return (it != symbol_table.end());
}

std::shared_ptr<const symbol_value> query_symbol(const std::string &symbol)
{
  auto it = find_iter(symbol);

  // 若符号不存在
  if(it == symbol_table.end())
  {
    auto symval = new symbol_value();
    symval->type = SYM_TYPE_UND;
    symval->value = -1;
    return std::shared_ptr<const symbol_value>(symval);
  }

  // 符号存在，返回其value
  return std::shared_ptr<const symbol_value>(it->second);
}