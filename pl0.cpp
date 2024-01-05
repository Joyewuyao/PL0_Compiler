#include "pl0.h"
#include <string>

const bool LOG_MODE = true; // 一个布尔常量，用于控制是否输出日志信息
extern int yyparse();		// 通常由 Bison 或 Yacc 这样的解析器生成器自动生成，用于执行语法分析

// 字符数组，存储输入和输出文件的名称
char in_name[128] = {0};
char out_name[128] = {0};

// 定义了一个字符串数组 mnemonic，其中包含了 PL/0 汇编指令的助记符。这些助记符代表不同的汇编指令，如 "LIT" (字面量) 或 "OPR" (操作)。
const char *mnemonic[8] = {
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC"};
// 定义了一个 Block 指针类型的向量 block_list。这个向量用于存储编译过程中的程序块（block）信息。
vector<Block *> block_list;

// 返回 block_list 中的最后一个元素。如果 block_list 为空，则返回 nullptr
Block *cur_block()
{
	if (block_list.empty())
		return nullptr;
	else
		return block_list.back();
}

/*语法分析中的临时栈,存储语法分析中的临时数据
旧的cx等
*/
// 定义了一个整型栈 temp_data。这个栈用于在语法分析过程中临时存储数据。
stack<int> temp_data;

const int MAX_CODE_COUNT = 2000;  // 代码数组的大小
Instruction code[MAX_CODE_COUNT]; // 用于存储生成的汇编指令
int cx;							  // 代码分配索引，可能用于追踪 code 数组中当前的写入位置。
int start_cx;					  // 当前block的开始地址

const int STACK_SIZE = 50000; // 指定解释器的栈空间大小
long data_stack[STACK_SIZE];  // 数据存储

// 用于复制给定的 C 字符串 src。函数会分配足够的内存，复制字符串内容，并返回新的字符串指针。
char *clone(const char *src)
{
	size_t L = strlen(src);
	char *p = new char[L + 1];
	strncpy(p, src, L);
	p[L] = 0;

	return p;
}

// 从基底b开始向上在SL链跳约l层,返回最后一次的地址
// 用于在所谓的静态链（SL链）中计算从基底 b 开始、向上跳 l 层后的地址。这个函数在实现诸如变量作用域链这样的功能时非常有用。
long base(long b, long l, int data_area[])
{
	long b1;

	b1 = b;

	while (l > 0) // find base l levels down
	{
		b1 = data_area[b1];
		l = l - 1;
	}

	return b1;
}

// 用于向当前程序块添加一个常量。
void add_const_item(char *name, int value)
{
	Block *block = cur_block();
	assert(block != nullptr); // 确保当前程序块不是空指针，这是一个调试时的安全检查。

	// 如果日志模式（LOG_MODE）开启，则打印添加常量的日志信息。
	if (LOG_MODE)
	{
		printf("### add a constant  %s = %d to block %s at level %d\n", name, value, block->name, block->level);
	}
	block->add_const_item(name, value); // 在当前程序块中添加一个新的常量项，名称和值由参数 name 和 value 指定。
}

// 用于向当前程序块添加一个变量
void add_var_item(char *name)
{
	Block *block = cur_block();
	assert(block != nullptr); // 确保当前程序块不为空
	// 在日志模式下打印添加变量的信息
	if (LOG_MODE)
	{
		printf("### add a variable  %s  to block %s at level %d\n", name, block->name, block->level);
	}
	// 在当前程序块中添加一个新的变量项，名称由参数 name 指定。
	block->add_var_item(name);
}

// 增加一条pl/0 汇编指令
// 负责生成中间代码。根据给定的指令类型、层级、地址等信息，生成相应的汇编指令并存储在code数组中。
void gen(InstructionType f, long l, long a, const char *hint)
{
	code[cx].no = cx;		 // 设置当前指令的编号为 cx（代码分配索引）。
	code[cx].f = f;			 // 设置指令的功能代码为参数 f。
	code[cx].level = l;		 // 设置指令的层级为参数 l。
	code[cx].addr = a;		 // 设置指令的地址或数值为参数 a。
	code[cx].comment = hint; // 为指令添加一个注释，由参数 hint 提供。
	// 在日志模式下打印生成的指令信息
	if (LOG_MODE)
	{
		printf("### generate a new code:\n###");
		code[cx].print();
	}
	// 增加代码分配索引，为下一条指令的生成做准备
	++cx;
}
// 用于显示当前的程序块列表
void show_block_list()
{
	printf("### current block_list: "); // 打印提示信息
	for (auto &e : block_list)			// 遍历 block_list 中的每个元素
	{
		printf("%s ", e->name); // 打印每个程序块的名称
	}
	printf("\n");
}

// 遇到一个新的过程
// 创建一个新的程序块（过程）
void create_proc(char *name)
{
	Block *father = cur_block();			// 获取当前活动的程序块（父程序块）
	Block *block = new Block(name, father); // 创建一个新的 Block 实例，命名为 name，并将 father 设置为其父程序块
	// 如果日志模式开启，则打印日志信息，包括新程序块的层级和名称
	if (LOG_MODE)
	{
		printf("### at level %d add a new program %s \n", block->level - 1, name);
	}
	// 如果存在父程序块，将新创建的程序块加入父程序块的子程序列表中
	if (father != nullptr)
	{
		father->add_child_proc(block);
	}
	// 将新程序块加入到程序块列表中
	block_list.push_back(block);
	// 在日志模式下，显示当前的程序块列表
	if (LOG_MODE)
	{
		show_block_list();
	}
}
// 结束当前程序块的编译
void end_proc()
{
	Block *block = cur_block(); // 获取当前活动的程序块
	assert(block != nullptr);	// 确保当前程序块不为空

	char hint[128] = {0};
	sprintf(hint, "return from program %s ", block->name);

	// 生成返回语句
	gen(OPR, 0, 0, hint); // 生成一条返回操作的汇编指令

	printf("%s\n", hint);
	block->end_addr = cx; // 设置当前程序块的结束地址

	printf("### the code generated for block %s is:\n", block->name); // 打印生成的代码信息
	block->show_code();												  // 展示当前程序块的汇编代码
	printf("### will exit from block %s.\n", block->name);

	if (block->father != nullptr)
	{
		printf("### will return to father block %s\n", block->father->name);
	}

	block_list.pop_back(); // 从程序块列表中移除当前程序块

	if (LOG_MODE) // 在日志模式下，显示更新后的程序块列表
	{
		show_block_list();
	}
}
// 开始生成当前过程的代码
void begin_proc_code()
{
	Block *block = cur_block(); // 获取当前活动的程序块
	assert(block != nullptr);	// 确保当前程序块不为空

	block->start_addr = cx; // 设置程序块的起始地址

	block->fill_addr(); // 在所有调用函数的call 语句中回填本函数地址

	int dx = block->var_table.size();
	char hint[128] = {0};
	sprintf(hint, "start of program %s with %d vars has level %d start from addr %d", block->name, dx, block->level, cx);
	gen(INT, 0, dx + 3, hint); // 生成一条分配内存的汇编指令，用于初始化程序块的局部变量。

	printf("%s\n", hint);
}
// 创建一个程序的主入口
void create_program()
{
	create_proc(clone("_main")); // 创建一个名为 "_main" 的主程序块
}
// 结束程序的编译
void end_program()
{
	FILE *fp = fopen(out_name, "w"); // 打开输出文件以写入编译后的代码
	for (int i = 0; i < cx; ++i)	 // 遍历所有生成的汇编指令，并将它们写入文件
	{
		code[i].print(fp);
	}
	fclose(fp); // 关闭文件
	printf("result code has save to file %s\n", out_name);
}
// 用于将一个数值加载到运行时栈顶
void load(int value)
{
	char hint[128] = {0};							 // 存储提示信息
	sprintf(hint, "push number %d to stack", value); // 将提示信息格式化存入 hint 中。提示信息表明将要将一个数值推送到栈顶
	gen(LIT, 0, value, hint);						 // 生成一个字面量（LIT）指令，用于将数值 value 推送到栈顶
}
// 用于加载一个变量或常量的值到栈顶
void load(const char *name)
{
	Block *b = cur_block(); // 获取当前活动的程序块
	assert(b != nullptr);	// 确保当前程序块不为空

	Block *p = b; // 初始化一个指针 p 指向当前程序块
	char hint[128] = {0};
	while (p != NULL) // 在程序块链中向上查找
	{
		int value = 0;
		if (p->get_const_value(name, value)) // 如果在当前程序块找到了名为 name 的常量，则生成指令将其值加载到栈顶。
		{
			sprintf(hint, "push constant %s @ %s = %d", name, p->name, value);
			gen(LIT, 0, value, hint);
			break;
		}

		int offset = 0;
		if (p->get_var_offset(name, offset)) // 如果找到了变量，则生成指令将其值加载到栈顶。
		{
			int diff = b->level - p->level;
			sprintf(hint, "push %s @ %s[%d] at level %d", name, p->name, offset - 3, p->level);
			gen(LOD, diff, offset, hint);
			break;
		}

		p = p->father; // 如果在当前程序块找不到，则移动到父程序块继续查找
	}
}
// 用于将栈顶的值存储到指定变量中
void write_var(const char *name)
{
	Block *b = cur_block();
	assert(b != nullptr);

	Block *p = b;
	char hint[128] = {0};
	while (p != NULL)
	{
		int offset = 0;						 // 用于存储变量在数据区的偏移量
		if (p->get_var_offset(name, offset)) // 如果找到了变量，则生成指令将栈顶的值存储到这个变量的位置。
		{
			int diff = b->level - p->level;
			sprintf(hint, "pop top to %s @ %s[%d] at level %d ", name, p->name, offset - 3, p->level);
			gen(STO, diff, offset, hint);
			break;
		}

		p = p->father; // 如果在当前程序块找不到，则移动到父程序块继续查找
	}
}
// 在当前程序块和其祖先中查找给定名称的程序块
Block *find_block(const char *proc_name)
{
	Block *b = cur_block();
	assert(b != nullptr);

	// 现在孩子里查找
	for (auto e : b->children)
	{
		if (strcmp(e->name, proc_name) == 0)
			return e;
	}

	// 然后在祖先里查找
	Block *p = b;
	while (p != NULL && strcmp(p->name, proc_name) != 0)
		p = p->father;

	return p;
}
// 在 if 条件为假时生成跳转指令（JPC）
void after_if_conditon()
{
	gen(JPC, 0, 0, "if false,jump to after if");
	temp_data.push(cx - 1);
}

// 回填 if 语句的跳转地址
void after_if_body()
{
	// 回填if的跳转地址
	int old_addr = temp_data.top();
	temp_data.pop();
	code[old_addr].addr = cx;
}

// 记录 while 循环开始的地址
void before_while_conditon()
{
	temp_data.push(cx); // 记录while的入口地址
}

// 在 while 条件为假时生成跳转指令
void after_while_conditon()
{
	gen(JPC, 0, 0, "if false,jump to after while");
	temp_data.push(cx - 1);
}

// 回填 while 循环的出口地址，并生成跳回循环开始的指令。
void after_while_body()
{
	int old_addr = temp_data.top();
	temp_data.pop();
	int while_enter_addr = temp_data.top();
	temp_data.pop();

	gen(JMP, 0, while_enter_addr, "jump back to while");
	code[old_addr].addr = cx; // 回填while的出口地址
}

// 构造函数 初始化程序块实例，设置程序块的名称和父程序块。如果存在父程序块，则将当前程序块的层级设置为父程序块层级加一。
Block::Block(char *name, Block *father)
	: name(name), father(father)
{
	if (father != NULL)
	{
		level = father->level + 1;
	}
}
// 打印当前程序块生成的汇编代码。遍历程序块内所有的汇编指令（从 start_addr 到 end_addr），并打印每条指令。
void Block::show_code()
{
	printf("generated code for program %s:\n", name);
	for (int i = start_addr; i < end_addr; i++)
	{
		// printf("%10d%5s%3d%5d\n", i, mnemonic[code[i].f], code[i].level, code[i].addr);
		code[i].print();
	}
}
// 在程序块的常量表中查找指定名称的常量。如果找到，则返回 true 并通过引用参数 value 返回常量值；否则返回 false。
bool Block::get_const_value(const string &name, int &value) const
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
// 在程序块的变量表中查找指定名称的变量。如果找到，返回 true 并通过引用参数 offset 返回变量的偏移量；否则返回 false。
bool Block::get_var_offset(const string &name, int &offset) const
{
	int L = var_table.size();
	for (int i = 0; i < L; ++i)
	{
		if (var_table[i] == name)
		{
			offset = i + 3;
			return true;
		}
	}

	return false;
}
// 遍历 ref_addrs（引用地址列表），为每个引用地址的指令设置起始地址
void Block::fill_addr() const
{
	for (const auto &e : ref_addrs)
	{
		code[e].addr = start_addr;
	}
}
// Instruction 类 将一条指令打印到文件。根据指令类型（如 INT），打印指令的编号、类型、层级和地址，以及注释。
void Instruction::print(FILE *fp) const
{
	if (f == INT)
	{
		fprintf(fp, "\n\t//%s\n", comment.c_str());
		fprintf(fp, "\tNO.      f     l    a\n");
		fprintf(fp, "\t%-6d%5s%5d%5d\n", no, mnemonic[f], level, addr);
	}
	else
		fprintf(fp, "\t%-6d%5s%5d%5d\t//%s\n", no, mnemonic[f], level, addr, comment.c_str());
}

int main(int argc, char *argv[])
{
	// 这部分代码被预处理器指令包围，因此在实际编译时不会被包含。它的作用是为了测试或特定场景预先设置输入和输出文件名，但目前已被禁用。
#if 0
	int caseno = 1;
	sprintf(in_name, "test%02d.pl0", caseno);
	sprintf(out_name, "test%02d_code.txt", caseno);
#endif
// 这部分代码在编译时会被包含
#if 1
	printf("please input pl0 source file name:"); // 请求用户输入 PL/0 源文件的名称。
	fgets(in_name, 128, stdin);					  // 从标准输入读取最多 128 个字符到 in_name 数组中。
	char *p = strchr(in_name, '\n');			  // 在 in_name 中查找换行符（\n）
	// 如果找到换行符，则将其替换为字符串结束符（\0），这样做是为了移除输入字符串中的换行符。
	if (p != nullptr)
	{
		*p = 0;
	}

	printf("please input result file name:"); // 请求用户输入结果文件的名称。
	fgets(out_name, 128, stdin);
	p = strchr(out_name, '\n');
	if (p != nullptr)
	{
		*p = 0;
	}

#endif

	freopen(in_name, "r", stdin); // 将标准输入重定向到指定的输入文件，这意味着后续的读取操作将从该文件读取数据而不是从标准输入（如键盘）读取
	yyparse();					  // 调用 yyparse() 函数开始语法分析。yyparse() 通常由 Bison 或 Yacc 这类工具自动生成，用于解析给定的语法。
	return 0;
}
