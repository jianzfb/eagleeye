#ifndef _EAGLEEYE_AUTOKEEPRATIO_OP_
#define _EAGLEEYE_AUTOKEEPRATIO_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include "eagleeye/engine/nano/op/image_rotate_op.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class AutoKeepRatioOp:public BaseOp<1, 2>,DynamicCreator<AutoKeepRatioOp>{
public:
    using BaseOp<1,2>::init;
    
    AutoKeepRatioOp();
    AutoKeepRatioOp(int width, int height, ImageRotateMode rotate);

    virtual ~AutoKeepRatioOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    int m_target_w;
    int m_target_h;
    ImageRotateMode m_rotate;

    int m_src_handler;
    int m_tgt_handler;

    void* m_src_ptr;
    void* m_tgt_ptr;

    int m_resize_handler;
    int m_resize_buf_size;
    void* m_resize_ptr;

    int m_rotate_handler;
    int m_rotate_buf_size;
    void* m_rotate_ptr;

    
};
}    
}

#endif