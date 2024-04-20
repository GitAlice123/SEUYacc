/*
读入一个name.y文件，解析得到name.tab.h和name.tab.c
其中x.tab.h声明所有的token
x.tab.c实现了yyparse函数
*/
#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <queue>
using namespace std;

/**
*	所有产生式，每个产生式都有一个序号，这个序号为vector的下标
*	每个产生式使用pair<产生式左部，产生式右部>的数据结构存储
*	所有终结符和非终结符都使用整数代替，右部的产生式使用一个vector<int>存储
*/
vector<pair<int, vector<int>>> AllProducers;

/**
 * 所有状态，状态号就是下标
*/
vector<LRState>AllStates;


/**
 * 读入.y文件时，%token后面的单词和语法推导式中用单引号引起来的都是终结符
 * 语法推导式中间遇到的小写单词都是非终结符
 * 终结符和非终结符在读入后分别使用一个map存储，sym_name->sym_num
 * 终结符：map_terminal 非终结符：map_nonterminal
 * 同时需要区分哪些序号是终结符，哪些是非终结符
 * 这边采取终结符取正数，非终结符取负数的方式
*/
unordered_map<string,int>map_terminal;
unordered_map<string,int>map_nonterminal;



// LR(1)item
typedef struct LRItem {
	// dotPos starts from 0, ranges from 0 to n
	// n is the total number of symbols on the right side of producer
	// when the dot is exactly on the left of producer idx k
	// the dotPos=k 
	int dotPos = 0;//点的位置
	int LRitemID = -1;//产生式标号
	int predictiveSymbol;//预测符，每个item只放一个预测符
}LRItem; 



// DFA state
typedef struct {
	//<发出边上符号，状态号>
	unordered_map<int, int> edgesMap;
	unordered_set<LRItem> LRItemsSet;
}LRState;



/**
 * compute first set
 * notice that in c99.y, there's no epsilon on the right side of the producer
*/
unordered_set<int>ComputerFirst(int){
	
}



/**
 * 状态内部拓展
*/
void ExpandStateInside(int stateID){
	queue<LRItem>Q;
	// push all state's items into Q
	LRState this_state=AllStates[stateID];
	for(auto r:this_state.LRItemsSet){
		Q.push(r);
	}
	while(!Q.empty()){
		auto r=Q.front();
		Q.pop();

		// if the dot is on the rightmost position
		if(r.dotPos==AllProducers[r.LRitemID].second.size()){
			continue;
		}else{
			// 点后面的第一个符号
			int sym_after_dot=AllProducers[r.LRitemID].second[r.dotPos];
			// 如果是终结符
			if(sym_after_dot>0) continue;
			else{

			}
		}	
	}
}

