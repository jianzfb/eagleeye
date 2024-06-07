#ifndef _EAGLEEYE_FACEALIGN_OP_
#define _EAGLEEYE_FACEALIGN_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class FaceAlignOp:public BaseOp<2, 1>, DynamicCreator<FaceAlignOp>{
public:
    using BaseOp<2, 1>::init;
    FaceAlignOp();    
    FaceAlignOp(int target_h, int target_w, int margin=0);
    virtual ~FaceAlignOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_target_h;
    int m_target_w;
    int m_margin;
    float m_margin_ratio;

    int m_src_handler;
    int m_tgt_handler;

    void* m_src_ptr;
    void* m_tgt_ptr;
};
}
}

#endif