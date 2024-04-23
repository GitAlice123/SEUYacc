#pragma GCC optimize("O2")
/*
读入一个name.y文件，解析得到name.tab.h和name.tab.c
其中x.tab.h声明所有的token
x.tab.c实现了yyparse函数
*/
#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <map>
#include <unordered_set>
#include <string>
#include <queue>
#include <fstream>
#include <sstream>
using namespace std;

// LR(1)item
typedef struct LRItem
{
	// dotPos starts from 0, ranges from 0 to n
	// n is the total number of symbols on the right side of producer
	// when the dot is exactly on the left of producer idx k
	// the dotPos=k
	int dotPos;			  // 点的位置
	int producerID;		  // 产生式标号
	int predictiveSymbol; // 预测符，每个item只放一个预测符

	bool operator<(const LRItem &other) const
	{
		// 按顺序比较所有属性
		if (dotPos != other.dotPos)
		{
			return dotPos < other.dotPos;
		}
		if (producerID != other.producerID)
		{
			return producerID < other.producerID;
		}
		if (predictiveSymbol != other.predictiveSymbol)
		{
			return predictiveSymbol < other.predictiveSymbol;
		}
		// 所有属性都相等，返回 false
		return false;
	}

	bool operator==(const LRItem &other) const
	{
		return dotPos == other.dotPos && producerID == other.producerID && predictiveSymbol == other.predictiveSymbol;
	}
	LRItem() = default;
	LRItem(int DotPos = 0, int ID = -1, int preSym = 0) : dotPos(DotPos), producerID(ID), predictiveSymbol(preSym) {}
} LRItem;

// DFA state
typedef struct LRState
{
	//<发出边上符号，状态号>
	map<int, int> edgesMap;
	set<LRItem> LRItemsSet;
	LRState() : edgesMap(), LRItemsSet() {}

} LRState;


// 状态数
int num_state;
// 非终结符数
int num_NTermin;
// 终结符数
int num_Termin;


/**
 *	所有产生式，每个产生式都有一个序号，这个序号为vector的下标
 *	每个产生式使用pair<产生式左部，产生式右部>的数据结构存储
 *	所有终结符和非终结符都使用整数代替，右部的产生式使用一个vector<int>存储
 */
vector<pair<int, vector<int>>> AllProducers;

/**
 * 所有状态，状态号就是下标
 */
vector<LRState> AllStates;

/**
 * 文法开始符号
 */
int start_sym;

/**
 * 为了方便后续使用，建立一个表记录对于每个非终结符，
 * 他在哪些产生式中位于产生式左侧
 * 由于读入的时候按顺序，所以只需要记录产生式vec中的起止标号
 */
map<int, pair<int, int>> left_producer_range;

/**
 * 读入.y文件时，%token后面的单词和语法推导式中用单引号引起来的都是终结符
 * 语法推导式中间遇到的小写单词都是非终结符
 * 终结符和非终结符在读入后分别使用一个map存储，sym_name->sym_num
 * 终结符：map_terminal 非终结符：map_nonterminal
 * 同时需要区分哪些序号是终结符，哪些是非终结符
 * 这边采取终结符取正数，非终结符取负数的方式，0为S文法开始符号非终结符
 */
map<string, int> map_symbols;

/**
 * compute first set of symbol X
 */
unordered_set<int> ComputerFirst(int X)
{
	// notice that in c99.y, there'_S no epsilon on the right side of the producer.

	unordered_set<int> first_set;
	if (X > 0)
	{
		// X is terminal
		first_set.insert(X);
	}
	else
	{
		// X is nonterminal
		// iterate all the producers whose left is X
		// notice that with the sequence of reading in the .y
		// the producers whose left is X always stay together
		// so we just need to find the first X
		int total_pro_num = AllProducers.size();
		bool flag = false;
		int begin = left_producer_range[X].first;
		int end = left_producer_range[X].second;
		for (int i = begin; i <= end; i++)
		{
			if(AllProducers[i].second[0]==X) continue;
			first_set.merge(ComputerFirst(AllProducers[i].second[0]));
		}
	}
	return first_set;
}

/**
 * 状态内部拓展
 */
void ExpandStateInside(LRState& this_state)
{
	queue<LRItem> Q;
	// // push all state'_S items into Q
	// LRState& this_state = AllStates[stateID];
	for (auto r : this_state.LRItemsSet)
	{
		Q.push(r);
	}
	while (!Q.empty())
	{
		auto r = Q.front();
		Q.pop();

		vector<int> &rightside_of_producer = AllProducers[r.producerID].second;
		// if the dot is on the rightmost position
		if (r.dotPos == rightside_of_producer.size())
		{
			continue;
		}
		else
		{
			// 点后面的第一个符号
			int sym_after_dot = rightside_of_producer[r.dotPos];
			if (sym_after_dot > 0)
			{
				// if terminal
				continue;
			}
			else
			{
				// if nonterminal
				// if A->·BC,γ compute first(C)
				if (r.dotPos < rightside_of_producer.size() - 1)
				{
					auto test=rightside_of_producer[r.dotPos + 1];
					auto predict_set = ComputerFirst(test);
					// find the producers with sym_after_dot on the left
					int begin = left_producer_range[sym_after_dot].first;
					int end = left_producer_range[sym_after_dot].second;
					for (int i = begin; i <= end; i++)
					{
						for (auto k : predict_set)
						{
							// k is one sym in the predict_set
							// create a new item
							LRItem new_item(0, i, k);
							auto it = this_state.LRItemsSet.find(new_item);
							if (this_state.LRItemsSet.find(new_item) == this_state.LRItemsSet.end())
							{
								this_state.LRItemsSet.insert(new_item);
								Q.push(new_item);
							}
						}
					}
				}
				else
				{
					// A->·B,γ the first set is γ
					int begin = left_producer_range[sym_after_dot].first;
					int end = left_producer_range[sym_after_dot].second;
					for (int i = begin; i <= end; i++)
					{
						LRItem new_item(0, i, r.predictiveSymbol);
						if (this_state.LRItemsSet.find(new_item) == this_state.LRItemsSet.end())
						{
							this_state.LRItemsSet.insert(new_item);
							Q.push(new_item);
						}
					}
				}
			}
		}
	}
}

/**
 * 建立LR(1)DFA
 */
void createLR1DFA()
{
	// 创建0号item，放入0号state
	// 读入.y的时候，产生式编号从1开始
	// 0号item为S->·start_sym,#
	LRItem zero_item(0, 0, 1);
	LRState zero_state;
	zero_state.LRItemsSet.insert(zero_item);
	AllStates.push_back(zero_state);
	// Q记录待拓展的state_id
	queue<int> Q;
	Q.push(0);
	auto& test=AllStates[0];
	ExpandStateInside(test);
	// 记录每个state当前暂时生成的所有新状态
	// 先存进来，但是有可能和之前的状态有重复，就不再增加新状态
	// temp_state_set_map:从当前这个状态会发出去的线上的字符->可能的新状态
	// 如果重复了，不增加状态但是要edgesMap里面增加连线
	map<int, LRState> temp_state_set_map;
	while (!Q.empty())
	{
		auto cur_state_id = Q.front();
		cout<<"当前要外部拓展的state："<<cur_state_id<<endl;
		// ExpandStateInside(cur_state_id);
		auto cur_s = AllStates[cur_state_id];
		temp_state_set_map.clear();
		Q.pop();
		// 遍历这个state里面每个LRitem r
		for (auto r : cur_s.LRItemsSet)
		{
			// 得到这个r的产生式右半边的vector
			vector<int> &right_part = AllProducers[r.producerID].second;
			// 如果当前这个r的点在最后面，直接略过
			if (r.dotPos == right_part.size())
				continue;
			// 点后面的符号
			int sym_after_dot = right_part[r.dotPos];

			LRItem new_item(r.dotPos + 1, r.producerID, r.predictiveSymbol);
			if (temp_state_set_map.find(sym_after_dot) != temp_state_set_map.end())
			{
				temp_state_set_map[sym_after_dot].LRItemsSet.insert(new_item);
			}
			else
			{
				LRState newstate;
				newstate.LRItemsSet.insert(new_item);
				temp_state_set_map[sym_after_dot] = newstate;
			}
		}
		// 检查该temp_state_set_map中是否有新状态产生

		bool ok = true;
		// 对每个刚才生成的新状态检查，是否有重复状态
		// 必须要内部拓展之后才能进行比较
		for (auto& temp_state : temp_state_set_map)
		{
			ExpandStateInside(temp_state.second);
			ok = true;
			// 遍历所有已有的状态
			// TODO：跑太慢了，如何优化？
			// 把没有内部拓展的所有状态也记录一下，比较的时候只比较没有内部拓展的
			for (int i = 0; i < AllStates.size(); i++)
			{
				if (AllStates[i].LRItemsSet == temp_state.second.LRItemsSet)
				{
					ok = false;
					// 加入edgesMap
					AllStates[cur_state_id].edgesMap.insert(pair<int, int>(temp_state.first, i));
					break;
				}
			}
			if (ok)
			{
				// 这是一个可以接受的新状态
				AllStates.push_back(temp_state.second);
				// 加入队列，新状态的下标为AllStates.size()-1
				int new_state_idx = AllStates.size() - 1;
				Q.push(new_state_idx);
				// 加入edgesMap
				// AllStates[cur_state_id].edgesMap.insert(pair<int, int>(temp_state.first, new_state_idx));
				AllStates[cur_state_id].edgesMap[temp_state.first]=new_state_idx;
			}
		}
	}
	num_state=AllStates.size();
}


/**
 * 初始化全局存储量
*/
void init()
{
	// 初始化0号产生式和0号非终结符S
	AllProducers.clear();
	AllProducers.push_back(pair<int, vector<int>>(0, {start_sym}));
	// 填写left_producer_range
	left_producer_range[0] = pair<int, int>(0, 0);
	// 设置'#'为文法结束符号，也即设置它的序号
	map_symbols["_S"] = 0;
	map_symbols["#"] = 1;
}

/**
 * 读入.y文件
*/
int readYaccFile(const char *filename)
{

	// TODO:读入.y文件
	ifstream file(filename);
	if (!file)
	{
		std::cerr << "Failed to open file." << std::endl;
		return 1;
	}
	string line;
	int value_termin = 2;
	int value_nontermin = -1;

	/**
	 * 读取token部分
	 */
	while (getline(file, line))
	{
		if (line == "")
			continue;
		// 每一个line都是token序列
		if (line.substr(0, 6) != "\%token")
			break;
		string token;
		istringstream iss(line);
		iss >> token;
		while (iss >> token)
		{
			map_symbols[token] = value_termin++;
		}
	}

	/**
	 * 读取start_sym
	 */
	map_symbols[line.substr(7)] = value_nontermin;
	start_sym = value_nontermin--;

	init();

	/**
	 * 读取产生式，过程中填写left_producer_range
	 */
	// 读取%%
	getline(file, line);
	// 开始读取产生式部分
	getline(file, line);
	while (line != "\%\%")
	{
		// 每次循环读取一组产生式，以一个左符开始的为一组
		if (line == "")
		{
			getline(file, line);
			continue;
		}
		// 读取左符
		string left_str = line;
		// 看左符是否已经被注册
		if (map_symbols.find(left_str) == map_symbols.end())
			// 注册左符
			map_symbols[left_str] = value_nontermin--;
		// 循环读取这一组产生式
		// 这一组产生式从这个下标开始存
		int cur_producer_idx = AllProducers.size();
		left_producer_range[map_symbols[left_str]].first = cur_producer_idx;
		getline(file, line);
		while (line != "\t;")
		{
			// 每次循环读入一个产生式(占一行)
			AllProducers.push_back(pair<int, vector<int>>(map_symbols[left_str], vector<int>()));
			string word;
			istringstream iss(line);
			// 把:或者|读进来
			iss >> word;
			while (iss >> word)
			{
				// 检查是否已经注册
				if (map_symbols.find(word) == map_symbols.end())
				{
					// 没有注册过，注册之
					// 检查是终结符还是非终结符，如果是终结符只可能是普通符号
					if (word[0] == '\'')
					{
						// 开头是引号的，是普通符号，注册
						map_symbols[word] = value_termin++;
					}
					else
					{
						// 是非终结符
						map_symbols[word] = value_nontermin--;
					}
				}
				// 放入产生式vector中
				AllProducers[cur_producer_idx].second.push_back(map_symbols[word]);
			}
			left_producer_range[map_symbols[left_str]].second = cur_producer_idx;
			cur_producer_idx++;
			getline(file, line);
		}
		getline(file, line);
	}
	num_NTermin=-value_nontermin;
	num_Termin=value_termin-1;
	return 0;
}











// 分析表：前面是GOTO部分，后面是ACTION部分，就用map_symbols的下标
// 加一个偏移量：num_NTermin-1
vector<vector<string>>LRTable(num_state,vector<string>(num_Termin+num_NTermin));

/**
 * 生成分析表
*/
void setLRTable(){
	// 先填移进，遍历所有状态，根据edgesMap填入即可
/*  */
	for(int i=0;i<num_state;i++){
		LRState&state=AllStates[i];
		for(auto edge:state.edgesMap){
			LRTable[i][edge.first+num_NTermin-1]="S"+to_string(edge.second);
		}
	}
}

int main()
{
	char *filename = "test.y";
	int status = readYaccFile(filename);
	createLR1DFA();
	setLRTable();
	return 0;
}