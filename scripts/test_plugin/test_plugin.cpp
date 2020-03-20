#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/processnode/DataSourceNode.h"
#include "eagleeye/common/EagleeyeVisTool.h"
#include "eagleeye/processnode/IncrementOrAddNode.h"
#include "test_plugin.h"

namespace eagleeye{

/**
 * @brief register pipeline plugin
 * 
 */
EAGLEEYE_PIPELINE_REGISTER(test, 1.0.0.0, xxxxx);

/**
 * @brief configure pipeline plugin
 * 
 */
EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(test)
// 1.step build datasource node
DataSourceNode<ImageSignal<float>>* data_source = 
                new DataSourceNode<ImageSignal<float>>();
data_source->setSourceType(EAGLEEYE_SIGNAL_IMAGE);
data_source->setSourceTarget(EAGLEEYE_CAPTURE_STILL_IMAGE);


// 2.step build your algorithm node
// for example:
// InstancePersonSegMRCNNNode* instance_person_seg_node = new InstancePersonSegMRCNNNode("maskrcnn", "GPU");
//

IncrementOrAddNode<ImageSignal<float>,ImageSignal<float>>* a = 
            new IncrementOrAddNode<ImageSignal<float>,ImageSignal<float>>(2, 1);

IncrementOrAddNode<ImageSignal<float>,ImageSignal<float>>* b = 
            new IncrementOrAddNode<ImageSignal<float>,ImageSignal<float>>();


// 3.step add all node to pipeline
// 3.1.step add data source node
test->add(data_source,"data_source", SOURCE_NODE);

// 3.2.step add your algorithm node
// for example:
// test->add(instance_person_seg_node, "instance_person_seg_node");
test->add(a, "a");
test->add(b, "b", SINK_NODE);

// 4.step link all node in pipeline
// for example:
// test->bind("data_source",0,"instance_person_seg_node",0);
test->bind("data_source", 0, "a", 0);
test->bind("a", 0, "b", 0);
test->bind("b", 0, "a", 1);

EAGLEEYE_END_PIPELINE_INITIALIZE
}