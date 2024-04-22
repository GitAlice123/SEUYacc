#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <string>
#include <bitset>
#include <iomanip>
#include <numeric>
#include <complex>
#include <sstream>
#include <unordered_map>

#define ll long long
#define ull unsigned long long
#define eb emplace_back
using namespace std;
constexpr int N = 1e3 + 3;
constexpr double eps = 1e-10;
constexpr int INF = 0x3f3f3f3f;

int minimumOperations(vector<vector<int>> &grid)
{
    // 下面的要相等，右边的要不相等
    int num_row=grid.size();
    int num_col=grid[0].size();
    // 对每一列开一个PQ，存<出现次数，数>
    int temp_arr[10];
    priority_queue<pair<int,int>> PQ_arr[num_col];
    for(int j=0;j<num_col;j++){
        memset(temp_arr,0,sizeof(temp_arr));
        for(int i=0;i<num_row;i++){
            temp_arr[grid[i][j]]++;
        }
        for(int i=0;i<10;i++){
            PQ_arr[j].push(pair<int,int>(temp_arr[i],i));
        }
    }
    // BFS，找所有PQ中出现次数最多的那个下标
    

}

int main()
{
}
