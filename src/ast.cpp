#include <iostream>
#include "ast.hpp"

void CompUnitAST::Dump() const {
  std::cout << "CompUnitAST { ";
  func_def->Dump();
  std::cout << " }";
}

void CompUnitAST::KoopaIR() const {
  func_def->KoopaIR();
}

void FuncDefAST::Dump() const {
  std::cout << "FuncDefAST { ";
  func_type->Dump();
  std::cout << ", " << ident << ", ";
  block->Dump();
  std::cout << " }";
}

void FuncDefAST::KoopaIR() const {
  std::cout << "fun @" << ident << "(): ";
  func_type->KoopaIR();
  std::cout << " {" << std::endl;
  block->KoopaIR();
  std::cout << std::endl << "}" << std::endl;
}

void FuncTypeAST::Dump() const {
  std::cout << "FuncTypeAST { int }";
}

void FuncTypeAST::KoopaIR() const {
  std::cout << "i32";
}

void BlockAST::Dump() const {
  std::cout << "BlockAST { ";
  stmt->Dump();
  std::cout << " }";
}

void BlockAST::KoopaIR() const {
  std::cout << "%entry:" << std::endl;
  std::cout << "  ";
  stmt->KoopaIR();
}

void StmtAST::Dump() const {
  std::cout << "StmtAST { "<<number<<" }";
}

void StmtAST::KoopaIR() const {
  std::cout << "ret " << number;
}