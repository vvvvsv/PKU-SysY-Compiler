#include <unordered_map>
#include "symtable.hpp"


static std::unordered_map<std::string, std::unique_ptr<symbol_value>> symbol_table;

void insert_symbol(const std::string &symbol, symbol_type type, int val)
{
  auto symval = new symbol_value();
}


bool exist_symbol(const std::string &symbol)
{
}

const std::unique_ptr<symbol_value> query_symbol(const std::string &symbol)
{
}