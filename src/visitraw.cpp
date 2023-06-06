#include <cassert>
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <algorithm>
#include "visitraw.hpp"
#include "koopa.h"

// 类型为 koopa_raw_value 的有返回值的语句的相对于 sp 的存储位置
static std::unordered_map<koopa_raw_value_t, int> loc;
// 栈帧长度
static int stack_frame_length = 0;
// 已经使用的栈帧长度
static int stack_frame_used = 0;
// 当前正在访问的函数有没有保存ra
static int saved_ra = 0;

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 忽略函数声明
  if(func->bbs.len == 0)
    return;

  // 执行一些其他的必要操作
  std::cout << "  .text" << std::endl;
  std::cout << "  .globl " << func->name+1 << std::endl;
  std::cout << func->name+1 << ":" <<std::endl;

  // 清空
  stack_frame_length = 0;
  stack_frame_used = 0;

  // 计算栈帧长度需要的值
  // 局部变量个数
  int local_var = 0;
  // 是否需要为 ra 分配栈空间
  int return_addr = 0;
  // 需要为传参预留几个变量的栈空间
  int arg_var = 0;

  // 遍历基本块
  for (size_t i = 0; i < func->bbs.len; ++i)
  {
    const auto& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
    local_var += insts.len;
    for (size_t j = 0; j < insts.len; ++j)
    {
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
      if(inst->ty->tag == KOOPA_RTT_UNIT)
        local_var--;
      if(inst->kind.tag == KOOPA_RVT_CALL)
      {
        return_addr = 1;
        arg_var = std::max(arg_var, std::max(0, int(inst->kind.data.call.args.len) - 8));
      }
    }
  }
  stack_frame_length = (local_var + return_addr + arg_var) << 2;
  // 将栈帧长度对齐到 16
  stack_frame_length = (stack_frame_length + 16 - 1) & (~(16 - 1));
  stack_frame_used = arg_var<<2;

  if (stack_frame_length != 0) {
    std::cout << "  li t0, " << -stack_frame_length << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;
  }

  if(return_addr) {
    std::cout << "  li t0, " << stack_frame_length - 4 << std::endl;
    std::cout << "  add t0, t0, sp" << std::endl;
    std::cout << "  sw ra, 0(t0)" << std::endl;
    saved_ra = 1;
  }
  else {
    saved_ra = 0;
  }

  // 访问所有基本块
  Visit(func->bbs);
  std::cout << std::endl;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // 当前块的label, %entry开头的不打印
  if(strncmp(bb->name+1, "entry", 5))
    std::cout << bb->name+1 << ":" << std::endl;
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_ALLOC:
      // 访问 alloc 指令
      loc[value] = stack_frame_used;
      stack_frame_used += 4;
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      // 访问 global alloc 指令
      Visit(value->kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_BRANCH:
      // 访问 branch 指令
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      // 访问 jump 指令
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // 访问 call 指令
      Visit(kind.data.call, value);
      break;
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    default:
      // 其他类型暂时遇不到
      // assert(false);
      break;
  }
}

// 将 value 的值放置在标号为 reg 的寄存器中
static void load2reg(const koopa_raw_value_t &value, const std::string &reg) {
  // std::cout<<value->kind.tag<<reg<<std::endl;
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    std::cout << "  li " << reg << ", " << value->kind.data.integer.value << std::endl;
  }
  else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    // 函数参数
    const auto& index = value->kind.data.func_arg_ref.index;
    if (index < 8) {
      std::cout << "  mv " << reg << ", a" << index << std::endl;
    }
    else {
      std::cout << "  li t6, " << stack_frame_length + (index - 8) * 4 << std::endl;
      std::cout << "  add t6, t6, sp" << std::endl;
      std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
    }
  }
  else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
      // la t6, var
      std::cout << "  la t6, " << value->name+1 << std::endl;
      // lw t0, 0(t6)
      std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
  }
  else {
    // value->kind.tag = KOOPA_RVT_LOAD, KOOPA_RVT_BINARY 之类的有返回值的语句
    std::cout << "  li t6, " << loc[value] << std::endl;
    std::cout << "  add t6, t6, sp" << std::endl;
    std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
  }
}

// 将标号为 reg 的寄存器中的value的值保存在内存中
static void save2mem(const koopa_raw_value_t &value, const std::string &reg) {
  assert(value->kind.tag != KOOPA_RVT_INTEGER);
  if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    // la t6, var
    std::cout << "  la t6, " << value->name+1 << std::endl;
    // sw t0, 0(t6)
    std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
  }
  else {
    std::cout << "  li t6, " << loc[value] << std::endl;
    std::cout << "  add t6, t6, sp" << std::endl;
    std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
  }
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer) {
  std::cout << "  li a0, " << integer.value << std::endl;
}

// 访问 global alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
  std::cout << "  .data" << std::endl;
  std::cout << "  .globl " << value->name+1 << std::endl;
  std::cout << value->name+1 << ":" << std::endl;
  if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
    // 初始化为 0
    std::cout << "  .zero 4" << std::endl;
  }
  else if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
    // 初始化为 int
    std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
  }
  std::cout << std::endl;
}


// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {
  load2reg(load.src, "t0");
  // 若有返回值则保存到栈里
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    save2mem(value, "t0");
  }
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store) {
  load2reg(store.value, "t0");
  save2mem(store.dest, "t0");
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value) {
  // 将运算数存入 t0 和 t1
  load2reg(binary.lhs, "t0");
  load2reg(binary.rhs, "t1");

  // 进行运算，结果存入t0
  if(binary.op == KOOPA_RBO_NOT_EQ) {
    std::cout << "  xor t0, t0, t1" << std::endl;
    std::cout << "  snez t0, t0" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_EQ) {
    std::cout << "  xor t0, t0, t1" << std::endl;
    std::cout << "  seqz t0, t0" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_GT) {
    std::cout << "  sgt t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_LT) {
    std::cout << "  slt t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_GE) {
    std::cout << "  slt t0, t0, t1" << std::endl;
    std::cout << "  xori t0, t0, 1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_LE) {
    std::cout << "  sgt t0, t0, t1" << std::endl;
    std::cout << "  xori t0, t0, 1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_ADD) {
    std::cout << "  add t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_SUB) {
    std::cout << "  sub t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_MUL) {
    std::cout << "  mul t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_DIV) {
    std::cout << "  div t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_MOD) {
    std::cout << "  rem t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_AND) {
    std::cout << "  and t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_OR) {
    std::cout << "  or t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_XOR) {
    std::cout << "  xor t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_SHL) {
    std::cout << "  sll t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_SHR) {
    std::cout << "  srl t0, t0, t1" << std::endl;
  }
  else if(binary.op == KOOPA_RBO_SAR) {
    std::cout << "  sra t0, t0, t1" << std::endl;
  }

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    save2mem(value, "t0");
  }
}

// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch) {
  load2reg(branch.cond, "t0");
  std::cout << "  bnez t0, DOUBLE_JUMP_" << branch.true_bb->name+1 << std::endl;
  std::cout << "  j " << branch.false_bb->name+1 << std::endl;
  std::cout << "DOUBLE_JUMP_" << branch.true_bb->name+1 << ":" << std::endl;
  std::cout << "  j " << branch.true_bb->name+1 << std::endl;
}


// 访问 jump 指令
void Visit(const koopa_raw_jump_t &jump) {
  std::cout << "  j " << jump.target->name+1 << std::endl;
}

// 访问 call 指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value) {
  // 处理参数
  for (size_t i = 0; i < call.args.len; ++i) {
    auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    if (i < 8) {
      load2reg(arg, "a" + std::to_string(i));
    }
    else {
      load2reg(arg, "t0");
      std::cout << "  li t6, " << (i - 8) * 4 << std::endl;
      std::cout << "  add t6, t6, sp" << std::endl;
      std::cout << "  sw t0, 0(t6)" << std::endl;
    }
  }
  // call half
  std::cout << "  call " << call.callee->name+1 << std::endl;

  // 若有返回值则将 a0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    save2mem(value, "a0");
  }
}


// 访问 return 指令
void Visit(const koopa_raw_return_t &ret) {
  // 返回值存入 a0
  if(ret.value != nullptr)
    load2reg(ret.value, "a0");
  // 从栈帧中恢复 ra 寄存器
  if (saved_ra) {
    std::cout << "  li t0, " << stack_frame_length - 4 << std::endl;
    std::cout << "  add t0, t0, sp" << std::endl;
    std::cout << "  lw ra, 0(t0)" << std::endl;
  }
  // 恢复栈帧
  if (stack_frame_length != 0) {
    std::cout << "  li t0, " << stack_frame_length << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;
  }
  std::cout << "  ret" << std::endl;
}
