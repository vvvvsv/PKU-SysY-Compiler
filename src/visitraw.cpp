#include <cassert>
#include <iostream>
#include <unordered_map>
#include "visitraw.hpp"
#include "koopa.h"

// 类型为 koopa_raw_value 的有返回值的语句的存储位置
static std::unordered_map<koopa_raw_value_t, std::string> loc;
// 栈帧长度
static int stack_frame_length = 0;
// 已经使用的栈帧长度
static int stack_frame_used = 0;

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
  // 执行一些其他的必要操作
  std::cout << "  .text" << std::endl;
  std::cout << "  .globl " << func->name+1 << std::endl;
  std::cout << func->name+1 << ":" <<std::endl;

  // 清空
  stack_frame_length = 0;
  stack_frame_used = 0;

  // 计算栈帧长度
  int var_cnt = 0;

  // 遍历基本块
  for (size_t i = 0; i < func->bbs.len; ++i)
  {
    const auto& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
    var_cnt += insts.len;
    for (size_t j = 0; j < insts.len; ++j)
    {
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
      if(inst->ty->tag == KOOPA_RTT_UNIT)
        var_cnt--;
    }
  }
  // std::cout<<"++++"<<var_cnt<<std::endl;
  stack_frame_length = var_cnt << 2;
  // std::cout<<"++++"<<stack_frame_length<<std::endl;
  // 将栈帧长度对齐到 16
  stack_frame_length = (stack_frame_length + 16 - 1) & (~(16 - 1));
  // std::cout<<"++++"<<stack_frame_length<<std::endl;

  if (stack_frame_length != 0)
    std::cout << "  addi sp, sp, -" << stack_frame_length << std::endl;

  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
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
      loc[value] = std::to_string(stack_frame_used) + "(sp)";
      stack_frame_used += 4;
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_BINARY:
      Visit(kind.data.binary, value);
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
  else {
    // value->kind.tag = KOOPA_RVT_LOAD, KOOPA_RVT_BINARY 之类的有返回值的语句
    std::cout << "  lw " << reg << ", " << loc[value] << std::endl;
  }
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer) {
  std::cout << "  li a0, " << integer.value << std::endl;
}

void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {
  load2reg(load.src, "t0");
  loc[value] = std::to_string(stack_frame_used) + "(sp)";
  stack_frame_used += 4;
  std::cout << "  sw t0, " << loc[value] << std::endl;
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store) {
  load2reg(store.value, "t0");
  std::cout << "  sw t0, " << loc[store.dest] << std::endl;
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

  // 将 t0 中的结果存入栈
  loc[value] = std::to_string(stack_frame_used) + "(sp)";
  stack_frame_used += 4;
  std::cout << "  sw t0, " << loc[value] << std::endl;
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret) {
  // 访问返回值
  load2reg(ret.value, "a0");
  // 恢复栈帧
  if (stack_frame_length != 0)
    std::cout << "  addi sp, sp, " << stack_frame_length << std::endl;
  std::cout << "  ret" << std::endl;
}
