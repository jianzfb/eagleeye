#include "eagleeye/algorithm/algorithm.h"
#include "eagleeye/algorithm/hungarian.h"
#include <vector>

// struct WeightedBipartiteEdge {
//     const int left;
//     const int right;
//     const float cost;

//     WeightedBipartiteEdge() : left(), right(), cost() {}
//     WeightedBipartiteEdge(const int left, const int right, const float cost) : left(left), right(right), cost(cost) {}
// };


// const std::pair<float, std::vector<int> > _bruteForceInternal(const int n, 
// 															  const std::vector<WeightedBipartiteEdge> edges, 
// 															  std::vector<bool>& left_matched, 
// 															  std::vector<bool>& right_matched, 
// 															  const int edge_upto=0, 
// 															  const int match_count=0){
// 	if (match_count == n) {
// 		return std::make_pair(0.0f, std::vector<int>());
// 	}

// 	float best_cost = float(1 << 20);
// 	std::vector<int> best_edges;
// 	for (int edge_index = edge_upto; edge_index < edges.size(); ++edge_index){
// 		const WeightedBipartiteEdge& edge = edges[edge_index];
// 		if (!left_matched[edge.left] && !right_matched[edge.right]){
// 			left_matched[edge.left] = true;
// 			right_matched[edge.right] = true;
// 			std::pair<float, std::vector<int>> remainder = _bruteForceInternal(n, edges, left_matched, right_matched, edge_index + 1, match_count + 1);
// 			left_matched[edge.left] = false;
// 			right_matched[edge.right] = false;

// 			if (remainder.first + edge.cost < best_cost) {
// 				best_cost = remainder.first + edge.cost;
// 				best_edges = remainder.second;
// 				best_edges.push_back(edge_index);
// 			}
// 		}
// 	}

// 	return std::make_pair(best_cost, best_edges);
// }

// const std::vector<int> _bipartiteBruteForce(const int m, 
// 											const int n, 
// 											const std::vector<WeightedBipartiteEdge> edges, 
// 											std::vector<float>& costs) {
// 	std::vector<bool> left_matched(m, false), right_matched(n, false);
// 	std::vector<int> edge_indices = ::_bruteForceInternal(m<n?m:n, edges, left_matched, right_matched).second;
// 	std::vector<int> matching(m, -1);
// 	costs.resize(m);
// 	for (int i = 0; i < edge_indices.size(); ++i) {
// 		const WeightedBipartiteEdge& edge = edges[edge_indices[i]];
// 		matching[edge.left] = edge.right;
// 		costs[edge.left] = edge.cost;
// 	}
// 	return matching;
// }

namespace eagleeye{
// void bipartiteBruteForce(const Matrix<float> weight_mat, Matrix<float>& bind_map){
// 	int workers_n = weight_mat.rows();
// 	int tasks_n = weight_mat.cols();

//     std::vector<WeightedBipartiteEdge> edges;
//     for(int i=0; i<workers_n; ++i){
//     	for(int j=0; j<tasks_n; ++j){
//     		edges.push_back(WeightedBipartiteEdge(i,j,weight_mat.at(i,j)));
//     	}
//     }

//     std::vector<float> costs;
//     std::vector<int> matching = _bipartiteBruteForce(workers_n, tasks_n, edges, costs);
//     bind_map = Matrix<float>(workers_n, 3);
//     for(int i=0; i<workers_n; ++i){
//     	bind_map.at(i,0) = i;
//     	bind_map.at(i,1) = matching[i];
//     	if(matching[i] >= 0){
// 	    	bind_map.at(i,2) = costs[i];
//     	}
// 	    else{
// 	    	bind_map.at(i,2) = 1<<20;
// 	    }
//     }
// }

Algorithm::Algorithm(){

}
Algorithm::~Algorithm(){

}

}

