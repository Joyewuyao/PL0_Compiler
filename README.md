```
flex demo.l                                   //生成lex.yy.c文件
bison -d demo.y                               //生成demo.tab.c和demo.tab.h文件
g++ -o te.exe lex.yy.c demo.tab.c pl0.cpp     //连接生成test.exe可执行文件

测试：
please input pl0 source file name:test.pl0
please input result file name:result.txt
//生成的中间代码保存在result.txt文件中
```

