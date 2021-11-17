#include "eagleeye/common/EagleeyeNodeManager.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <vector>
#include <dlfcn.h>
#include <dirent.h>


namespace eagleeye
{
std::shared_ptr<NodeManager> NodeManager::m_p;
NodeManager::NodeManager(){
}

NodeManager::~NodeManager(){
    std::map<std::string, void*>::iterator iter, iend(m_so_map.end());
    for(iter = m_so_map.begin(); iter!=iend; ++iter){
        dlclose(iter->second);
    }
}

static void empty_dladdr()
{
    //
}

void NodeManager::update(){
    // 发现当前so所在目录
    Dl_info dl_info;
    dladdr((void*)empty_dladdr, &dl_info);
    std::string so_path = dl_info.dli_fname;

    std::string separator = "/";
    std::vector<std::string> terms = split(so_path, separator);
    terms[terms.size() - 1] = "libfeatureext.so";
    std::string libfeatureext_path = pathJoin(terms);
    EAGLEEYE_LOGD("Load featureext %s", libfeatureext_path.c_str());

    // 发现节点生成动态库
    void* handle = dlopen(libfeatureext_path.c_str(), RTLD_LAZY);
    BUILD_NODE_FUNC_TYPE build_node_func = (BUILD_NODE_FUNC_TYPE)dlsym(handle, "eagleeye_build_node");
    if(build_node_func == NULL){
        EAGLEEYE_LOGE("Couldnt find node build func");
    }

    m_so_map["featureext"] = handle;
    m_node_build_func_map["featureext"] = build_node_func;
}

AnyNode* NodeManager::build(std::string node){
    std::map<std::string, BUILD_NODE_FUNC_TYPE>::iterator iter, iend(m_node_build_func_map.end());
    for(iter = m_node_build_func_map.begin(); iter != iend; ++iter){
        AnyNode* node_ptr = m_node_build_func_map[iter->first](node.c_str());
        if(node_ptr != NULL){
            return node_ptr;
        }
    }

    EAGLEEYE_LOGE("Couldnt build node %s.", node.c_str());
    return NULL;
}

std::shared_ptr<NodeManager> NodeManager::get(){
    if(NodeManager::m_p.get() == NULL){
        NodeManager::m_p = std::shared_ptr<NodeManager>(new NodeManager());
        m_p->update();
    }

    return NodeManager::m_p;
}
} // namespace eagleeye
