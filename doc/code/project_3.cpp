#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/processnode/Placeholder.h"
#include "eagleeye/processnode/AutoNode.h"
#include "eagleeye/processnode/DebugNode.h"

namespace eagleeye {
    /**
     * @brief register pipeline plugin
     *
     */
    EAGLEEYE_PIPELINE_REGISTER(stab, 1.0.0.0, xxxxx);

    /**
     * @brief configure pipeline plugin
     *
     */
    EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(stab)
        // 1.step build datasource node
        // Placeholder的构造函数参数为true，表明构建图像队列信号
        auto* data_source = new Placeholder<ImageSignal<Array<unsigned char, 3>>>(true);

        // 2.step build your algorithm node
        AutoNode* auto_psn_node = new AutoNode(
            [](){
                SubPipeline* subpipeline = new SubPipeline();

                // DebugNode是人像分割节点的替代，仅用于验证框架流程，不完成实际功能
                // 两个输入端口，三个输出端口
                // 输入端口0：RGB图像； 输入端口1：前一帧人像MASK
                // 输出端口0：当前帧人像MASK，用于连接自身的1号端口；输出端口1：输出当前帧RGB图像；输出端口2：当前帧人像MASK，输出给下一模块
                DebugNode* psn = new DebugNode(2,3);
                psn->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);
                psn->setPortCategoryType(1, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_RGB);
                psn->setPortCategoryType(2, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_FLOAT);

                // 节点指针，名字，节点类型（源节点），忽略端口号
                //（对于源节点，如果不设置，则subpipeline将上游连接的所有信号均输入到此节点）
                subpipeline->add(psn, "psn", SOURCE_NODE, 1);
                // 节点指针，名字，节点类型（灭节点），忽略端口号
                //（对于灭节点，如果不设置，则subpipeline将灭节点的所有输出信号均输出）
                subpipeline->add(psn, "psn", SINK_NODE, 0);
                // 构建连接关系，由于人像分割节点需要使用前一帧分割结果信息，故建立如下连接
                subpipeline->bind("psn", 0, "psn", 1);
                return subpipeline;
            }
        );

        AutoNode* auto_draw_contour_node = new AutoNode(
            [](){
                // DebugNode是人像描边节点的替代，仅用于验证框架流程，不完成实际功能
                // 两个输入端口，一个输出端口
                // 输入端口0：RGB图像；输入端口1：人像分割MASK
                // 输出端口0：人像描边RGB图
                DebugNode* draw_contour_node = new DebugNode(2,1);
                draw_contour_node->setPortCategoryType(0, SIGNAL_CATEGORY_IMAGE, EAGLEEYE_RGB);

                return draw_contour_node;
        });

        // 3.step add all node to pipeline
        stab->add(data_source, "data_source", SOURCE_NODE);
        stab->add(auto_psn_node, "auto_psn_node");
        stab->add(auto_draw_contour_node, "draw_contour_node", SINK_NODE);

        // 4.step link all node in pipeline
        stab->bind("data_source", 0, "auto_psn_node", 0);
        stab->bind("auto_psn_node", 0, "draw_contour_node", 0);
        stab->bind("auto_psn_node", 1, "draw_contour_node", 1);

    EAGLEEYE_END_PIPELINE_INITIALIZE
}