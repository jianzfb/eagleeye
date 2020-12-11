#ifndef __EAGLEEYE_FACTORY_H_
#define __EAGLEEYE_FACTORY_H_
#include <string>
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/CJsonObject.hpp"


namespace eagleeye
{
/**
 * @brief create node handler from factory
 */     
AnyNode* eagleeye_node_factory(neb::CJsonObject config, std::function<AnyNode*()> option_func=std::function<AnyNode*()>());  

/**
 * @brief get all nodes config
 */ 
void eagleeye_get_nodes_config(neb::CJsonObject& config);
} // namespace eagleeye


#endif