#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <string>
#include <cstring>
#include "koopa.h"
#include "ast.hpp"
#include "visitraw.hpp"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  // 打开输出文件
  ofstream outputfile(output);
  assert(outputfile);

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  // 先把 KoopaIR 输出到 ss 里
  stringstream ss;
  streambuf *oldcoutbuf = cout.rdbuf(ss.rdbuf());
  ast->KoopaIR();

  if(string(mode)=="-koopa")
  {
    cout.rdbuf(outputfile.rdbuf());
    cout << ss.str();
  }
  else if(string(mode)=="-riscv")
  {
    char* buf = new char[ss.str().length()+1];
    strcpy(buf, ss.str().c_str());
    // 解析字符串 buf, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(buf, &program);
    delete []buf;
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    // ...
    cout.rdbuf(outputfile.rdbuf());
    Visit(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    koopa_delete_raw_program_builder(builder);

  }

  cout.rdbuf(oldcoutbuf);
  outputfile.close();
  return 0;
}