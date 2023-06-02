%code requires {
  #include <memory>
  #include <string>
  #include "ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include "ast.hpp"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
%parse-param { std::unique_ptr<BaseAST> &ast }

// 定义 yylval
%union {
  std::string *str_val;
  int int_val;
  char char_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST> > *vec_val;
}

// lexer 返回的所有 token 种类的声明
%token INT RETURN CONST
%token LAND LOR
%token <str_val> IDENT RELOP EQOP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> Decl ConstDecl BType ConstDef ConstInitVal VarDecl VarDef InitVal
%type <ast_val> FuncDef FuncType
%type <ast_val> Block BlockItem Stmt
%type <ast_val> Exp LVal PrimaryExp UnaryExp MulExp AddExp
%type <ast_val> RelExp EqExp LAndExp LOrExp ConstExp
%type <vec_val> ConstDefList BlockItemList VarDefList
%type <int_val> Number
%type <char_val> UnaryOp MulOp AddOp

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->type = 1;
    ast->const_decl1_var_decl2 = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->type = 2;
    ast->const_decl1_var_decl2 = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDefList ';' {
    auto ast = new ConstDeclAST();
    ast->b_type = unique_ptr<BaseAST>($2);
    ast->const_def_list = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = ast;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST();
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefList ',' ConstDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->b_type = unique_ptr<BaseAST>($1);
    ast->var_def_list = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

VarDefList
  : VarDef {
    auto vec = new vector<unique_ptr<BaseAST> >();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefList ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->type = 1;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->type = 2;
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST();
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->block_item_list = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = ast;
  }
  ;

BlockItemList
  : {
    auto vec = new vector<unique_ptr<BaseAST> >();
    $$ = vec;
  }
  | BlockItemList BlockItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

BlockItem
  : Decl {
    auto ast=new BlockItemAST();
    ast->type = 1;
    ast->decl1_stmt2 = unique_ptr<BaseAST>($1);
    $$=ast;
  }
  | Stmt {
    auto ast=new BlockItemAST();
    ast->type = 2;
    ast->decl1_stmt2 = unique_ptr<BaseAST>($1);
    $$=ast;
  }
  ;

Stmt
  : LVal '=' Exp ';' {
    auto ast = new StmtAssignAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtExpAST();
    ast->type = 1;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtExpAST();
    ast->type = 2;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtBlockAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtReturnAST();
    ast->type = 1;
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtReturnAST();
    ast->type = 2;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lorexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast=new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$=ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->type = 1;
    ast->exp1_lval2 = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->type = 2;
    ast->exp1_lval2 = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->type = 3;
    ast->number = $1;
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->type = 1;
    ast->primaryexp1_unaryexp2 = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->type = 2;
    ast->unaryop = $1;
    ast->primaryexp1_unaryexp2 = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    $$ = '+';
  }
  | '-' {
    $$ = '-';
  }
  | '!' {
    $$ = '!';
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->type = 1;
    ast->unaryexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpAST();
    ast->type = 2;
    ast->mulexp = unique_ptr<BaseAST>($1);
    ast->mulop = $2;
    ast->unaryexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulOp
  : '*' {
    $$ = '*';
  }
  | '/' {
    $$ = '/';
  }
  | '%' {
    $$ = '%';
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->type = 1;
    ast->mulexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpAST();
    ast->type = 2;
    ast->addexp = unique_ptr<BaseAST>($1);
    ast->addop = $2;
    ast->mulexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddOp
  : '+' {
    $$ = '+';
  }
  | '-' {
    $$ = '-';
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->type = 1;
    ast->addexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    ast->type = 2;
    ast->relexp = unique_ptr<BaseAST>($1);
    ast->relop = *unique_ptr<string>($2);
    ast->addexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->type = 1;
    ast->relexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast->type = 2;
    ast->eqexp = unique_ptr<BaseAST>($1);
    ast->eqop = *unique_ptr<string>($2);
    ast->relexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->type = 1;
    ast->eqexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAndExpAST();
    ast->type = 2;
    ast->landexp = unique_ptr<BaseAST>($1);
    ast->eqexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->type = 1;
    ast->landexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOR LAndExp {
    auto ast = new LOrExpAST();
    ast->type = 2;
    ast->lorexp = unique_ptr<BaseAST>($1);
    ast->landexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast=new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$=ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  extern int yylineno;
  extern char *yytext;
  int len = strlen(yytext);
  int i;
  char buf[512] = {0};
  for (i=0; i<len; ++i)
    sprintf(buf, "%s%d ", buf, yytext[i]);
  fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);
}