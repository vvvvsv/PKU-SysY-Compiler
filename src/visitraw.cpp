#include <cassert>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <algorithm>
#include "visitraw.hpp"
#include "koopa.h"

// 类型为 koopa_raw_value 的有返回值的语句 -> 该语句相对于 sp 的存储位置
static std::unordered_map<koopa_raw_value_t, int> loc;
// 栈帧长度
static int stack_frame_length = 0;
// 已经使用的栈帧长度
static int stack_frame_used = 0;
// 当前正在访问的函数有没有保存ra
static int saved_ra = 0;
// 生成数组/指针的 (global) alloc 语句 -> 该数组/指针的维数.
// [[i32, 2], 3] -> 3, 2; **[[i32, 2], 3] ->  1, 1, 3, 2
static std::unordered_map<koopa_raw_value_t, std::vector<int> > dimvec;
// getelemptr 和 getptr 语句 -> 生成的指针的维数,
// 表示为 dimvec 中的 vector 的某段的 begin 和 end
typedef std::pair<std::vector<int>::iterator, std::vector<int>::iterator> pvitvit;
static std::unordered_map<koopa_raw_value_t, pvitvit> dimlr;

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
      else if(inst->kind.tag == KOOPA_RVT_ALLOC &&
        inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
      {
        local_var--;
        int arrmem = 1;
        auto base = inst->ty->data.pointer.base;
        while(base->tag == KOOPA_RTT_ARRAY)
        {
          dimvec[inst].push_back(base->data.array.len);
          arrmem *= base->data.array.len;
          base = base->data.array.base;
        }
        dimlr[inst] = std::make_pair(dimvec[inst].begin(), dimvec[inst].end());
        local_var += arrmem;
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
  std::cout<<std::endl;
  switch (kind.tag) {
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_ALLOC:
      // 访问 alloc 指令
      Visit("alloc", value);
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
    case KOOPA_RVT_GET_PTR:
      // 访问 getptr 指令
      Visit(kind.data.get_ptr, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      // 访问 getelemptr 指令
      Visit(kind.data.get_elem_ptr, value);
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

// value->kind.tag 为 KOOPA_RVT_GET_PTR 或 KOOPA_RVT_GET_ELEM_PTR,
// 或者是 KOOPA_RVT_GET_LOAD 且 load 的是函数开始保存数组参数的位置
static int value_is_ptr(const koopa_raw_value_t &value) {
  if (value->kind.tag == KOOPA_RVT_GET_PTR)
    return 1;
  if (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR)
    return 1;
  if (value->kind.tag == KOOPA_RVT_LOAD) {
    const auto& load = value->kind.data.load;
    if (load.src->kind.tag == KOOPA_RVT_ALLOC)
      if(load.src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
        return 1;
  }
  return 0;
}

// 将 value 的值放置在标号为 reg 的寄存器中
static void load2reg(int value, const std::string &reg) {
  std::cout << "  li " << reg << ", " << value << std::endl;
}

// 将 value 的地址放置在标号为 reg 的寄存器中
static void loadaddr2reg(const koopa_raw_value_t &value, const std::string &reg) {
  if(value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    const auto& index = value->kind.data.func_arg_ref.index;
    assert(index >= 8);
    load2reg(stack_frame_length + (index - 8) * 4, reg);
    std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;
  }
  else if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::cout << "  la " << reg << ", " << value->name+1 << std::endl;
  }
  // else if(value->kind.tag == KOOPA_RVT_GET_PTR ||
  //   value->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
  //   load2reg(loc[value], reg);
  //   std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;
  //   std::cout << "  lw " << reg << ", 0(" << reg << ")" << std::endl;
  // }
  else {
    load2reg(loc[value], reg);
    std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;
  }
}

// 将 value 的值放置在标号为 reg 的寄存器中
static void load2reg(const koopa_raw_value_t &value, const std::string &reg) {
  if (value->kind.tag == KOOPA_RVT_INTEGER) {
    load2reg(value->kind.data.integer.value, reg);
  }
  else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    // 函数参数
    const auto& index = value->kind.data.func_arg_ref.index;
    if (index < 8) {
      std::cout << "  mv " << reg << ", a" << index << std::endl;
    }
    else {
      // 一定保存在栈里
      loadaddr2reg(value, "t6");
      std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
    }
  }
  else {
    // 一定保存在栈里, 或者是全局符号
    loadaddr2reg(value, "t6");
    std::cout << "  lw " << reg << ", 0(t6)" << std::endl;
  }
}

// 将标号为 reg 的寄存器中的value的值保存在内存中
static void save2mem(const koopa_raw_value_t &value, const std::string &reg) {
  assert(value->kind.tag != KOOPA_RVT_INTEGER);
  loadaddr2reg(value, "t6");
  std::cout << "  sw " << reg << ", 0(t6)" << std::endl;

  // if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
  //   // la t6, var
  //   std::cout << "  la t6, " << value->name+1 << std::endl;
  //   // sw t0, 0(t6)
  //   std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
  // }
  // else {
  //   std::cout << "  li t6, " << loc[value] << std::endl;
  //   std::cout << "  add t6, t6, sp" << std::endl;
  //   std::cout << "  sw " << reg << ", 0(t6)" << std::endl;
  // }
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer) {
  std::cout << "  li a0, " << integer.value << std::endl;
}

// 遍历 agg 并输出为一系列 .word 格式
static void dfs_aggregate(const koopa_raw_value_t& value) {
  if(value->kind.tag == KOOPA_RVT_INTEGER) {
    // 到叶子了
    std::cout << "  .word " << value->kind.data.integer.value << std::endl;
  }
  else if(value->kind.tag == KOOPA_RVT_AGGREGATE) {
    const auto& agg = value->kind.data.aggregate;
    for(int i = 0; i < agg.elems.len; i++) {
      dfs_aggregate(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]));
    }
  }
}

// 访问 global alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
  std::cout << "  .data" << std::endl;
  std::cout << "  .globl " << value->name+1 << std::endl;
  std::cout << value->name+1 << ":" << std::endl;
  if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
    // 初始化为 int
    std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
  }
  else if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
    // 初始化为 0
    auto base = value->ty->data.pointer.base;
    if (base->tag == KOOPA_RTT_INT32)
      std::cout << "  .zero 4" << std::endl;
    else if (base->tag == KOOPA_RTT_ARRAY) {
      int zeromem = 4;
      while(base->tag == KOOPA_RTT_ARRAY)
      {
        dimvec[value].push_back(base->data.array.len);
        zeromem *= base->data.array.len;
        base = base->data.array.base;
      }
      dimlr[value] = std::make_pair(dimvec[value].begin(), dimvec[value].end());
      std::cout << "  .zero " << zeromem << std::endl;
    }
  }
  else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE) {
    // 数组，初始化为 Aggregate
    auto base = value->ty->data.pointer.base;
    while(base->tag == KOOPA_RTT_ARRAY)
    {
      dimvec[value].push_back(base->data.array.len);
      base = base->data.array.base;
    }
    dimlr[value] = std::make_pair(dimvec[value].begin(), dimvec[value].end());
    dfs_aggregate(global_alloc.init);
  }
  std::cout << std::endl;
}

// 访问没有 data 的 mode 指令
void Visit(const std::string &mode, const koopa_raw_value_t &value) {
  if (mode == "alloc") {
    auto base = value->ty->data.pointer.base;
    if (base->tag == KOOPA_RTT_INT32) {
      loc[value] = stack_frame_used;
      stack_frame_used += 4;
    }
    else if (base->tag == KOOPA_RTT_ARRAY) {
      // dimvec 在计算栈帧长度时已经算过了
      int arrmem = 4;
      for (auto i: dimvec[value])
        arrmem *= i;
      loc[value] = stack_frame_used;
      stack_frame_used += arrmem;
    }
    else if (base->tag == KOOPA_RTT_POINTER) {
      // 计算 dimvec, 前面有几个 * 就补几个 1
      while(base->tag == KOOPA_RTT_POINTER) {
        dimvec[value].push_back(1);
        base = base->data.array.base;
      }
      while(base->tag == KOOPA_RTT_ARRAY)
      {
        dimvec[value].push_back(base->data.array.len);
        base = base->data.array.base;
      }
      dimlr[value] = std::make_pair(dimvec[value].begin(), dimvec[value].end());
      loc[value] = stack_frame_used;
      stack_frame_used += 4;
    }
  }
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {
  load2reg(load.src, "t0");
  if(value_is_ptr(load.src))
    std::cout << "  lw t0, 0(t0)" << std::endl;

  // 若有返回值则保存到栈里
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    // 保存 dimlr
    if(dimlr.find(load.src) != dimlr.end())
      dimlr[value] = dimlr[load.src];
    // 存入栈
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    save2mem(value, "t0");
  }
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store) {
  load2reg(store.value, "t0");
  if(value_is_ptr(store.value))
    std::cout << "  lw t0, 0(t0)" << std::endl;
  // 这里不能直接调用 save2mem
  loadaddr2reg(store.dest, "t6");
  if(value_is_ptr(store.dest))
    std::cout << "  lw t6, 0(t6)" << std::endl;
  std::cout << "  sw t0, 0(t6)" << std::endl;
}

// 计算 offset, 为 4 * Prod from begin+1 to end
int calc_offset(const pvitvit& begin_end) {
  int offset = 4;
  auto it = begin_end.first;
  ++it;
  for(; it != begin_end.second; ++it) {
    offset *= (*it);
  }
  return offset;
}

// 访问 getptr 指令
void Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value) {
  const auto& begin_end = dimlr[get_ptr.src];
  loadaddr2reg(get_ptr.src, "t0");
  if(value_is_ptr(get_ptr.src))
    std::cout << "  lw t0, 0(t0)" << std::endl;

  load2reg(get_ptr.index, "t1");
  load2reg(calc_offset(begin_end), "t2");
  std::cout << "  mul t1, t1, t2" << std::endl;
  std::cout << "  add t0, t0, t1" << std::endl;

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    // 保存 dimlr
    dimlr[value] = begin_end;
    // 存入栈
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    save2mem(value, "t0");
  }
}

// 访问 getelemptr 指令
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value) {
  const auto& begin_end = dimlr[get_elem_ptr.src];
  loadaddr2reg(get_elem_ptr.src, "t0");
  if(value_is_ptr(get_elem_ptr.src))
    std::cout << "  lw t0, 0(t0)" << std::endl;

  load2reg(get_elem_ptr.index, "t1");
  load2reg(calc_offset(begin_end), "t2");
  std::cout << "  mul t1, t1, t2" << std::endl;
  std::cout << "  add t0, t0, t1" << std::endl;

  // 若有返回值则将 t0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    // 保存 dimlr
    auto first = begin_end.first;
    ++first;
    dimlr[value] = std::make_pair(first, begin_end.second);
    // 存入栈
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    save2mem(value, "t0");
  }
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
