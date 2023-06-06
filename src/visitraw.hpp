#pragma once
#include <string>
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

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer);

// 访问 global alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value);

// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value);

// 访问 store 指令
void Visit(const koopa_raw_store_t &store);

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value);

// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch);

// 访问 jump 指令
void Visit(const koopa_raw_jump_t &jump);

// 访问 call 指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value);

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret);