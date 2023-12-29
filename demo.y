%{
#include "pl0.h"

extern char* yytext;
extern int yylex();
extern int yyerror(const char * format,...);

%}


%union{
	int num;

	/* 写string id编译不过
	   但是写string* id是可以的
	   string* id;
	*/
	char *id;
}

//关键字定义,由于BEGIN是bison的关键字,所以必须改个名字!!
%token BEGIN_BODY,END_BODY 
%token CONST,VAR,PROC 
%token CALL
%token ODD 
%token IF,THEN
%token WHILE,DO

//运算符和分隔符
//%token NUM 
//%token ID 

%token <num> NUM
%token<id> ID

%token PLUS,MINUS,MUL,DIV         //算术运算符
%token EQ,NE,LT,LE,GT,GE   //比较运算符
%token LPAREN,RPAREN   //左右括号
%token COMMA,SEMICOLON,DOT,ASSIGN

//%start program
//%start factor
//%start term
//%start expr
//%start cond
//%start statement
//%start subprog
%start program
//%start const_list

%%
program:
	{ create_program(); } subprog DOT { end_program();}
	;

subprog:
	const_decl_part 
	var_decl_part 
	proc_decl_part {begin_proc_code();} 
	statement {end_proc();}
	;

const_decl_part:
	const_decl
	|
	;

const_decl:
	CONST const_list SEMICOLON
	;

const_list:
	const_def
	|const_list COMMA const_def
	;

const_def:
	ID EQ NUM {
		add_const_item($1,$3);
	}
	;

var_decl_part:
	var_decl
	|
	;

var_decl:
	VAR id_list SEMICOLON
	;

id_list:
	ID {add_var_item($1); }
	|id_list COMMA ID {add_var_item($3); }
	;


proc_decl_part:
	proc_list
	|
	;

proc_list:
	proc_decl
	|proc_list proc_decl
	;

proc_decl:
	proc_head subprog SEMICOLON;
	;

proc_head:
	PROC ID SEMICOLON
	{
		create_proc($2);
	}
	;

statements:
	statement
	|statements SEMICOLON statement
	;

statement:
	 assign_stat
	|call_stat
	|compound_stat
	|if_stat
	|while_stat
	|
	;

assign_stat:
	ID	ASSIGN expr{
		write_var($1);
	}
	;

call_stat:
	CALL ID{
		call($2);
	}
	;

compound_stat:
	BEGIN_BODY statements END_BODY
	{}
	;

if_stat:
	IF cond{
		after_if_conditon();
	} THEN statement
	{
		after_if_body();
	}
	;

while_stat:
	WHILE {before_while_conditon();} 
	cond {after_while_conditon()} 
	DO statement
	{after_while_body();}
	;


cond:
	ODD expr{
		gen(OPR, 0, 6, "OPR ODD");
	}
	|expr EQ expr{
		gen(OPR, 0, 8, "OPR == ");
	}
	|expr NE expr
	{
		gen(OPR, 0, 9, "OPR !=");
	}
	|expr LT expr
	{
		gen(OPR, 0, 10,"OPR <");
	}
	|expr GE expr
	{
		gen(OPR, 0, 11,"OPR >=");
	}
	|expr GT expr{
		gen(OPR, 0, 12,"OPR >");
	}
	|expr LE expr
	{
		gen(OPR, 0, 13,"OPR <=");
	}
	;

expr:
	first_op
	|expr PLUS term {
		gen(OPR,0,2,"OPR PLUS");	
	}
	|expr MINUS term {
		gen(OPR,0,3,"OPR MINUS");	
	}
	;

first_op:
	term
	|PLUS term
	|MINUS term{
		gen(OPR,0,1,"OPR NEG");	
	}
	;

term: 
	factor
	|term MUL factor{
		gen(OPR,0,4,"OPR MULTI");	
	}
	|term DIV factor{
		gen(OPR,0,4,"OPR DIV");	
	}
	;

factor: 
	  ID {
			load($1);
	}
	| NUM {
		load($1);
	}
	|LPAREN expr RPAREN
	;


%%
