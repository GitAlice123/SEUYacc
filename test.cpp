#include<vector>
#include<iostream>
#include<fstream>
#include<stack>
using namespace std;

vector<vector<int>>ACTION_table;
vector<vector<int>>GOTO_table;
vector<pair<int, vector<int>>> AllProducers;
// 状态数
int num_state;
// 非终结符数
int num_NTermin;
// 终结符数
int num_Termin;

stack<int>state_stack;
stack<int>token_stack;

int main(int argc, char* argv[]){
    cout<<int('{')<<endl;
}
