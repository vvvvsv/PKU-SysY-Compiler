#pragma once
#include "koopa.h"

// 访问 raw program
void Visit(const koopa_raw_program_t &program);

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice);

// 访问函数
void Visit(const koopa_raw_function_t &func);

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb);

// 访问指令
void Visit(const koopa_raw_value_t &value);

// 访问 return 指令
void Visit(const koopa_raw_return_t &value);

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &value);