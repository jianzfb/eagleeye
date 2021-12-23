#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/processnode/Placeholder.h"
#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/processnode/DebugNode.h"
#include "eagleeye/processnode/AsynNode.h"

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
EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(TRACKING)
// 1.step build datasource node
Placeholder<ImageSignal<Array<unsigned char, 3>>>* frame_placeholder = new Placeholder<ImageSignal<Array<unsigned char, 3>>>();
Placeholder<ImageSignal<int>>* rect_placeholder = new Placeholder<ImageSignal<int>>();

// 2.step build your algorithm node
AsynNode* asyn_node = new AsynNode(1,[](){
    // DebugNode是检测及REID节点的替代，仅用于验证框架流程，不完成实际功能
    // 二个输入端口，一个输出端口
    // 输入端口0：RGB图像；输入端口1：用户指定目标初始区域
    // 输出端口0：目标精确定位区域（ImageSignal<float>类型信号）
    DebugNode* det_node = new DebugNode(2, 1);
    det_node->setPortCategoryType(1, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);

    return det_node;
},1,1);

// DebugNode是KCF目标跟踪节点的替代，仅用于验证框架流程，不完成实际功能
// 三个输入端口，一个输出端口
// 输入端口0：RGB图像；输入端口1：用户指定目标初始区域；输入端口2：来自重绑定精确区域（可能为空）
// 输出端口0：目标精确定位区域（ImageSignal<float>类型信号）
DebugNode* tracking_node = new DebugNode(3,1);
tracking_node->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);

// 3.step add all node to pipeline
TRACKING->add(frame_placeholder, "frame", SOURCE_NODE);
TRACKING->add(rect_placeholder, "rect", SOURCE_NODE);
TRACKING->add(asyn_node, "detreid");
TRACKING->add(tracking_node, "track", SINK_NODE);

// 4.step link all node in pipeline
TRACKING->bind("frame", 0, "detreid", 0);
TRACKING->bind("rect", 0, "detreid", 1);
TRACKING->bind("frame",0,"track",0);
TRACKING->bind("rect", 0, "track", 1);
TRACKING->bind("detreid", 0, "track", 2);
EAGLEEYE_END_PIPELINE_INITIALIZE
}