#pragma once
#include <string>
#include <memory>

// 符号表中符号的类型
enum symbol_type
{
  CONST, // 常量
  VAR,   // 变量
  UND    // 符号不存在
};

// 符号表中符号的值
struct symbol_value
{
  symbol_type type; // 符号的类型
  int value;        // 符号的值
};

// 插入符号定义
void insert_symbol(const std::string &symbol, symbol_type type, int val);

// 确认符号定义是否存在, 若存在返回1, 否则返回0
bool exist_symbol(const std::string &symbol);

// 查询符号定义, 返回这个符号的值
const std::unique_ptr<symbol_value> query_symbol(const std::string &symbol);
