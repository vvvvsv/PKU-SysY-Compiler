#pragma once
#include <string>
#include <memory>

// 符号表中符号的类型
enum symbol_type
{
  SYM_TYPE_CONST, // 常量
  SYM_TYPE_VAR,   // 变量
  SYM_TYPE_UND    // 符号不存在
};

// 符号表中符号的值
struct symbol_value
{
  symbol_type type; // 符号的类型
  int value;        // 符号的值
};

// 插入符号定义, 若成功插入返回0, 否则返回-1
int insert_symbol(const std::string &symbol, symbol_type type, int value);

// 确认符号定义是否存在, 若存在返回1, 否则返回0
int exist_symbol(const std::string &symbol);

// 查询符号定义, 返回指向这个符号的值的指针. 若符号不存在，返回的symbol_type为UND
std::shared_ptr<const symbol_value> query_symbol(const std::string &symbol);
