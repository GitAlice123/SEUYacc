#include "test.tab.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <vector>
#include <map>
#include <string>
using namespace std;
vector<vector<int>>ACTION_table{
{-1,-1,-4,-1,-5,},
{-1,2,-1,-1,-1,},
{-1,5,-1,-6,-1,},
{-1,0,-1,-1,-1,},
{-1,4,-1,4,-1,},
{-1,-1,-4,-1,-5,},
{-1,-1,-11,-1,-12,},
{-1,3,-1,3,-1,},
{-1,5,-1,5,-1,},
{-1,1,-1,-1,-1,},
{-1,5,-1,-1,-1,},
{-1,4,-1,-1,-1,},
{-1,-1,-11,-1,-12,},
{-1,3,-1,-1,-1,},
};
vector<vector<int>>GOTO_table{
{1,2,3,-1,},
{-1,-1,-1,-1,},
{-1,-1,-1,-1,},
{-1,-1,-1,-1,},
{-1,-1,-1,-1,},
{7,8,-1,-1,},
{9,10,-1,-1,},
{-1,-1,-1,-1,},
{-1,-1,-1,-1,},
{-1,-1,-1,-1,},
{-1,-1,-1,-1,},
{-1,-1,-1,-1,},
{13,10,-1,-1,},
{-1,-1,-1,-1,},
};
map<int,string>map_id2token{
{0,"_S"},
{1,"#"},
{2,"ID"},
{3,"EQUALS"},
{4,"MUL"},
{5,""},
{6,""},
{7,""},
};
vector<pair<int,vector<int>>>AllProducers{
{0,{-1,}},
{-1,{-2,3,-3,}},
{-1,{-3,}},
{-2,{4,-3,}},
{-2,{2,}},
{-3,{-2,}},
};
int num_NTermin=4;
int num_Termin=4;
int num_state=14;
int yyparse(){
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
				int reduce = action;
				for(int i = 0; i < AllProducers[reduce].second.size(); i++)
				{
					state_stack.pop();
					int t=token_stack.top();
					token_stack.pop();
					if(t>0)
					{
						string Ter=Termin_stack.top();
						Termin_stack.pop();
						graphfile<<"node"<<node_num++<<"[label= \""<< Ter <<"\"];"<<endl;
						if(i==0){
							graphfile<<"node"<<node_num++<<"[label= \""<< map_id2token[AllProducers[reduce].first]<<"\"];"<<endl;
							cur_left=node_num-1;
							graphfile<<"node"<<cur_left<<" -> "<<"node"<<node_num-2<<";"<<endl;
						}
						else{
							graphfile<<"node"<<cur_left<<" -> "<<"node"<<node_num-1<<";"<<endl;
						}
					}
					else
					{
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
		graphfile << "}" << std::endl;
		graphfile.close();
		system("dot -Tpng tree.dot -o tree.png");
		system("xdg-open tree.png");
		return 0;
	}


int main(){
    yyparse();
}