#include <iostream>
#include "ast.hpp"

static int koopacnt = 0;

void CompUnitAST::KoopaIR() const {
  func_def->KoopaIR();
}

void FuncDefAST::KoopaIR() const {
  std::cout << "fun @" << ident << "(): ";
  func_type->KoopaIR();
  std::cout << " {" << std::endl;
  block->KoopaIR();
  std::cout << std::endl << "}" << std::endl;
}

void FuncTypeAST::KoopaIR() const {
  std::cout << "i32";
}

void BlockAST::KoopaIR() const {
  std::cout << "%entry:" << std::endl;
  stmt->KoopaIR();
}

void StmtAST::KoopaIR() const {
  exp->KoopaIR();
  std::cout << "  ret %" << koopacnt-1;
}

void ExpAST::KoopaIR() const {
  unaryexp->KoopaIR();
}

void PrimaryExpAST::KoopaIR() const {
  if(type==1) {
    exp->KoopaIR();
  }
  else if(type==2) {
    std::cout << "  %" << koopacnt << " = add 0, ";
    std::cout<< number << std::endl;
    koopacnt++;
  }
}

void UnaryExpAST::KoopaIR() const {
  if(type==1) {
    primaryexp1_unaryexp2->KoopaIR();
  }
  else if(type==2) {
    primaryexp1_unaryexp2->KoopaIR();
    if(unaryop=='-') {
      std::cout << "  %" << koopacnt << " = sub 0, %";
      std::cout << koopacnt-1 <<std::endl;
      koopacnt++;
    }
    else if(unaryop=='!') {
      std::cout << "  %" << koopacnt << " = eq 0, %";
      std::cout << koopacnt-1 <<std::endl;
      koopacnt++;
    }
  }
}
