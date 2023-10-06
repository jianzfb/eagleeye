#include "eagleeye/engine/nano/op/yuv_op.h"
#include "eagleeye/common/EagleeyeYUV.h"
#include "eagleeye/common/EagleeyeMacro.h"
namespace eagleeye{
namespace dataflow{
YuvOp::YuvOp(){
    m_mode = 0;
    OP_SUPPORT(CPU);
}    
YuvOp::YuvOp(const YuvOp& op){
    OP_SUPPORT(CPU);
}
int YuvOp::init(std::map<std::string, std::vector<float>> params){
    m_mode = 0; // 0: RGB, 1: BGR
    if(params.find("mode") != params.end()){
        this->m_mode = (int)(params["mode"][0]);
    }
    return 0;
}

int YuvOp::runOnCpu(const std::vector<Tensor>& input){
    // 0: yuv data
    // 1: height,width,rotation,format
    unsigned char* yuv_ptr = const_cast<unsigned char*>(input[0].cpu<unsigned char>());
    const int* meta_info_ptr = input[1].cpu<int>();
    int height = meta_info_ptr[0];
    int width = meta_info_ptr[1];
    int rotation = meta_info_ptr[2];    // 0,90,180,270
    int format = meta_info_ptr[3];      // 15(EAGLEEYE_YUV_I420),16(EAGLEEYE_YUV_NV21),17(EAGLEEYE_YUV_NV12)

    if(this->m_outputs[0].numel() != height * width * 3){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{height, width, 3},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
    }
    unsigned char* out_ptr = this->m_outputs[0].cpu<unsigned char>();

    if(this->m_mode == 0){
        // to RGB
        if(format == EAGLEEYE_YUV_I420){
            eagleeye_I420_to_RGB(yuv_ptr, width, height, out_ptr);
        }
        else if(format == EAGLEEYE_YUV_NV21){
            eagleeye_NV21_to_RGB(yuv_ptr, width, height, out_ptr);
        }
        else if(format == EAGLEEYE_YUV_NV12){
            eagleeye_NV12_to_RGB(yuv_ptr, width, height, out_ptr);
        }
    }
    else{
        // to BGR
        if(format == EAGLEEYE_YUV_I420){
            eagleeye_I420_to_BGR(yuv_ptr, width, height, out_ptr);
        }
        else if(format == EAGLEEYE_YUV_NV21){
            eagleeye_NV21_to_BGR(yuv_ptr, width, height, out_ptr);
        }
        else if(format == EAGLEEYE_YUV_NV12){
            eagleeye_NV12_to_BGR(yuv_ptr, width, height, out_ptr);
        }        
    }

    return 0;
}
int YuvOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
