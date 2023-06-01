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

// 进入新的作用域
void enter_code_block();

// 离开当前作用域
void exit_code_block();

// 返回当前符号表的表号(作用域号), 格式形如 "SYM_TABLE_233_"
std::string current_code_block();

// 插入符号定义
void insert_symbol(const std::string &symbol, symbol_type type, int value);

// 确认符号定义是否存在, 若存在返回1, 否则返回0
int exist_symbol(const std::string &symbol);

// 查询符号定义, 返回该符号所在符号表表号和指向这个符号的值的指针.
// 符号表表号格式形如 "SYM_TABLE_233_"
// 若符号不存在, 返回的表号为-1, symbol_type为UND
std::pair<std::string, std::shared_ptr<const symbol_value>> query_symbol(const std::string &symbol);