#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/processnode/ParallelNode.h"
#include "eagleeye/processnode/DebugNode.h"
#include "eagleeye/processnode/Placeholder.h"


namespace eagleeye{
EAGLEEYE_PIPELINE_REGISTER(posedet, 1.0.0.0, xxxxx);


EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(posedet)
// Placeholder的构造函数参数为true，表明构建图像队列信号
Placeholder<ImageSignal<Array<unsigned char, 3>>>* placeholder = new Placeholder<ImageSignal<Array<unsigned char, 3>>>(true);

// 创建并行节点（2个线程）
ParallelNode* parallel_node = new ParallelNode(2, [](){
    // DebugNode是人体姿态关键点检测节点的替代，仅用于验证框架流程，不完成实际功能
    // 一个输入端口，一个输出端口
    // 输入端口0：RGB图像；
    // 输出端口0：当前帧人体关键点检测结果（ImageSignal<float>类型信号）
    DebugNode* det = new DebugNode(1,1);
    det->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);

    return det;
});

posedet->add(placeholder, "placeholder");
posedet->add(parallel_node, "parallel_node");
posedet->bind("placeholder", 0, "parallel_node",0);

EAGLEEYE_END_PIPELINE_INITIALIZE
}