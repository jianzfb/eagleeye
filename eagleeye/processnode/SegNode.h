#ifndef _EAGLEEYE_SEGNODE_H_
#define _EAGLEEYE_SEGNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/engine/proxy_run.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeOpenCL.h"


namespace eagleeye{
enum SegResizeMode{
    DEFAULT_RESIZE = 0,
    SAME_RESIZE
};

class SegNode:public ImageProcessNode<ImageSignal<Array<unsigned char, 3>>, ImageSignal<float>>{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef SegNode							                                                Self;
	typedef ImageProcessNode<ImageSignal<Array<unsigned char, 3>>, ImageSignal<float>>	    Superclass;

    SegNode(std::string model_name, 
                 std::string device, 
                 std::string input_node, 
                 std::vector<int> input_size,
                 std::string output_node,
                 std::vector<int> output_size,
                 std::vector<std::string> snpe_special_nodes,
                 SegResizeMode resized_mode=DEFAULT_RESIZE,
                 bool need_softmax=false);
    virtual ~SegNode();

    /**
	 *	@brief get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(SegNode);


    typedef ImageSignal<Array<unsigned char, 3>>    IMAGE_SIGNAL_TYPE;
    typedef ImageSignal<float>                      MASK_SIGNAL_TYPE;

    /**
     * @brief define input/output port
     * 
     */
    EAGLEEYE_INPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,      0,      FRAME);
    EAGLEEYE_OUTPUT_PORT_TYPE(MASK_SIGNAL_TYPE,      0,      MASK);

    /**
     * @brief Set/Get the Writable Path object
     * 
     * @param path 
     */
    void setModelPath(std::string path);
    void getModelPath(std::string& path);

    /**
     * @brief run seg model
     * 
     * @param frame 
     * @return Tensor<float> 
     */
    void runSeg(const Matrix<Array<unsigned char,3>>& frame, int label, Matrix<float>& label_map);

protected:
    float m_mean_r;
    float m_mean_g;
    float m_mean_b;

    float m_var_r;
    float m_var_g;
    float m_var_b;

    int m_model_h;
    int m_model_w;

    int m_frame_width;
    int m_frame_height;  

    SegResizeMode m_resized_mode;

private:
    SegNode(const SegNode&);
    void operator=(const SegNode&);

    ModelRun* m_seg_model;
    std::string m_input_node;
    std::string m_output_node;

    int m_output_h;
    int m_output_w;
    int m_class_num;
    Matrix<Array<float,3>> m_model_input_f;
    unsigned char* m_temp_ptr;

    bool m_need_softmax;
// #ifdef EAGLEEYE_OPENCL_OPTIMIZATION
//     EAGLEEYE_OPENCL_DECLARE_KERNEL_GROUP(segpreprocess);
// #endif

};

}
#endif