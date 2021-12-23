#ifndef _EAGLEEYE_NODE_MANAGER_H_
#define _EAGLEEYE_NODE_MANAGER_H_
#include <string>
#include <memory>
#include <map>
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/CJsonObject.hpp"


namespace eagleeye
{
typedef AnyNode* (*BUILD_NODE_FUNC_TYPE)(const char*, neb::CJsonObject);

class NodeManager
{
public:
    static std::shared_ptr<NodeManager> get();

    /**
     * @brief 更新节点生成库
     */ 
    void update();

    /**
     * @brief 创建节点
     */ 
    AnyNode* build(std::string node, neb::CJsonObject node_param);


    virtual ~NodeManager();

private:
    NodeManager();

    static std::shared_ptr<NodeManager> m_p;
    std::map<std::string, BUILD_NODE_FUNC_TYPE> m_node_build_func_map;
    std::map<std::string, void*> m_so_map;
};


} // namespace eagleeye
#endif