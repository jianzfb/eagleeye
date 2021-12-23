#ifndef _EAGLEEYE_DATAFLOW_FACTORY_
#define _EAGLEEYE_DATAFLOW_FACTORY_
#include "eagleeye/common/CJsonObject.hpp"
#include "eagleeye/engine/nano/dataflow/node.hpp"
#include "eagleeye/engine/nano/dataflow/graph.hpp"
#include <string>

namespace eagleeye{
namespace dataflow{
class Graph;
Node* build_node_op(Graph* g, std::string op_name, std::string op_cls, neb::CJsonObject op_config);
}   // namespace dataflow
} // namespace eagleeye

#endif