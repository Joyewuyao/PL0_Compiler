%{
#include "pl0.h"
#include "demo.tab.h"

const bool PRINT_TOKEN_FLAG = true;
void print_token(){
	if (PRINT_TOKEN_FLAG)
		printf("### read a token: %s\n",yytext);
}

%}


%%

"//".*\n {};

"begin" { print_token(); return BEGIN_BODY;}
"end" {print_token(); return END_BODY;}
"const" {print_token(); return CONST;}
"var" {print_token(); return VAR;}
"procedure" { print_token(); return PROC;}
"call" print_token(); {return CALL;}
"if" {print_token(); return IF;}
"then" {print_token(); return THEN;}
"while" {print_token(); return WHILE;}
"do" {print_token(); return DO;}
"odd" {print_token(); return ODD;}

[1-9][0-9]*|"0" {
	print_token();
	yylval.num=atoi(yytext);
	return NUM;
}

[a-zA-Z_][a-zA-Z_0-9]* {
	print_token();
	//yylval.id=new string(yytext);
	yylval.id=clone(yytext);
	//printf("id=%s\n",yytext);
	return ID;
}

":=" { print_token(); return ASSIGN; }
"<>" {print_token(); return NE;}
"<=" {print_token(); return LE; }
"<=" {print_token(); return GE; }

"=" {print_token(); return EQ; }
"<" {print_token(); return LT; }
">" {print_token(); return GT; }

"." {print_token(); return DOT;}
"," {print_token(); return COMMA;}
";" {print_token(); return SEMICOLON;}
"+" {print_token(); return PLUS;}
"-" {print_token(); return MINUS;}
"*" {print_token(); return MUL;}
"/" {print_token(); return DIV;}
"(" {print_token(); return LPAREN; }
")" {return RPAREN; }


[ \t\n] { //print_token(); 
		}
	;
. {printf("mistake char :%s\n",yytext); exit(-1);}
	;

 
%%
int yyerror(const char * format,...){
	printf("some error\n");
	return 1;
}
 
int yywrap(void)
{  
  return 1;  
} 
