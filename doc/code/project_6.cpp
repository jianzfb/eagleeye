#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/processnode/Placeholder.h"
#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/processnode/DebugNode.h"


namespace eagleeye{

/**
 * @brief register pipeline plugin
 * 
 */
EAGLEEYE_PIPELINE_REGISTER(TRACKING, 1.0.0.0, xxxx);

/**
 * @brief configure pipeline plugin
 * 
 */
EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(DynamicSky)
// 1.step build datasource node
// 获取从静态换天管线的RGB图像
AnyNode* rgb_input = DynamicSky->branch("StaticSky", "placeholder", 0);
// 获取从静态换天管线的分割节点天空MASK输出
AnyNode* sky_seg_node = DynamicSky->branch("StaticSky", "seg", 0);

// DebugNode是动态天空素材与前景融合节点的替代，仅用于验证框架流程，不完成实际功能
// 两个输入端口，一个输出端口
// 输入端口0：RGB图像；输入端口1：天空区域MASK；
// 输出端口0：融合后RGB图（ImageSignal<Array<unsigned char, 3>>类型信号）
DebugNode* merge_node = new DebugNode(3,1);
merge_node->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_RGB);

// 3.step add all node to pipeline
DynamicSky->add(rgb_input, "rgb");
DynamicSky->add(sky_seg_node, "seg");
DynamicSky->add(merge_node, "merge", SINK_NODE);

// 4.step link all node in pipeline
DynamicSky->bind("rgb", 0, "merge", 0);
DynamicSky->bind("seg", 0, "merge", 1);
EAGLEEYE_END_PIPELINE_INITIALIZE
}