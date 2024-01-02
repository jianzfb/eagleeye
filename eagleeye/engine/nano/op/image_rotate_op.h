#ifndef _EAGLEEYE_IMAGEROTATE_OP_
#define _EAGLEEYE_IMAGEROTATE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum ImageRotateMode{
    IMAGE_ROTATE_0      = 0,
    IMAGE_ROTATE_90     = 1,
    IMAGE_ROTATE_180    = 2,
    IMAGE_ROTATE_270    = 3
};

class ImageRotateOp:public BaseOp<1, 1>,DynamicCreator<ImageRotateOp>{
public:
    using BaseOp<1,1>::init;
    ImageRotateOp(){
        m_rotate_mode = IMAGE_ROTATE_0;
        m_src_handler = 0;
        m_tgt_handler = 0;
        m_src_ptr = NULL;
        m_tgt_ptr = NULL;
    }
    ImageRotateOp(ImageRotateMode rotate_mode){
        m_rotate_mode = rotate_mode;
        m_src_handler = 0;
        m_tgt_handler = 0;
        m_src_ptr = NULL;
        m_tgt_ptr = NULL;
    }
    virtual ~ImageRotateOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    ImageRotateMode m_rotate_mode;

    int m_src_handler;
    int m_tgt_handler;

    void* m_src_ptr;
    void* m_tgt_ptr;
};

class ImageRotateDynamicOp: public BaseOp<2, 1>,DynamicCreator<ImageRotateDynamicOp>{
public:
    using BaseOp<2,1>::init;
    ImageRotateDynamicOp(){
        m_src_handler = 0;
        m_tgt_handler = 0;
        m_src_ptr = NULL;
        m_tgt_ptr = NULL;
    }
    virtual ~ImageRotateDynamicOp(){
        m_src_handler = 0;
        m_tgt_handler = 0;
        m_src_ptr = NULL;
        m_tgt_ptr = NULL;
    }

    virtual int init(std::map<std::string, std::vector<float>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    int m_src_handler;
    int m_tgt_handler;

    void* m_src_ptr;
    void* m_tgt_ptr;
};
}
}

#endif