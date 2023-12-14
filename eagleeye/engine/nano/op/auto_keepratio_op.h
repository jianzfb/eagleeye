#ifndef _EAGLEEYE_AUTO_KEEPRATIO_OP_
#define _EAGLEEYE_AUTO_KEEPRATIO_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include "eagleeye/engine/nano/op/image_rotate_op.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class AutoKeepRatioOp:public BaseOp<1, 2>, DynamicCreator<AutoKeepRatioOp>{
public:
    using BaseOp<1, 2>::init;
    AutoKeepRatioOp();
    AutoKeepRatioOp(int target_h, int target_w, ImageRotateMode image_rotate_mode);
    AutoKeepRatioOp(const AutoKeepRatioOp& op);
    virtual ~AutoKeepRatioOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_target_h;
    int m_target_w;
    ImageRotateMode m_image_rotate_mode;

    void* m_rk_src_image_ptr;
    void* m_rk_resize_image_ptr;
    void* m_rk_rotate_image_ptr;
    void* m_rk_paste_image_ptr;

    int m_rk_src_handler;
    int m_rk_resize_tgt_handler;
    int m_rk_rotate_tgt_handler;
    int m_rk_paste_tgt_handler;

    int m_resize_dst_buf_size;
    int m_rotate_dst_buf_size;
};
}
}

#endif