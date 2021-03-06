// Aidan Wolter
// Program Translation - COSC 4503

#include "SyntaxAnalyzer.h"
#include <stdlib.h>
#include <string.h>

SyntaxAnalyzer::SyntaxAnalyzer(FILE *syn_file, LexicalAnalyzer *lexicalAnalyzer, SymbolTable *symbolTable, ParseTree *parseTree) {
  parseTable = new CSV(syn_file, &rows, &cols);
  this->lexicalAnalyzer = lexicalAnalyzer;
  this->symbolTable = symbolTable;
  this->parseTree = parseTree;

  //keep track of where the error is even when finishing off the line
  last_line_index = 0;
  last_line_number = 1;
}

//parse the input and check for syntax errors
int SyntaxAnalyzer::parse() {
  bool error_found = false;

  //continue parsing until error or accept
  Token *t_stack[256]; //token stack
  int s_stack[256]; //state stack
  s_stack[0] = 0;   //start in state 0
  int t_stack_cnt = 0;
  int s_stack_cnt = 1;
  Token *t = lexicalAnalyzer->nextToken();
  Token *last_t = new Token();
  char *stopstr;

  if(t->t == 2) {
    symbolTable->add(t->t, t->l, 0, 4); //type = 0 for now
  }

  while(true) {

    //error in lexer, so continue
    if(t->t == -1) {
      error_found = true;

      //save where the error was found
      last_line_index = lexicalAnalyzer->getLineIndex();

      //reset the stacks
      s_stack[0] = 0;   //start in state 0
      s_stack_cnt = 1;
      t_stack_cnt = 0;

      //get the next token
      memcpy(last_t, t, sizeof(&t));
      t = lexicalAnalyzer->nextToken();
      if(t->t == 2) {
        symbolTable->add(t->t, t->l, 0, 4); //type = 0 for now
      }
    }

    //newline
    else if(t->t == -2) {

      //get another token
      last_line_index = lexicalAnalyzer->getLineIndex();
      memcpy(last_t, t, sizeof(&t));
      t = lexicalAnalyzer->nextToken();
      if(t->t == 2) {
        symbolTable->add(t->t, t->l, 0, 4); //type = 0 for now
      }
    }

    else if(t->t == -3 && error_found) {
      break;
    }

    //follow the table
    else {

      //get the command from the table
      //state+2 because we do not use the headers of the table
      int state = s_stack[s_stack_cnt-1];
      int token_index = colFromToken(t);
      char *cmd = parseTable->get(state+2, token_index);

      //cell is empty, so report expecting something else
      if(cmd == NULL || strlen(cmd) == 0) {
        error_found = true;

        //jump to next line by copying the error condition
        Token *error_t = t->duplicate();
        int error_state = state;
        strcpy(error_t->l, t->l);
        last_line_number = lexicalAnalyzer->getLineNumber();

        //continue grabbing tokens until or \n or eof
        memcpy(last_t, t, sizeof(&t));
        t = lexicalAnalyzer->nextToken();
        while(t->t != -2 && t->t != -3) {
          memcpy(last_t, t, sizeof(&t));
          t = lexicalAnalyzer->nextToken();
        }

        //throw syntax error as long as no lex error
        if(t->t != -1) {
          throw_unexpected_token(error_t, error_state, lexicalAnalyzer->getCurLine());
        }

        //reset the stacks
        s_stack[0] = 0;   //start in state 0
        s_stack_cnt = 1;
        t_stack_cnt = 0;

        //get another token
        memcpy(last_t, t, sizeof(&t));
        t = lexicalAnalyzer->nextToken();
        if(t->t == 2) {
          symbolTable->add(t->t, t->l, 0, 4); //type = 0 for now
        }
        continue;
      }

      #ifdef DEBUG
      printf("cmd: %s\n", cmd);
      #endif

      //reduction
      if(cmd[0] == 'r') {

        char *stopstr;
        int prod = strtod(&cmd[1], &stopstr);
        int pop_size = 0; //how much to pop off the stacks
        Token *rep_token = new Token();//what to push onto the token stack

        //check each production
        switch(prod) {

          //S -> S S
          case STATEMENT_PAIR_PROD: {
            pop_size = 2;
            rep_token->t = S_TOKEN;
            rep_token->l[0] = 'S';

            Statement *stm2 = (Statement*)parseTree->pop();
            Statement *stm1 = (Statement*)parseTree->pop();
            CompoundStatement *stm = new CompoundStatement(stm1, stm2);
            parseTree->push(stm);
            break;
          }

          //S ->
          case STATEMENT_NULL_PROD: {
            pop_size = 0;
            rep_token->t = S_TOKEN;
            rep_token->l[0] = 'S';
            break;
          }

          //S -> id assign E
          case STATEMENT_ASGN_PROD: {
            pop_size = 3;
            rep_token->t = S_TOKEN;
            rep_token->l[0] = 'S';

            char *lexem = (char*)malloc(sizeof(t_stack[t_stack_cnt-3]));
            strcpy(lexem, t_stack[t_stack_cnt-3]->l);
            IdExpression *id = new IdExpression(lexem);
            Expression *exp = (Expression*)parseTree->pop();
            AssignStatement *stm = new AssignStatement(id, exp);
            parseTree->push(stm);

            //update the size of the id in the symbol table
            Symbol *sym = symbolTable->get(lexem);
            sym->size = exp->data_size;
            break;
          }

          //S -> huck id
          case STATEMENT_HUCK_PROD: {
            pop_size = 2;
            rep_token->t = S_TOKEN;
            rep_token->l[0] = 'S';

            char *lexem = (char*)malloc(sizeof(&t_stack[t_stack_cnt-1]));
            strcpy(lexem, t_stack[t_stack_cnt-1]->l);
            IdExpression *exp = new IdExpression(lexem);
            PrintStatement *print = new PrintStatement(exp);
            parseTree->push(print);
            break;
          }

          //E -> num
          case EXPRESSION_NUM_PROD: {
            pop_size = 1;
            rep_token->t = E_TOKEN;
            rep_token->l[0] = 'E';

            int val = strtod((const char*)&t_stack[t_stack_cnt-1]->l, &stopstr);
            NumExpression *num = new NumExpression(val);
            parseTree->push(num);
            break;
          }

          //E -> id
          case EXPRESSION_ID_PROD: {
            pop_size = 1;
            rep_token->t = E_TOKEN;
            rep_token->l[0] = 'E';

            char *lexem = (char*)malloc(sizeof(&t_stack[t_stack_cnt-1]));
            strcpy(lexem, t_stack[t_stack_cnt-1]->l);
            IdExpression *id = new IdExpression(lexem);
            parseTree->push(id);
            break;
          }

          //E -> str
          case EXPRESSION_STR_PROD: {
            pop_size = 1;
            rep_token->t = E_TOKEN;
            rep_token->l[0] = 'E';

            char *s = (char*)malloc(sizeof(&t_stack[t_stack_cnt-1]));
            strcpy(s, t_stack[t_stack_cnt-1]->l);

            //remove the quotes
            s++;
            s[strlen(s)-1] = 0; 

            StrExpression *str = new StrExpression(s);
            parseTree->push(str);
            break;
          }

          //E -> E B E
          case EXPRESSION_OPR_PROD: {
            pop_size = 3;
            rep_token->t = E_TOKEN;
            rep_token->l[0] = 'E';

            Expression *exp2 = (Expression*)parseTree->pop();
            BinaryOperation *op = (BinaryOperation*)parseTree->pop();
            Expression *exp1 = (Expression*)parseTree->pop();
            OperationExpression *exp = new OperationExpression(exp1, exp2, op);
            parseTree->push(exp);
            break;
          }

          //L -> E comma L
          case LIST_PAIR_PROD: {
            pop_size = 3;
            rep_token->t = L_TOKEN;
            rep_token->l[0] = 'L';

            ExpressionList *list = (ExpressionList*)parseTree->pop();
            Expression *exp = (Expression*)parseTree->pop();
            ExpressionList *newlist = new PairExpressionList(exp, list);
            parseTree->push(newlist);
            break;
          }

          //L -> E
          case LIST_END_PROD: {
            pop_size = 1;
            rep_token->t = L_TOKEN;
            rep_token->l[0] = 'L';
            break;
          }

          //B -> plus
          case OPERATION_PLUS_PROD: {
            pop_size = 1;
            rep_token->t = B_TOKEN;
            rep_token->l[0] = 'B';

            Plus *plus = new Plus();
            parseTree->push(plus);
            break;
          }

          //B -> minus
          case OPERATION_MINUS_PROD: {
            pop_size = 1;
            rep_token->t = B_TOKEN;
            rep_token->l[0] = 'B';

            Minus *minus = new Minus();
            parseTree->push(minus);
            break;
          }

          //B -> mult
          case OPERATION_MULT_PROD: {
            pop_size = 1;
            rep_token->t = B_TOKEN;
            rep_token->l[0] = 'B';

            Multiply *multiply = new Multiply();
            parseTree->push(multiply);
            break;
          }

          //B -> div
          case OPERATION_DIV_PROD: {
            pop_size = 1;
            rep_token->t = B_TOKEN;
            rep_token->l[0] = 'B';

            Divide *divide = new Divide();
            parseTree->push(divide);
            break;
          }
        }

        //pop off the size of the production
        s_stack_cnt -= pop_size;
        t_stack_cnt -= pop_size;

        //push the replacement token
        Token *tmp = rep_token->duplicate();
        t_stack[t_stack_cnt++] = tmp;

        //push the new state from the goto
        int cur_state = s_stack[s_stack_cnt-1];
        int new_token_index = colFromToken(rep_token);
        char *cmd_goto = parseTable->get(cur_state+2, new_token_index);
        s_stack[s_stack_cnt++] = strtod(cmd_goto, &stopstr);
      }

      //shift
      else if(cmd[0] == 's') {
        int newstate = strtod(&cmd[1], &stopstr);
        Token *tmp = new Token();
        tmp->t = t->t;
        strcpy(tmp->l, t->l);
        t_stack[t_stack_cnt++] = tmp;
        s_stack[s_stack_cnt++] = newstate;

        //get another token
        last_line_index = lexicalAnalyzer->getLineIndex();
        memcpy(last_t, t, sizeof(&t));
        t = lexicalAnalyzer->nextToken();

        //if an id
        if(t->t == 2) {
          symbolTable->add(t->t, t->l, 0, 4); //type = 0 for now
        }
      }

      //accept command
      else if(cmd[0] == 'a') {
        if(!error_found) {
          return 0;
        }
        return -1;
      }

      //invalid command
      else {
        printf("Error: Parse table contains unknown command (%s)\n", cmd);
        return 0;
      }


      #ifdef DEBUG
      printf("t_stack: ");
      for(int i = 0; i < t_stack_cnt; i++) {
        printf("%d, ", t_stack[i]->t);
      }
      printf("\ns_stack: ");
      for(int i = 0; i < s_stack_cnt; i++) {
        printf("%d, ", s_stack[i]);
      }
      printf("\n");
      #endif
    }
  }

  return -1;
}

//search for the token in the parse table and return the col index
int SyntaxAnalyzer::colFromToken(Token *t) {
  for(int i = 0; i < cols; i++) {
    int tmp_token = strtod(parseTable->get(1, i), NULL);
    if(tmp_token == t->t) {
      return i;
    }
  }
  return -1;
}

//print an error message indicated where the unrecognized token is
void SyntaxAnalyzer::throw_unexpected_token(Token *t, int state, char *cur_line) {

  //get valid options
  char *options[256];
  int options_len = 0;
  for(int i = 0; i < cols; i++) {
    if(strlen(parseTable->get(state+2, i)) > 0) {
      options[options_len++] = parseTable->get(0, i);
    }
  }

  //build the options string
  char options_str[256];
  options_str[0] = '{';
  int options_str_len = 1;
  for(int i = 0; i < options_len; i++) {

    //add the option
    strcpy(&options_str[options_str_len], options[i]);
    options_str_len += strlen(options[i]);

    //only add a comma if not the last option
    if(i < options_len-1) {
      options_str[options_str_len++] = ',';
    }
  }
  options_str[options_str_len++] = '}';
  options_str[options_str_len] = '\0';

  printf("Error: Syntax analyzer found '%s' on line %d while expecting one of the set %s\n", t->l, last_line_number, options_str);
  printf("\t%s\n", cur_line);
  printf("\t");
  for(int i = 0; i < last_line_index; i++) {
    printf(" ");
  }
  printf("^\n");
}
