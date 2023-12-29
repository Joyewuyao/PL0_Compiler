#include "pl0.h"

const bool LOG_MODE = true;
extern int yyparse();

char in_name[128] = { 0 };
char out_name[128] = { 0 };

const char* mnemonic[8] ={
	"LIT"
	,"OPR"
	,"LOD"
	,"STO"
	,"CAL"
	,"INT"
	,"JMP"
	,"JPC"
};

vector<Block*> block_list;

Block* cur_block() {
	if (block_list.empty())
		return nullptr;
	else
		return block_list.back();
}

/*语法分析中的临时栈,存储语法分析中的临时数据
旧的cx等
*/
stack<int> temp_data; 

const int MAX_CODE_COUNT = 2000;         // size of code array
Instruction code[MAX_CODE_COUNT];
int cx;						// code allocation index
int start_cx;               //当前block的开始地址


// the following array space for interpreter
const int STACK_SIZE = 50000;
long data_stack[STACK_SIZE];	// datastore

char* clone(const char* src){
	size_t L = strlen(src);
	char* p = new char[L + 1];
	strncpy(p, src, L);
	p[L] = 0;

	return p;
}

//从基底b开始向上在SL链跳约l层,返回最后一次的地址
long base(long b,long l,int data_area[])
{
	long b1;

	b1 = b;

	while (l > 0)         // find base l levels down
	{
		b1 = data_area[b1];
		l = l - 1;
	}

	return b1;
}

void add_const_item(char* name, int value) {
	Block* block = cur_block();
	assert(block != nullptr);

	if (LOG_MODE)
	{
		printf("### add a constant  %s = %d to block %s at level %d\n"
			, name, value, block->name,block->level
		);
	}
	block->add_const_item(name, value);
}

void add_var_item(char* name) {
	Block* block = cur_block();
	assert(block != nullptr);

	if (LOG_MODE)
	{
		printf("### add a variable  %s  to block %s at level %d\n"
			, name, block->name,block->level
		);
	}
	block->add_var_item(name);
}

//增加一条pl/0 汇编指令
void gen(InstructionType f, long l, long a,const char* hint){
	code[cx].no = cx;
	code[cx].f = f; 
	code[cx].level = l; 
	code[cx].addr = a;
	code[cx].comment = hint;

	if (LOG_MODE)
	{
		printf("### generate a new code:\n###");
		code[cx].print();
	}

	++cx;
}

void show_block_list() {
	printf("### current block_list: ");
	for (auto& e : block_list) {
		printf("%s ", e->name);
	}
	printf("\n");
}


//遇到一个新的过程
void create_proc(char* name) {
	Block* father = cur_block();
	Block* block = new Block(name,father);

	if (LOG_MODE) {
		printf("### at level %d add a new procedure %s \n"
			, block->level - 1, name);
	}
	
	if (father != nullptr){
		father->add_child_proc(block);
	}

	block_list.push_back(block);

	if (LOG_MODE) {
		show_block_list();
	}
}

void end_proc() {
	Block* block = cur_block();
	assert(block != nullptr);
	
	

	char hint[128] = { 0 };
	sprintf(hint, "return from proc %s "
		, block->name);

	//生成返回语句
	gen(OPR, 0, 0, hint); // return

	printf("%s\n", hint);
	block->end_addr = cx;

	printf("### the code generated for block %s is:\n", block->name);
	block->show_code();
	printf("### will exit from block %s.\n", block->name);

	if (block->father!=nullptr){
		printf("### will return to father block %s\n", block->father->name);
	}

	block_list.pop_back();

	if (LOG_MODE)
	{
		show_block_list();
	}
}

void begin_proc_code()
{
	Block* block = cur_block();
	assert(block != nullptr );

	block->start_addr = cx;

	block->fill_addr(); //在所有调用函数的call 语句中回填本函数地址

	int dx = block->var_table.size();
	char hint[128] = { 0 };
	sprintf(hint, "start of proc %s with %d vars has level %d start from addr %d"
		, block->name, dx
		,block->level
		,cx);
	gen(INT, 0, dx + 3, hint);

	printf("%s\n", hint);
}

void create_program()
{
	create_proc(clone("_main"));
}

void end_program() {
	FILE* fp = fopen(out_name, "w");
	for (int i=0;i<cx;++i)
	{
		code[i].print(fp);
	}
	fclose(fp);
	printf("result code has save to file %s\n", out_name);
}

void load(int value)
{
	char hint[128] = { 0 };
	sprintf(hint, "push number %d to stack", value);
	gen(LIT, 0, value, hint);
	
}

void load(const char* name)
{
	Block *b = cur_block();
	assert(b != nullptr);
	
	Block* p = b;
	char hint[128] = { 0 };
	while (p !=NULL)
	{
		int value = 0;
		if (p->get_const_value(name,value))
		{
			sprintf(hint,"push constant %s @ %s = %d"
				, name, p->name, value);
			gen(LIT, 0, value, hint);
			break;
		}

		int offset = 0;
		if (p->get_var_offset(name,offset))
		{
			int diff = b->level - p->level ;
			sprintf(hint
				,"push %s @ %s[%d] at level %d"
				, name, p->name,offset-3,p->level);
			gen(LOD, diff, offset,hint);
			break;
		}

		p = p->father;
	}
}

void write_var(const char* name)
{
	Block *b = cur_block();
	assert(b != nullptr);

	Block* p = b;
	char hint[128] = { 0 };
	while (p != NULL)
	{
		int offset = 0;
		if (p->get_var_offset(name, offset))
		{
			int diff = b->level - p->level;
			sprintf(hint
				, "pop top to %s @ %s[%d] at level %d "
				, name, p->name, offset-3, p->level);
			gen(STO, diff, offset, hint);
			break;
		}

		p = p->father;
	}
}

Block* find_block(const char* proc_name) {
	Block *b = cur_block();
	assert(b != nullptr);


	//现在孩子里查找
	for (auto e:b->children){
		if (strcmp(e->name,proc_name)==0)
			return e;
	}

	//然后在祖先里查找
	Block* p = b;
	while (p != NULL && strcmp(p->name, proc_name) != 0)
		p = p->father;
	
	return p;
}

void call(const char* proc_name)
{
	Block *b = cur_block();
	assert(b != nullptr);

	Block* p = find_block(proc_name);
	assert(p != NULL);
	
	int addr = p->start_addr;
	int diff = b->level - p->father->level;  //这里要计算的是p的父亲和b的层次差,pl0保证了p的父亲>=b
	char hint[128] = { 0 };
	sprintf(hint, "call proc %s", proc_name);
	gen(CAL, diff, addr,hint);
	if (addr < 0){
		//如果p的起始地址尚未确定,则把当前地址加入到p的引用列表里,等待以后回填
		p->add_ref(cx - 1);
	}
}

void after_if_conditon(){
	gen(JPC, 0, 0, "if false,jump to after if");
	temp_data.push(cx-1);
}

void after_if_body()
{
	//回填if的跳转地址
	int old_addr = temp_data.top();
	temp_data.pop();
	code[old_addr].addr = cx;
}

void before_while_conditon()
{
	temp_data.push(cx); //记录while的入口地址
}

void after_while_conditon()
{
	gen(JPC, 0, 0, "if false,jump to after while");
	temp_data.push(cx - 1);
}


void after_while_body()
{
	int old_addr = temp_data.top();
	temp_data.pop();
	int while_enter_addr = temp_data.top();
	temp_data.pop();

	gen(JMP, 0, while_enter_addr, "jump back to while");
	code[old_addr].addr = cx;  //回填while的出口地址
}


Block::Block(char* name, Block* father)
	:name(name),father(father)
{
	if (father!=NULL){
		level = father->level + 1;
	}
}

void Block::show_code()
{
	printf("generated code for procedure %s:\n", name);
	for (int i = start_addr; i < end_addr; i++) {
		//printf("%10d%5s%3d%5d\n", i, mnemonic[code[i].f], code[i].level, code[i].addr);
		code[i].print();
	}
}

bool Block::get_const_value(const string& name, int &value) const
{
	auto it = const_table.find(name);
	if (it != const_table.end())
	{
		value = it->second;
		return true;
	}
	else
		return false;
}

bool Block::get_var_offset(const string& name, int &offset) const
{
	int L = var_table.size();
	for (int i=0;i<L;++i)
	{
		if (var_table[i] == name){
			offset = i + 3;
			return true;
		}
	}

	return false;
}

void Block::fill_addr() const
{
	for (const auto& e:ref_addrs){
		code[e].addr = start_addr;
	}
}

void Instruction::print(FILE* fp) const
{
	if (f == INT) {
		fprintf(fp, "\n\t//%s\n",comment.c_str());
		fprintf(fp, "\tNO.      f     l    a\n");
		fprintf(fp, "\t%-6d%5s%5d%5d\n", no, mnemonic[f], level, addr);
	}
	else
		fprintf(fp,"\t%-6d%5s%5d%5d\t//%s\n",no,mnemonic[f], level, addr,comment.c_str());
}

int main(int argc, char* argv[])
{
#if 0
	int caseno = 1;
	sprintf(in_name, "test%02d.pl0", caseno);
	sprintf(out_name, "test%02d_code.txt", caseno);
#endif
#if 1
	printf("please input pl0 source file name:");
	fgets(in_name, 128, stdin);
	char* p = strchr(in_name, '\n');
	if (p != nullptr){
		*p = 0;
	}
	
	printf("please input result file name:");
	fgets(out_name, 128, stdin);
	p = strchr(out_name, '\n');
	if (p != nullptr)
	{
		*p = 0;
	}
	
#endif

	freopen(in_name, "r", stdin);
	yyparse();
	return 0;
}
