#ifndef _EAGLEEYE_VIDEOSTREAM_NODE_H_
#define _EAGLEEYE_VIDEOSTREAM_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageIONode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/hardware/rk.h"


namespace eagleeye{
class VideoStreamNode:public AnyNode{
public:
    typedef VideoStreamNode         Self;
    typedef AnyNode                 Superclass;

    /**
	 *	@brief get class identity	
	 */	 
	EAGLEEYE_CLASSIDENTITY(VideoStreamNode);

    VideoStreamNode(int queue_size, bool get_then_auto_remove=false, bool set_then_auto_remove=false);
    virtual ~VideoStreamNode();

	/**
	 *	@brief parse video data
	 */
	virtual void executeNodeInfo();

    void decode(uint8_t* package_data, int package_size);

    virtual void postexit();

private:
    VideoStreamNode(const VideoStreamNode&);
    void operator=(const VideoStreamNode&);

    int m_queue_size;
    bool m_set_then_auto_remove;
    bool m_get_then_auto_remove;
    RKH264Decoder* m_decoder;
};
}
#endif