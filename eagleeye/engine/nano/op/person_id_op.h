#ifndef _EAGLEEYE_PERSON_ID_OP_
#define _EAGLEEYE_PERSON_ID_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

// 由于人体ID不像FaceID强判别性，导致极易存在同一个人存在若干组ID
// 如果发现当前计算的特征不在数据库中，则新增
// 如果在，则作为同一个ID的不同样本
// 初始化时，清空数据库
namespace eagleeye{
namespace dataflow{
class PersonIdOp:public BaseOp<1, 1>, DynamicCreator<PersonIdOp>{
public:
    using BaseOp<1, 1>::init;
    PersonIdOp();
    PersonIdOp(const PersonIdOp& op);
    virtual ~PersonIdOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params);

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::map<std::string, Tensor> m_person_gallery;
    std::map<std::string, int> m_person_offset;
    float m_score_thres;
};
}
}

#endif