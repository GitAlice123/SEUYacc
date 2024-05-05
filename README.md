# SEUYacc
## 语言
* C++
## 功能
* 分析C99.y
## 环境依赖
* 需要下载graphviz
## 运行方法
* 首先编译myYacc.cpp，生成myYacc.tab.cpp和myYacc.tab.h
* 然后编译myYacc.tab.cpp为可执行文件(parser)
* 编写token.txt
* 运行parser，传入token.txt
## 备注
*为了编写方便，将c99.y中的用引号括起来的终结符（如','这种直接ECHO的符号）换成了终结符的名字，在c99.y中可见
