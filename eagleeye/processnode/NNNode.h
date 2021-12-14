#ifndef _EAGLEEYE_NNNODE_H_
#define _EAGLEEYE_NNNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"
// #include "eagleeye/engine/nano/op/placeholder.h"
// #include "eagleeye/engine/nano/dataflow/node_impl_asyn.hpp"
// #include "eagleeye/engine/nano/dataflow/asyn.hpp"
#include "eagleeye/engine/nano/dataflow/graph.hpp"
#include "eagleeye/common/CJsonObject.hpp"
#include<functional>


namespace eagleeye{

typedef std::function<bool(dataflow::Graph* g, std::string op_name, std::string op_cls, neb::CJsonObject op_config, dataflow::Node*&, std::string resource_folder)> OpBFuncType;
class NNNode:public AnyNode{
public:    
    NNNode();
    virtual ~NNNode();

    /**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(NNNode);

    /**
	 * @brief execute control
	 * 
	 */
	virtual void executeNodeInfo();

    /**
     * @brief 加载算子结构
     * 
     * @param nn_info 
     * @return true 
     * @return false 
     */
    bool load(neb::CJsonObject nn_info, OpBFuncType op_builder, std::string resource_folder);

    /**
     * @brief Set the Output Signal Type object
     * 
     * @param port 
     * @param type 
     */
    void makeOutputSignal(int port, std::string type_str);

private:
    NNNode(const NNNode&);
    void operator=(const NNNode&);

    dataflow::Graph* m_g;
    std::map<int, std::string> m_input_map;
    std::map<int, std::string> m_output_map;
};
}
#endif