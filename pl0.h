#pragma once
#include<cstdio>
#include<cstdlib>	
#include <cctype>
#include <cstring>
#include <cassert>

#include <map>
#include <stack>
#include <vector>
#include <set>
using namespace std;

//extern int num;
//extern string id;

char* clone(const char* src);

enum InstructionType
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC         // functions
};

/*
lit 0, a : load constant a
opr 0, a : execute operation a
lod l, a : load variable l, a
sto l, a : store variable l, a
cal l, a : call procedure a at level l
Int 0, a : increment t-register by a
jmp 0, a : jump to a
jpc 0, a : jump conditional to a
*/

struct Instruction
{
	int no;
	InstructionType f;		// function code
	long level; 			// level
	long addr; 				// displacement address
	string comment;		

	void print(FILE* fp=stdout)const;
};

#if 0
enum SymbolType{
	CONSTANT, VARIABLE, PROCEDURE
};
#endif

struct Block {
	explicit Block(char* name,Block* father);
	char* name;
	Block* father;

	map<string, int> const_table;
	vector<string> var_table;
	
	int start_addr = -1;
	int end_addr =0;
	int level = 0;
	vector<Block*> children;

	void add_var_item(char* name) {
		var_table.push_back(name);
	}

	void add_const_item(char* name, int value) {
		const_table[name] = value;
	}

	void add_child_proc(Block *block) {
		children.push_back(block);
	}

	void show_code();

	bool get_const_value(const string& name,int &value)const;
	bool get_var_offset(const string& name,int &offset)const;

	set<int> ref_addrs; //需要用到本函数首地址的call语句列表
	void fill_addr()const;  //回填本函数的首地址
	void add_ref(int addr) {
		ref_addrs.insert(addr);
	}
};

void add_const_item(char* name, int value);
void add_var_item(char* name);


void create_proc(char* name);
void end_proc();

void begin_proc_code();


void create_program();
void end_program();

void load(int value);
void load(const char* name);

void write_var(const char* name);

void call(const char* proc_name);

void after_if_conditon();
void after_if_body();

void before_while_conditon();
void after_while_conditon();
void after_while_body();


void gen(InstructionType f, long l, long a, const char* hint);
/*
struct Symbol {
	char* name;
	SymbolType kind;
	int val;
	int level;
	int addr;
};
*/

#if 0
/*
#define norw        11             // no. of reserved words
#define txmax       100            // length of identifier table
#define nmax        14             // max. no. of digits in numbers
#define al          10             // length of identifiers
#define amax        2047           // maximum constatnt value(also is maximun address)
#define levmax      3              // maximum depth of block nesting
#define cxmax       2000           // size of code array
*/


enum ObjectType
{
	CONSTANT, VARIABLE, PROCEDURE
};




//extern char* ERR_MSG[];
#endif














