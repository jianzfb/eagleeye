#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/processnode/DataSourceNode.h"
#include "eagleeye/common/EagleeyeVisTool.h"
#include "None_plugin.h"

namespace eagleeye{

/**
 * @brief register pipeline plugin
 * 
 */
EAGLEEYE_PIPELINE_REGISTER(None, None, None);

/**
 * @brief configure pipeline plugin
 * 
 */
EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(None)
// 1.step build datasource node
DataSourceNode<ImageSignal<Array<unsigned char, 3>>>* data_source = new DataSourceNode<ImageSignal<Array<unsigned char, 3>>>();
data_source->setSourceType(EAGLEEYE_SIGNAL_IMAGE);
data_source->setSourceTarget(EAGLEEYE_CAPTURE_STILL_IMAGE);

// 2.step build your algorithm node
// for example:
// InstancePersonSegMRCNNNode* instance_person_seg_node = new InstancePersonSegMRCNNNode("maskrcnn", "GPU");
//

// 3.step add all node to pipeline
// 3.1.step add data source node
None->add(data_source,"data_source");
// 3.2.step add your algorithm node
// for example:
// None->add(instance_person_seg_node, "instance_person_seg_node");

// 4.step link all node in pipeline
// for example:
// None->bind("data_source",0,"instance_person_seg_node",0);

EAGLEEYE_END_PIPELINE_INITIALIZE
}