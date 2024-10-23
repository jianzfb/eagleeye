#ifndef _EAGLEEYE_FACE_ID_OP_
#define _EAGLEEYE_FACE_ID_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class FaceIdOp:public BaseOp<2, 1>, DynamicCreator<FaceIdOp>{
public:
    using BaseOp<2, 1>::init;
    FaceIdOp();
    FaceIdOp(const FaceIdOp& op);
    virtual ~FaceIdOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params);

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::string m_cache_folder;
    std::map<std::string, long> m_face_gallery_update_time;
    std::map<std::string, std::map<std::string, Tensor>> m_face_gallery;
    float m_score_thres;
};
}
}

#endif