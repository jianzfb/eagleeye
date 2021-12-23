#ifndef _EAGLEEYE_GRAPH_H_
#define _EAGLEEYE_GRAPH_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <string>
#include <vector>
#include <map>
namespace eagleeye{
/**
 * @brief 对依赖关系进行拓扑排序
 */    
std::vector<std::string> eagleeye_topology_sort(std::map<std::string, std::vector<std::string>>& dependent_nodes);
}
#endif