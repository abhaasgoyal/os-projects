#include "fatsim.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <unordered_map>

typedef std::vector<bool> vec_bool;
typedef std::vector<long> vec_long;
typedef std::vector<vec_long> t_adj_list;

vec_long make_graph(vec_long fat, t_adj_list & graph)
{
  vec_long end_index_list;
  for (unsigned long i = 0; i < fat.size(); i++) {
    if (fat[i] == -1) {
      end_index_list.push_back(i);
    } else {
      graph[fat[i]].push_back(i);
    }
  }
  return end_index_list;
}

long DFSUtil(long u, t_adj_list & graph, vec_bool & visited, long height)
{
  visited[u] = true;
  long max_height = height;
  for (unsigned long i = 0; i < graph[u].size(); i++) {
    if (visited[graph[u][i]] == false) {
      long temp_height = DFSUtil(graph[u][i], graph, visited, height + 1);
      max_height = std::max(temp_height, max_height);
    }
  }
  return max_height;
}

vec_long DFS(t_adj_list & adj, long V, vec_long fat_enders)
{
  vec_long max_chains;
  for (auto u : fat_enders) {
    vec_bool visited(V, false);
    max_chains.push_back(DFSUtil(u, adj, visited, 0) + 1);
  }
  sort(max_chains.begin(), max_chains.end());
  return max_chains;
}

// reimplement this function
vec_long fat_check(const vec_long & fat)
{
  long n_files = fat.size();
  t_adj_list graph(n_files);
  //  vec_long graph[n_files];
  vec_long fat_enders = make_graph(fat, graph);
  return DFS(graph, n_files, fat_enders);
}
