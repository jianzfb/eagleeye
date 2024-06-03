#ifndef _EAGLEEYE_CACHENODE_H_
#define _EAGLEEYE_CACHENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include "eagleeye/framework/pipeline/GroupSignal.h"
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>


namespace eagleeye{
class CacheNode:public AnyNode, DynamicNodeCreator<CacheNode>{
public:
    typedef CacheNode                   Self;
    typedef AnyNode                     Superclass;

    EAGLEEYE_CLASSIDENTITY(CacheNode);
    CacheNode(std::function<void(GroupSignal*)> post_process=nullptr);
    virtual ~CacheNode();

    virtual void executeNodeInfo();

    void setCacheSize(int size);
    int getCacheSize(int& size);

private:
    CacheNode(const CacheNode&);
    void operator=(const CacheNode&);
    std::vector<AnySignal*> m_cache_queue;
    int m_cache_size;
    int m_cache_i;

    std::function<void(GroupSignal*)> m_post_process;
};
}

#endif