#ifndef _EAGLEEYE_COLORCVT_OP_
#define _EAGLEEYE_COLORCVT_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum ColorCvtMode{
    COLOR_RGB2BGR = 0,
    COLOR_RGBA2RGB = 1,
    COLOR_RGBA2BGR = 2,
    COLOR_RGB2RGBA = 3,
    COLOR_BGR2RGBA = 4    
};

class ColorCvtOp:public BaseOp<1, 1>, DynamicCreator<ColorCvtOp>{
public:
    using BaseOp<1, 1>::init;
    ColorCvtOp();
    ColorCvtOp(ColorCvtMode mode);
    ColorCvtOp(const ColorCvtOp& op);
    virtual ~ColorCvtOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    void convertRGB2BGR(const Tensor src, Tensor& tgt);
    void convertRGBA2BGR(const Tensor src, Tensor& tgt);
    void convertRGBA2RGB(const Tensor src, Tensor& tgt);

    void convertRGB2RGBA(const Tensor src, Tensor& tgt);
    void convertBGR2RGBA(const Tensor src, Tensor& tgt);
protected:
    ColorCvtMode m_mode;
};
}
}

#endif