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
  std::cout << "StmtAST { ";
  exp->Dump();
  std::cout << " }";
}

void StmtAST::KoopaIR() const {
  std::cout << "ret ";
  exp->KoopaIR();
}

void ExpAST::Dump() const {
  std::cout << "ExpAST { ";
  unaryexp->Dump();
  std::cout << " }";
}

void ExpAST::KoopaIR() const {
}

void PrimaryExpAST::Dump() const {
  std::cout << "PrimaryExpAST { ";
  exp1_number2->Dump();
  std::cout << " }";
}

void PrimaryExpAST::KoopaIR() const {
}

void NumberAST::Dump() const {
  std::cout << "Number { " << int_const << " }";
}

void NumberAST::KoopaIR() const {
  std::cout << int_const;
}

void UnaryExpAST::Dump() const {
  std::cout << "UnaryExpAST { ";
  primaryexp1_unaryop2->Dump();
  if(type==2)
    null1_unaryexp2->Dump();
  std::cout << " }";
}

void UnaryExpAST::KoopaIR() const {
}

void UnaryOpAST::Dump() const {
  std::cout << "UnaryOpAST { "<< unaryop << " }";
}

void UnaryOpAST::KoopaIR() const {
}