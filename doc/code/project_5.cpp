#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/processnode/Placeholder.h"
#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/processnode/DebugNode.h"
#include "eagleeye/processnode/IfElseNode.h"
#include "eagleeye/processnode/State2BooleanNode.h"
#include "eagleeye/processnode/LogicalNode.h"
#include "eagleeye/processnode/AsynNode.h"
#include "eagleeye/common/EagleeyeVisTool.h"
#include "AF_plugin.h"

namespace eagleeye{

/**
 * @brief register pipeline plugin
 * 
 */
EAGLEEYE_PIPELINE_REGISTER(AF, 1.0.0.0, xxxx);

/**
 * @brief configure pipeline plugin
 * 
 */
EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(AF)
// 1.step build datasource node
Placeholder<ImageSignal<Array<unsigned char, 3>>>* placeholder = new Placeholder<ImageSignal<Array<unsigned char, 3>>>();

// 2.step build your algorithm node
// 检测以及发现主体跟踪目标 （子管道）
AsynNode* asyn_node = new AsynNode(1,[](){
    SubPipeline* sub_pipeline = new SubPipeline();
    // DebugNode是目标检测节点的替代，仅用于验证框架流程，不完成实际功能
    // 一个输入端口，二个输出端口
    // 输入端口0：RGB图像；
    // 输出端口0：目标精确定位区域（ImageSignal<float>类型信号）；输出端口1：对应区域特征描述（ImageSignal<float>类型信号）
    DebugNode* det_node = new DebugNode(1, 2);
    det_node->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);            // 目标框
    det_node->setPortCategoryType(1, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);            // 特征描述

    // DebugNode是发现主目标&主目标重绑定节点的替代，仅用于验证框架流程，不完成实际功能
    // 三个输入端口，二个输出端口
    // 输入端口0：跟踪状态（yes-未跟丢；no-跟丢）；输入端口1：检测获得的所有区域；输入端口2：检测区域对应的特征描述
    // 输出端口0：主体目标区域（ImageSignal<float>类型信号）；输出端口1：是否发现主体目标/是否发生重新绑定（BooleanSignal类型信号）
    DebugNode* finding_or_rebinding_main_obj = new DebugNode(3,2);
    finding_or_rebinding_main_obj->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);    // 目标框
    finding_or_rebinding_main_obj->setPortCategoryType(1, SIGNAL_CATEGORY_CONTROL);                  // 是否发现主体目标或是否发生重新绑定（0-未发现，1-发现）

    sub_pipeline->add(det_node, "objdet", SOURCE_NODE);
    sub_pipeline->add(finding_or_rebinding_main_obj, "finding_or_rebinding", SINK_NODE);

    // objdet port 0: box
    // objdet port 1: feature
    sub_pipeline->bind("objdet",0,"finding_or_rebinding",1);
    sub_pipeline->bind("objdet",1,"finding_or_rebinding",2);
    sub_pipeline->bind("UPPER", 0, "objdet", 0);
    sub_pipeline->bind("UPPER", 1, "finding_or_rebinding", 0);

    return sub_pipeline;
},1,1);


// tracking or do nothing
// DebugNode是目标跟踪节点的替代，仅用于验证框架流程，不完成实际功能
// 二个输入端口，二个输出端口
// 输入端口0：RGB图像；输入端口1：主体目标区域
// 输出端口0：目标精确定位区域（ImageSignal<float>类型信号）；输出端口1：跟踪状态（StateSignal类型信号）
DebugNode* tracking_node = new DebugNode(2,2);
tracking_node->setUnitName("tracking");
tracking_node->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);           // 跟踪目标框
tracking_node->setPortCategoryType(1, SIGNAL_CATEGORY_STATE);                           // 跟踪状态

// DebugNode是DO NOTHING节点的替代，仅用于验证框架流程，不完成实际功能
// 当跟踪丢失及未完成重新主体目标选择时，使用此节点
// 二个输入端口，二个输出端口
// 输入端口0：RGB图像；输入端口1：主体目标区域
// 输出端口0：目标精确定位区域（ImageSignal<float>类型信号）；输出端口1：跟踪状态（StateSignal类型信号）
DebugNode* donothing_node = new DebugNode(2,2);
donothing_node->setUnitName("donothing");
donothing_node->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);          // 跟踪目标框
donothing_node->setPortCategoryType(1, SIGNAL_CATEGORY_STATE);                          // 跟踪目标框

// 控制tracking or donothing
IfElseNode* tracking_or_donothing_node = new IfElseNode(tracking_node, donothing_node);

// 逻辑节点（控制当未获得检测结果时，跟踪模型依然正常执行）
LogicalNode* logical_node = new LogicalNode(LOGICAL_OR);

// feadback node
std::map<int, bool> tracking_state_map;
tracking_state_map[0] = false;      // 跟丢状态
tracking_state_map[1] = true;       // 正常跟踪状态
tracking_state_map[2] = true;       // 临时跟丢状态
State2BooleanNode* tracking_state_feadback = new State2BooleanNode(tracking_state_map);

// 3.step add all node to pipeline
AF->add(placeholder,"placeholder", SOURCE_NODE);
AF->add(asyn_node, "det_and_finding");
AF->add(logical_node, "logical_node");
AF->add(tracking_or_donothing_node, "tracking_or_donothing_node", SINK_NODE);
AF->add(tracking_state_feadback, "tracking_state_feadback_node");

// 4.step link all node in pipeline
AF->bind("placeholder", 0, "det_and_finding", 0);
AF->bind("placeholder", 0, "tracking_or_donothing_node", 1);
AF->bind("det_and_finding", 1, "logical_node", 0);
AF->bind("logical_node", 0, "tracking_or_donothing_node", 0);
AF->bind("det_and_finding", 0, "tracking_or_donothing_node", 2);
AF->bind("tracking_or_donothing_node", 1, "tracking_state_feadback_node", 0);
AF->bind("tracking_state_feadback_node", 0, "det_and_finding", 1);
AF->bind("tracking_state_feadback_node",0, "logical_node",1);

EAGLEEYE_END_PIPELINE_INITIALIZE
}