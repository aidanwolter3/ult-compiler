#ifndef CODE_GENERATOR_X86
#define CODE_GENERATOR_X86

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "../symbol_table/SymbolTable.h"

#include "CompoundStatement.h"
#include "AssignStatement.h"
#include "PrintStatement.h"
#include "IdExpression.h"
#include "NumExpression.h"
#include "OperationExpression.h"
#include "PairExpressionList.h"
#include "LastExpressionList.h"
#include "Plus.h"
#include "Minus.h"
#include "Divide.h"
#include "Multiply.h"

class CodeGenerator_x86 : public Visitor {
  private:
    char *code;
    int len;
    
    struct {
      unsigned int eax : 1;
      unsigned int ebx : 1;
      unsigned int ecx : 1;
      unsigned int edx : 1;
    } regs;

    const char* next_reg();
    void release_reg(const char *reg);

    SymbolTable *symbolTable;

    class CodeReturn {
      public:
        int type;
        char *tmp;
        CodeReturn(int size, int t) {
          tmp = (char*)malloc(3*sizeof(char));
          type = t;
        };
    };
  public:
    CodeGenerator_x86(const char *target, SymbolTable *symbolTable);
    void write_exit();
    void print_code();
    void write_code();
    void* visit(CompoundStatement *s);
    void* visit(AssignStatement *stm);
    void* visit(PrintStatement *stm);
    void* visit(IdExpression *exp);
    void* visit(NumExpression *exp);
    void* visit(OperationExpression *exp);
    void* visit(PairExpressionList *exp);
    void* visit(LastExpressionList *exp);
    void* visit(Plus *op);
    void* visit(Minus *op);
    void* visit(Divide *op);
    void* visit(Multiply *op);
};

#endif
