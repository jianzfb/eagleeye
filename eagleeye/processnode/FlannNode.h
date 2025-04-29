#ifndef _EAGLEEYE_FLANNNODE_H_
#define _EAGLEEYE_FLANNNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>
#include <map>
#include <memory>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeMemory.h"
#include <mutex>


namespace eagleeye{
enum FlannMode{
    FLANN_INSERT    = 0,
    FLANN_QUERY     = 1,
    FLANN_REMOVE    = 2
};

class FlannNode:public AnyNode, DynamicNodeCreator<FlannNode>{
public:
    typedef FlannNode                   Self;
    typedef AnyNode                     Superclass;
    EAGLEEYE_CLASSIDENTITY(FlannNode);

    FlannNode();
    FlannNode(std::string collection, std::string root_folder, int top_k=1, bool use_norm=true);
    virtual ~FlannNode();

    void setMode(FlannMode mode){m_mode = mode;};

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    void setWritableFolder(const std::string writable_folder);
    void getWritableFolder(std::string& writable_folder);

private:
    FlannNode(const FlannNode&);
    void operator=(const FlannNode&);

    std::string m_collection;
    std::string m_root_folder;
    bool m_is_init;

    FlannMode m_mode;
    int m_top_k;
    std::shared_ptr<KVMemoryManage> m_memory;
    std::mutex m_mu;
    bool m_use_norm;
};
}
#endif