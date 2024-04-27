// #pragma GCC optimize("O2")
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
#include <stack>
#include <iomanip>
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

// 存储后面的代码
string code;
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
 * 简化的状态存储：只存储没有内部拓展前的项目
*/
vector<set<LRItem>> SimpleStates;
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
 * 存从序号到token的反向映射
*/
map<int,string> map_id2token;

/**
 * 存算出来的first集
*/
vector<unordered_set<int>> first_record(num_NTermin+1);

/**
 * LRtable
*/
vector<vector<int>>ACTION_table(num_state,vector<int>(num_Termin+1,0));
vector<vector<int>>GOTO_table(num_state,vector<int>(num_NTermin,0));

/**
 * 下面是函数部分
*/

/**
 * compute first set of symbol X
 */
unordered_set<int> ComputerFirst(int X)
{
	// unordered_set<int> first_set;
	// if (X > 0)
	// {
	// 	// X is terminal
	// 	first_set.insert(X);
	// }
	// else
	// {
	// 	int total_pro_num = AllProducers.size();
	// 	bool flag = false;
	// 	int begin = left_producer_range[X].first;
	// 	int end = left_producer_range[X].second;
	// 	for (int i = begin; i <= end; i++)
	// 	{
	// 		if(AllProducers[i].second[0]==X) continue;
	// 		first_set.merge(ComputerFirst(AllProducers[i].second[0]));
	// 	}
	// }
	// return first_set;

	// 如果是空串，AllProducers[i].second是空的
	unordered_set<int> first_set;
	if (X > 0)
	{
		// X is terminal
		first_set.insert(X);
	}
	else
	{
		int total_pro_num = AllProducers.size();
		bool flag = false;
		// begin和end是X为左部的产生式的范围
		int begin = left_producer_range[X].first;
		int end = left_producer_range[X].second;
		for (int i = begin; i <= end; i++)
		{
			// 遍历产生式右部S->X1X2…Xn，对于不是最后一个符号的Xi求First(Xi)，如果其中包含空串，去掉空串，与原来的First(S)求并
			// 如果X1X2…Xn都能推出空串，空串也不要加入First(S)
			//当遍历到最后一个符号Xn时，如果其中包含空串，将空串加入First(S)
			for(int j=0;j<AllProducers[i].second.size();j++){
				// 如果AllProducers[i].second[j]还没有计算过first集，计算first集
				// 如果是终结符，直接加入
				// 如果右边第一个字符和左边相同，这个产生式不要算了
				if(AllProducers[i].second[j]==X){
					break;
				}
				if(AllProducers[i].second[j]>0){
					first_set.insert(AllProducers[i].second[j]);
					break;
				}
				if(first_record[AllProducers[i].second[j]+num_NTermin-1].size()==0){
					ComputerFirst(AllProducers[i].second[j]);
				}
				auto predict_set = first_record[AllProducers[i].second[j]+num_NTermin-1];
				if(predict_set.find(0)!=predict_set.end()){
					// 如果包含空串，去除空串，与原来的first_set求并
					predict_set.erase(0);
					first_set.merge(predict_set);
				}
				else{
					// 如果不包含空串，直接求并
					first_set.merge(predict_set);
					break;
				}
				if(j==AllProducers[i].second.size()-1){
					// 如果是最后一个符号，且包含空串，加入空串
					first_set.insert(0);
				}
			}
		}
	}
	// 存入first_record
	if(X<0)
		first_record[X+num_NTermin-1]=first_set;
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
					unordered_set<int> predict_set;
					if(test>0){
						// 如果是终结符，直接加入
						predict_set.insert(test);
					}else{
						// 如果是非终结符，加入first集
						if(first_record[test+num_NTermin-1].size()==0){
							// 如果first集为空，计算first集	
							ComputerFirst(test);
						}
						predict_set=first_record[test+num_NTermin-1];
					}
					// auto predict_set = ComputerFirst(test);
					// if the first set contains empty string
					if (predict_set.find(0) != predict_set.end())
					{
						predict_set.erase(0);
					}
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
	SimpleStates.push_back(zero_state.LRItemsSet);
	// Q记录待拓展的state_id
	queue<int> Q;
	Q.push(0);
	auto& test=AllStates[0];
	// ExpandStateInside(test);
	// 记录每个state当前暂时生成的所有新状态
	// 先存进来，但是有可能和之前的状态有重复，就不再增加新状态
	// temp_state_set_map:从当前这个状态会发出去的线上的字符->可能的新状态
	// 如果重复了，不增加状态但是要edgesMap里面增加连线
	map<int, LRState> temp_state_set_map;
	cout << "current state: " << endl;
	// 总共是1854个状态，进度条用来显示当前进度
	// 用s/1854表示当前进度
	int total_state = 1854;
	while (!Q.empty())
	{
		auto cur_state_id = Q.front();
		// 注意要在同一个位置输出，所以不要换行，用\r回到行首
		cout << cur_state_id << "/" << total_state << "\r";
		ExpandStateInside(AllStates[cur_state_id]);
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
			{
				// 填规约表
				if(ACTION_table.size()<cur_state_id+1){
					int ori_size=ACTION_table.size();
					ACTION_table.resize(cur_state_id+1);
					for(int i=ori_size;i<cur_state_id+1;i++){
						// 为每个state的ACTION_table分配空间，同时初始化为-1
						// ACTION_table[i].resize(num_Termin+1);
						for(int j=0;j<num_Termin+1;j++){
							ACTION_table[i].push_back(-1);
						}
					}
				}
				ACTION_table[cur_state_id][r.predictiveSymbol]=r.producerID;
				continue;
			}
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
			// ExpandStateInside(temp_state.second);
			ok = true;
			// 遍历所有已有的状态
			for (int i = 0; i < AllStates.size(); i++)
			{
				// if (AllStates[i].LRItemsSet == temp_state.second.LRItemsSet)
				// {
				// 	ok = false;
				// 	// 加入edgesMap
				// 	AllStates[cur_state_id].edgesMap.insert(pair<int, int>(temp_state.first, i));
				// 	break;
				// }
				if (SimpleStates[i] == temp_state.second.LRItemsSet)
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
				SimpleStates.push_back(temp_state.second.LRItemsSet);
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
	map_id2token[0]="_S";
	map_id2token[1]="#";
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
			map_id2token[value_termin]=token;
			map_symbols[token] = value_termin++;
		}
	}

	/**
	 * 读取start_sym
	 */
	map_symbols[line.substr(7)] = value_nontermin;
	map_id2token[value_nontermin]=line.substr(7);
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
		if (map_symbols.find(left_str) == map_symbols.end()){
			// 注册左符
			map_id2token[value_nontermin]=left_str;
			map_symbols[left_str] = value_nontermin--;
		}
			
		// 循环读取这一组产生式
		// 这一组产生式从这个下标开始存
		int cur_producer_idx = AllProducers.size();
		left_producer_range[map_symbols[left_str]].first = cur_producer_idx;
		getline(file, line);
		while (line.find(';')==string::npos)
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
						map_id2token[value_termin]=word;
						map_symbols[word] = value_termin++;
					}
					else
					{
						// 是非终结符
						map_id2token[value_nontermin]=word;
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

	// 读取代码部分
	
	while (getline(file, line))
	{
		code += line + "\n";
	}
	return 0;
}











// 加一个偏移量：num_NTermin-1

/**
 * 生成分析表
*/
void setLRTable(){
	// 先填移进，遍历所有状态，根据edgesMap填入
	for(int i=0;i<num_state;i++){
		LRState&state=AllStates[i];
		for(auto edge:state.edgesMap){
			if(edge.first<=0){
				// 线上为非终结符,填GOTO
				GOTO_table[i][edge.first+num_NTermin-1]=edge.second;
			}else{
				// 线上为终结符，填ACTION
				// TODO:修改文法使其不具备二义性
				// 移进取负，规约取正
				// if(ACTION_table[i][edge.first]>0){
				// 	cerr << "移进-规约冲突！" << endl;
				// 	cerr << "规约产生式编号为："<<ACTION_table[i][edge.first]<<endl;
				// 	cerr << "移进面临的符号编号为："<<edge.first<<endl;
        		// 	exit(EXIT_FAILURE);
				// }
				ACTION_table[i][edge.first]=-edge.second;
			}
		}
	}
	
}

/**
 * 上面生成的是cpp文件，实际上应该生成.c文件
 * 并且要生成yyparse函数，把code部分放进去
 * 注意C里面没有vector,stack,ifstream,ofstream
 * 但是有malloc和free，所以有些数据结构要自己实现
*/


/**
 * int yyparseTest(){
	ofstream graphfile("tree.dot");
	if (graphfile.is_open()) {
        graphfile << "digraph Tree {" << std::endl;
    } else {
        std::cerr << "Unable to open file" << std::endl;
        return 1;
    }

	ifstream fin("token.txt");
	if(!fin)
	{
		cout << "Cannot open file " << "token.txt" << endl;
		return 1;
	}
	stack<int>state_stack;
	stack<int>token_stack;
	stack<int>node_stack;
	stack<string>Termin_stack;
	state_stack.push(0);
	token_stack.push(1);

	string token;
	getline(fin, token);
	int token_num = stoi(token.substr(0, token.find(',')));
	int node_num=0;
	int cur_left,childNTnum;
	while(1)
	{
		int state = state_stack.top();
		int action = ACTION_table[state][token_num];
		if(action == -1)
		{
			cout << "Syntax error" << endl;
			return 0;
		}
		if(action == 0)
		{
			cout << "Accept" << endl;
			return 0;
		}
		if(action < 0)
		{
			state_stack.push(-action);
			token_stack.push(token_num);
			Termin_stack.push(token.substr(token.find(',')+1));
			getline(fin, token);

			if(token==""){
				token_num=1;
			}
			else{
				token_num = stoi(token.substr(0, token.find(',')));
			}
		}
		else
		{
			// 规约
			int reduce = action;
			for(int i = 0; i < AllProducers[reduce].second.size(); i++)
			{
				state_stack.pop();
				int t=token_stack.top();
				token_stack.pop();
				if(t>0){
					// 终结符
					string Ter=Termin_stack.top();
					Termin_stack.pop();
					graphfile<<"node"<<node_num++<<"[label= \""<< Ter <<"\"];"<<endl;
					if(i==0){
						graphfile<<"node"<<node_num++<<"[label= \""<< map_id2token[AllProducers[reduce].first]<<"\"];"<<endl;
						cur_left=node_num-1;
						graphfile<<"node"<<cur_left<<" -> "<<"node"<<node_num-2<<";"<<endl;
					}else{
						graphfile<<"node"<<cur_left<<" -> "<<"node"<<node_num-1<<";"<<endl;
					}
				}
				else{
					childNTnum=node_stack.top();
					node_stack.pop();
					if(i==0){
						graphfile<<"node"<<node_num++<<"[label= \""<< map_id2token[AllProducers[reduce].first]<<"\"];"<<endl;
						cur_left=node_num-1;
					}
					graphfile<<"node"<<cur_left<<" -> "<<"node"<<childNTnum<<";"<<endl;
				}
				if(i==AllProducers[reduce].second.size()-1){
					node_stack.push(cur_left);
				}
			}
			int goto_state = GOTO_table[state_stack.top()][AllProducers[reduce].first+num_NTermin-1];
			state_stack.push(goto_state);
			token_stack.push(AllProducers[reduce].first);
		}
	}
	graphfile << "}" << endl;
	graphfile.close();
	system("dot -Tpng tree.dot -o tree.png");
	system("xdg-open tree.png");
	return 0;
}
把上面的函数写入test.tab.cpp
在这个cpp文件里面，首先要包含test.tab.h，然后全局变量要有ACTION_table、GO_TO_table、
map_id2token、AllProducers、num_NTermin、num_Termin
这些全局变量都要在全局区初始化，初始化的值从这个文件里面读取
*/
void GenerateTabCpp(){
	ofstream fout("test.tab.cpp");
	fout<<"#include \"test.tab.h\""<<endl;
	fout<<"#include <iostream>"<<endl;
	fout<<"#include <fstream>"<<endl;
	fout<<"#include <sstream>"<<endl;
	fout<<"#include <stack>"<<endl;
	fout<<"#include <vector>"<<endl;
	fout<<"#include <map>"<<endl;
	fout<<"#include <string>"<<endl;

	fout<<"using namespace std;"<<endl;
	fout<<"vector<vector<int>>ACTION_table{"<<endl;
	for(int i=0;i<num_state;i++){
		fout<<"{";
		for(int j=0;j<num_Termin+1;j++){
			fout<<ACTION_table[i][j]<<",";
		}
		fout<<"},"<<endl;
	}
	fout<<"};"<<endl;
	fout<<"vector<vector<int>>GOTO_table{"<<endl;
	for(int i=0;i<num_state;i++){
		fout<<"{";
		for(int j=0;j<num_NTermin;j++){
			fout<<GOTO_table[i][j]<<",";
		}
		fout<<"},"<<endl;
	}
	fout<<"};"<<endl;
	fout<<"map<int,string>map_id2token{"<<endl;
	for(int i=0;i<num_NTermin+num_Termin;i++){
		fout<<"{"<<i<<",\""<<map_id2token[i]<<"\"},"<<endl;
	}
	fout<<"};"<<endl;
	fout<<"vector<pair<int,vector<int>>>AllProducers{"<<endl;
	for(int i=0;i<AllProducers.size();i++){
		fout<<"{"<<AllProducers[i].first<<",{";
		for(int j=0;j<AllProducers[i].second.size();j++){
			fout<<AllProducers[i].second[j]<<",";
		}
		fout<<"}},"<<endl;
	}
	fout<<"};"<<endl;
	fout<<"int num_NTermin="<<num_NTermin<<";"<<endl;
	fout<<"int num_Termin="<<num_Termin<<";"<<endl;
	fout<<"int num_state="<<num_state<<";"<<endl;
	
	// 实现yyparse函数，把上面的yyparseTest函数放进去
	fout<<"int yyparse(){"<<endl;
	fout<<"		ofstream graphfile(\"tree.dot\");"<<endl;
	fout<<"		if (graphfile.is_open()) {"<<endl;
	fout<<"			graphfile << \"digraph Tree {\" << std::endl;"<<endl;
	fout<<"		} else {"<<endl;
	fout<<"			std::cerr << \"Unable to open file\" << std::endl;"<<endl;
	fout<<"			return 1;"<<endl;
	fout<<"		}"<<endl;

	fout<<"		ifstream fin(\"token.txt\");"<<endl;
	fout<<"		if(!fin)"<<endl;
	fout<<"		{"<<endl;
	fout<<"			cout << \"Cannot open file \" << \"token.txt\" << endl;"<<endl;
	fout<<"			return 1;"<<endl;
	fout<<"		}"<<endl;
	fout<<"		stack<int>state_stack;"<<endl;
	fout<<"		stack<int>token_stack;"<<endl;
	fout<<"		stack<int>node_stack;"<<endl;
	fout<<"		stack<string>Termin_stack;"<<endl;
	fout<<"		state_stack.push(0);"<<endl;
	fout<<"		token_stack.push(1);"<<endl;

	fout<<"		string token;"<<endl;
	fout<<"		getline(fin, token);"<<endl;
	fout<<"		int token_num = stoi(token.substr(0, token.find(',')));"<<endl;
	fout<<"		int node_num=0;"<<endl;
	fout<<"		int cur_left,childNTnum;"<<endl;
	fout<<"		while(1)"<<endl;
	fout<<"		{"<<endl;
	fout<<"			int state = state_stack.top();"<<endl;
	fout<<"			int action = ACTION_table[state][token_num];"<<endl;
	fout<<"			if(action == -1)"<<endl;
	fout<<"			{"<<endl;
	fout<<"				cout << \"Syntax error\" << endl;"<<endl;
	fout<<"				return 0;"<<endl;
	fout<<"			}"<<endl;
	fout<<"			if(action == 0)"<<endl;
	fout<<"			{"<<endl;
	fout<<"				cout << \"Accept\" << endl;"<<endl;
	fout<<"				return 0;"<<endl;
	fout<<"			}"<<endl;
	fout<<"			if(action < 0)"<<endl;
	fout<<"			{"<<endl;
	fout<<"				state_stack.push(-action);"<<endl;
	fout<<"				token_stack.push(token_num);"<<endl;
	fout<<"				Termin_stack.push(token.substr(token.find(',')+1));"<<endl;
	fout<<"				getline(fin, token);"<<endl;
	fout<<"				if(token==\"\"){"<<endl;
	fout<<"					token_num=1;"<<endl;
	fout<<"				}"<<endl;
	fout<<"				else{"<<endl;
	fout<<"					token_num = stoi(token.substr(0, token.find(',')));"<<endl;
	fout<<"				}"<<endl;
	fout<<"			}"<<endl;
	fout<<"			else"<<endl;
	fout<<"			{"<<endl;
	fout<<"				int reduce = action;"<<endl;
	fout<<"				for(int i = 0; i < AllProducers[reduce].second.size(); i++)"<<endl;
	fout<<"				{"<<endl;
	fout<<"					state_stack.pop();"<<endl;
	fout<<"					int t=token_stack.top();"<<endl;
	fout<<"					token_stack.pop();"<<endl;
	fout<<"					if(t>0)"<<endl;
	fout<<"					{"<<endl;
	fout<<"						string Ter=Termin_stack.top();"<<endl;
	fout<<"						Termin_stack.pop();"<<endl;
	fout<<"						graphfile<<\"node\"<<node_num++<<\"[label= \\\"\"<< Ter <<\"\\\"];\"<<endl;"<<endl;
	fout<<"						if(i==0){"<<endl;
	fout<<"							graphfile<<\"node\"<<node_num++<<\"[label= \\\"\"<< map_id2token[AllProducers[reduce].first]<<\"\\\"];\"<<endl;"<<endl;
	fout<<"							cur_left=node_num-1;"<<endl;
	fout<<"							graphfile<<\"node\"<<cur_left<<\" -> \"<<\"node\"<<node_num-2<<\";\"<<endl;"<<endl;
	fout<<"						}"<<endl;
	fout<<"						else{"<<endl;
	fout<<"							graphfile<<\"node\"<<cur_left<<\" -> \"<<\"node\"<<node_num-1<<\";\"<<endl;"<<endl;
	fout<<"						}"<<endl;
	fout<<"					}"<<endl;
	fout<<"					else"<<endl;
	fout<<"					{"<<endl;
	fout<<"						childNTnum=node_stack.top();"<<endl;
	fout<<"						node_stack.pop();"<<endl;
	fout<<"					if(i==0){"<<endl;
	fout<<"						graphfile<<\"node\"<<node_num++<<\"[label= \\\"\"<< map_id2token[AllProducers[reduce].first]<<\"\\\"];\"<<endl;"<<endl;
	fout<<"						cur_left=node_num-1;"<<endl;
	fout<<"					}"<<endl;
	fout<<"					graphfile<<\"node\"<<cur_left<<\" -> \"<<\"node\"<<childNTnum<<\";\"<<endl;"<<endl;
	fout<<"				}"<<endl;
	fout<<"				if(i==AllProducers[reduce].second.size()-1){"<<endl;
	fout<<"					node_stack.push(cur_left);"<<endl;
	fout<<"				}"<<endl;
	fout<<"			}"<<endl;
	fout<<"			int goto_state = GOTO_table[state_stack.top()][AllProducers[reduce].first+num_NTermin-1];"<<endl;
	fout<<"			state_stack.push(goto_state);"<<endl;
	fout<<"			token_stack.push(AllProducers[reduce].first);"<<endl;
	fout<<"			}"<<endl;
	fout<<"		}"<<endl;
	fout<<"		graphfile << \"}\" << std::endl;"<<endl;
	fout<<"		graphfile.close();"<<endl;
	fout<<"		system(\"dot -Tpng tree.dot -o tree.png\");"<<endl;
	fout<<"		system(\"xdg-open tree.png\");"<<endl;
	fout<<"		return 0;"<<endl;
	fout<<"	}"<<endl;
	fout.close();


}


/**
 * 生成tab.h,里面声明了所有的token
	但是注意，我的map_symbols里面，正数是终结符，负数是非终结符
	token只需要终结符，所以只需要输出正数
  };
*/
void generateTabH(){
	ofstream fout("test.tab.h");
	for(auto sym:map_symbols){
		if(sym.second>0&&sym.second!=1){
			fout<<"#define "<<sym.first<<" "<<sym.second<<endl;
		}
	}
	fout<<endl;
	fout<<"char yytext[256];"<<endl;
	fout<<"int yytextlen = 0;"<<endl;
	fout.close();
}


/**
 * 测试yyparse函数，中间要生成语法树，用graphviz画出来
*/

int yyparseTest(){
	ofstream graphfile("tree.dot");
	if (graphfile.is_open()) {
        graphfile << "digraph Tree {" << std::endl;
    } else {
        std::cerr << "Unable to open file" << std::endl;
        return 1;
    }

	ifstream fin("token.txt");
	if(!fin)
	{
		cout << "Cannot open file " << "token.txt" << endl;
		return 1;
	}
	stack<int>state_stack;
	stack<int>token_stack;
	stack<int>node_stack;
	stack<string>Termin_stack;
	state_stack.push(0);
	token_stack.push(1);

	string token;
	getline(fin, token);
	int token_num = stoi(token.substr(0, token.find(',')));
	int node_num=0;
	int cur_left,childNTnum;
	while(1)
	{
		int state = state_stack.top();
		int action = ACTION_table[state][token_num];
		if(action == -1)
		{
			cout << "Syntax error" << endl;
			return 0;
		}
		if(action == 0)
		{
			cout << "Accept" << endl;
			return 0;
		}
		if(action < 0)
		{
			state_stack.push(-action);
			token_stack.push(token_num);
			Termin_stack.push(token.substr(token.find(',')+1));
			getline(fin, token);

			if(token==""){
				token_num=1;
			}
			else{
				token_num = stoi(token.substr(0, token.find(',')));
			}
		}
		else
		{
			// 规约
			int reduce = action;
			for(int i = 0; i < AllProducers[reduce].second.size(); i++)
			{
				state_stack.pop();
				int t=token_stack.top();
				token_stack.pop();
				if(t>0){
					// 终结符
					string Ter=Termin_stack.top();
					Termin_stack.pop();
					graphfile<<"node"<<node_num++<<"[label= \""<< Ter <<"\"];"<<endl;
					if(i==0){
						graphfile<<"node"<<node_num++<<"[label= \""<< map_id2token[AllProducers[reduce].first]<<"\"];"<<endl;
						cur_left=node_num-1;
						graphfile<<"node"<<cur_left<<" -> "<<"node"<<node_num-2<<";"<<endl;
					}else{
						graphfile<<"node"<<cur_left<<" -> "<<"node"<<node_num-1<<";"<<endl;
					}
				}
				else{
					childNTnum=node_stack.top();
					node_stack.pop();
					if(i==0){
						graphfile<<"node"<<node_num++<<"[label= \""<< map_id2token[AllProducers[reduce].first]<<"\"];"<<endl;
						cur_left=node_num-1;
					}
					graphfile<<"node"<<cur_left<<" -> "<<"node"<<childNTnum<<";"<<endl;
				}
				if(i==AllProducers[reduce].second.size()-1){
					node_stack.push(cur_left);
				}
			}
			int goto_state = GOTO_table[state_stack.top()][AllProducers[reduce].first+num_NTermin-1];
			state_stack.push(goto_state);
			token_stack.push(AllProducers[reduce].first);
		}
	}
	graphfile << "}" << endl;
	graphfile.close();
	system("dot -Tpng tree.dot -o tree.png");
	system("xdg-open tree.png");
	return 0;
}


int main()
{
	char *filename = "test.y";
	int status = readYaccFile(filename);
	first_record.resize(num_NTermin+1);
	generateTabH();
	createLR1DFA();
	ACTION_table.resize(num_state);
	GOTO_table.resize(num_state);
	// resize,顺便初始化ACTION_table和GOTO_table为全-1
	for(int i=0;i<num_state;i++){
		for(int j=0;j<num_Termin+1;j++){
			ACTION_table[i].push_back(-1);
		}
		for(int j=0;j<num_NTermin;j++){
			GOTO_table[i].push_back(-1);
		}
	}
	setLRTable();
	GenerateTabCpp();

	yyparseTest();

	return 0;
}