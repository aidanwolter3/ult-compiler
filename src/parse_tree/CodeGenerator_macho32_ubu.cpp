#include "../ult.h"
#include "CodeGenerator_macho32_ubu.h"

const char* CodeGenerator_macho32_ubu::head =
                           "global _start\n"
                           "section .text\n";
const char* CodeGenerator_macho32_ubu::putint =
                           "\nputint:\n"
                           "push eax\n"
                           "push ebx\n"
                           "push ecx\n"
                           "push edx\n"
                           "mov eax,dword[esp+20]\n"
                           "mov ecx,0\n"
                           "push 0\n"
                           "push 0x0A\n"
                           "mov ebx,10\n"
                           ".putint0:\n"
                           "add ecx,1\n"
                           "mov dx,0\n"
                           "div bx\n"
                           "add edx,0x30\n"
                           "push edx\n"
                           "cmp ax,0\n"
                           "jz .putint1\n"
                           "jmp .putint0\n"
                           ".putint1:\n"
                           "push dword esp\n"
                           "call putstr\n"
                           "add esp,4\n"
                           "lea ecx,[8+4*ecx]\n"
                           "add esp,ecx\n"
                           "pop edx\n"
                           "pop ecx\n"
                           "pop ebx\n"
                           "pop eax\n"
                           "ret\n";
const char* CodeGenerator_macho32_ubu::putstr =
                           "\nputstr:\n"
                           "push eax\n"
                           "mov esi,dword[esp+8]\n"
                           ".putstr0:\n"
                           "mov byte al,[esi]\n"
                           "cmp al,0\n"
                           "je .putstr1\n"
                           "push eax\n"
                           "call putchar\n"
                           "add esp,4\n"
                           "add esi,4\n"
                           "jmp .putstr0\n"
                           ".putstr1:\n"
                           "pop eax\n"
                           "ret\n";
const char* CodeGenerator_macho32_ubu::putchar =
                           "\nputchar:\n"
                           "push eax\n"
                           "push ebx\n"
                           "push ecx\n"
                           "push edx\n"
                           "push dword [esp+20]\n"
                           "mov ebx,1\n"
                           "mov ecx,esp\n"
                           "mov edx,1\n"
                           "mov eax,4\n"
                           "int 0x80\n"
                           "add esp,4\n"
                           "pop edx\n"
                           "pop ecx\n"
                           "pop ebx\n"
                           "pop eax\n"
                           "ret\n";

CodeGenerator_macho32_ubu::CodeGenerator_macho32_ubu(SymbolTable *symbolTable, char *output) {

  //grab the symbol table
  this->symbolTable = symbolTable;

  //initialize the code
  code = (char*)malloc(2000*sizeof(char));
  len = 0;

  //set that no registers are in use
  regs.eax = 0;
  regs.ebx = 0;
  regs.ecx = 0;
  regs.edx = 0;

  strcpy(this->output, output);

  len += sprintf(code+len, "%s", head);

  //add the necessary functions for printing an integer
  len += sprintf(code+len, "%s", putint);
  len += sprintf(code+len, "%s", putstr);
  len += sprintf(code+len, "%s", putchar);

  len += sprintf(code+len, "\n_start:\n"
                           "mov ebp,esp\n"
                           "sub esp,%d\n", symbolTable->size*4);
}
const char* CodeGenerator_macho32_ubu::next_reg() {
  if(regs.eax == 0) {
    const char *eax = "eax";
    regs.eax = 1;
    return eax;
  }
  if(regs.ebx == 0) {
    const char *ebx = "ebx";
    regs.ebx = 1;
    return ebx;
  }
  if(regs.ecx == 0) {
    const char *ecx = "ecx";
    regs.ecx = 1;
    return ecx;
  }
  if(regs.edx == 0) {
    const char *edx = "edx";
    regs.edx = 1;
    return edx;
  }
  const char *noreg = "";
  return noreg;
}
void CodeGenerator_macho32_ubu::release_reg(const char *reg) {
  const char *eax = "eax";
  const char *ebx = "ebx";
  const char *ecx = "ecx";
  const char *edx = "edx";
  if(strcmp(reg, eax) == 0) {
    regs.eax = 0;
  }
  else if(strcmp(reg, ebx) == 0) {
    regs.ebx = 0;
  }
  else if(strcmp(reg, ecx) == 0) {
    regs.ecx = 0;
  }
  else if(strcmp(reg, edx) == 0) {
    regs.edx = 0;
  }
}
void CodeGenerator_macho32_ubu::write_exit() {
  len += sprintf(code+len, "add esp,%d\n"
                           "\n;exit\n"
                           "push dword 0\n"
                           "mov eax,1\n"
                           "sub esp,12\n"
                           "int 0x80\n"
                           "add esp,4\n", symbolTable->size*4);
}
void CodeGenerator_macho32_ubu::print_code() {
  printf("%s", code);
}
void CodeGenerator_macho32_ubu::write_code() {
  FILE *f = fopen(output, "w");
  fprintf(f, "%s", code);
  fclose(f);
}
void* CodeGenerator_macho32_ubu::visit(CompoundStatement *s) {
  s->stm1->accept(this);
  s->stm2->accept(this);
  return new CodeReturn(0, STATEMENT_PAIR_PROD);
}
void* CodeGenerator_macho32_ubu::visit(AssignStatement *stm) {
  CodeReturn *c1 = (CodeReturn*)stm->id->accept(this);
  CodeReturn *c2 = (CodeReturn*)stm->exp->accept(this);

  len += sprintf(code+len, "mov %s,%s\n", c1->tmp, c2->tmp);

  release_reg(c2->tmp);

  return new CodeReturn(0, STATEMENT_ASGN_PROD);
}
void* CodeGenerator_macho32_ubu::visit(PrintStatement *stm) {
  CodeReturn *c = (CodeReturn*)stm->exp->accept(this);

  len += sprintf(code+len, "push dword %s\n", c->tmp);
  len += sprintf(code+len, "call putint\n");

  release_reg(c->tmp);

  return new CodeReturn(0, STATEMENT_HUCK_PROD);
}
void* CodeGenerator_macho32_ubu::visit(IdExpression *exp) {
  CodeReturn *c = new CodeReturn(0, EXPRESSION_ID_PROD);
  Symbol *sym = symbolTable->get(exp->lexem);
  sprintf(c->tmp, "[ebp-%d]", sym->loc);
  return c;
}
void* CodeGenerator_macho32_ubu::visit(NumExpression *exp) {
  const char *reg = next_reg();
  len += sprintf(code+len, "mov %s,%d\n", reg, exp->val);

  CodeReturn *c = new CodeReturn(4, EXPRESSION_NUM_PROD);
  strcpy(c->tmp, reg);
  return c;
}
void* CodeGenerator_macho32_ubu::visit(StrExpression *exp) {
  const char *reg = next_reg();
  len += sprintf(code+len, "mov %s,%d\n", reg, 911);

  CodeReturn *c = new CodeReturn(0, EXPRESSION_STR_PROD);
  strcpy(c->tmp, reg);
  return c;
}
void* CodeGenerator_macho32_ubu::visit(OperationExpression *exp) {
  CodeReturn *c1 = (CodeReturn*)exp->exp1->accept(this);
  CodeReturn *c2 = (CodeReturn*)exp->op->accept(this);
  CodeReturn *c3 = (CodeReturn*)exp->exp2->accept(this);

  const char *reg = next_reg();

  if(c2->type == OPERATION_PLUS_PROD) {
    len += sprintf(code+len, "mov %s,%s\n"
                             "add %s,%s\n", reg, c1->tmp, reg, c3->tmp);
  }
  else if(c2->type == OPERATION_MINUS_PROD) {
    len += sprintf(code+len, "mov %s,%s\n"
                             "sub %s,%s\n", reg, c1->tmp, reg, c3->tmp);
  }
  else if(c2->type == OPERATION_DIV_PROD) {
    len += sprintf(code+len, "push eax\n"
                             "push edx\n"
                             "mov eax,%s\n"
                             "div dword %s\n"
                             "push %s\n"
                             "add esp,4\n"
                             "pop edx\n"
                             "pop eax\n"
                             "mov %s,[esp-12]\n", c1->tmp, c3->tmp, c1->tmp, reg);
  }
  else if (c2->type == OPERATION_MULT_PROD) {
    len += sprintf(code+len, "push eax\n"
                             "push edx\n"
                             "mov eax,%s\n"
                             "mul dword %s\n"
                             "push %s\n"
                             "add esp,4\n"
                             "pop edx\n"
                             "pop eax\n"
                             "mov %s,[esp-12]\n", c1->tmp, c3->tmp, c1->tmp, reg);
  }

  release_reg(c1->tmp);
  release_reg(c3->tmp);

  CodeReturn *c = new CodeReturn(0, EXPRESSION_OPR_PROD);
  strcpy(c->tmp, reg);
  return c;
}
void* CodeGenerator_macho32_ubu::visit(PairExpressionList *exp) {
  exp->exp->accept(this);
  CodeReturn *c2 = (CodeReturn*)exp->list->accept(this);
  CodeReturn *c = new CodeReturn(0, LIST_PAIR_PROD);
  strcpy(c->tmp, c2->tmp);
  return c;
}
void* CodeGenerator_macho32_ubu::visit(LastExpressionList *exp) {
  CodeReturn *c1 = (CodeReturn*)exp->exp->accept(this);
  CodeReturn *c = new CodeReturn(0, LIST_END_PROD);
  strcpy(c->tmp, c1->tmp);
  return c;
}
void* CodeGenerator_macho32_ubu::visit(Plus *op) {
  return new CodeReturn(0, OPERATION_PLUS_PROD);
}
void* CodeGenerator_macho32_ubu::visit(Minus *op) {
  return new CodeReturn(0, OPERATION_MINUS_PROD);
}
void* CodeGenerator_macho32_ubu::visit(Divide *op) {
  return new CodeReturn(0, OPERATION_DIV_PROD);
}
void* CodeGenerator_macho32_ubu::visit(Multiply *op) {
  return new CodeReturn(0, OPERATION_DIV_PROD);
}
