#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.hpp"

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
  cout.rdbuf(outputfile.rdbuf());

  if(string(mode)=="-koopa")
  {
    cout << ss.str();
  }
  else if(string(mode)=="-riscv")
  {
  }

  cout.rdbuf(oldcoutbuf);
  outputfile.close();
  return 0;
}