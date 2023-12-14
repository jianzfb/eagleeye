#ifndef _EAGLEEYE_IMAGEROTATE_OP_H_
#define _EAGLEEYE_IMAGEROTATE_OP_H_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum ImageRotateMode{
    IMAGE_ROTATE_0 = 0,
    IMAGE_ROTATE_90 = 1,
    IMAGE_ROTATE_180 = 2,
    IMAGE_ROTATE_270 = 3,
    IMAGE_FLIP_H = 4
};

class ImageRotateOp:public BaseOp<1,1>, DynamicCreator<ImageRotateOp>{
public:
    using BaseOp<1, 1>::init;
    ImageRotateOp();
    ImageRotateOp(ImageRotateMode mode);
    ImageRotateOp(const ImageRotateOp& op);
    virtual ~ImageRotateOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_image_rotate_mode;

    int m_rk_src_handler;
    int m_rk_tgt_handler;

    void* m_rk_src_image_ptr;
    void* m_rk_tgt_image_ptr;
};

class ImageRotateDynamicOp:public BaseOp<2,1>, DynamicCreator<ImageRotateDynamicOp>{
public:
    using BaseOp<2, 1>::init;
    ImageRotateDynamicOp();
    ImageRotateDynamicOp(const ImageRotateDynamicOp& op);
    virtual ~ImageRotateDynamicOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_rk_src_handler;
    int m_rk_tgt_handler;

    void* m_rk_src_image_ptr;
    void* m_rk_tgt_image_ptr;
};
}
}

#endif