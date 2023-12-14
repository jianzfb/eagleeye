#include "eagleeye/engine/nano/op/inv_keepratio_layout_op.h"
#include "eagleeye/engine/nano/op/image_rotate_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <fstream>

namespace eagleeye{
namespace dataflow{
InvKeepRatioLayoutOp::InvKeepRatioLayoutOp(){}
InvKeepRatioLayoutOp::InvKeepRatioLayoutOp(const InvKeepRatioLayoutOp& op){}
InvKeepRatioLayoutOp::~InvKeepRatioLayoutOp(){}

int InvKeepRatioLayoutOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int InvKeepRatioLayoutOp::runOnCpu(const std::vector<Tensor>& input){
    // 0: layout 
    // 1: location
    const int* layout_info = input[0].cpu<int>();
    int layout_offset_x = layout_info[0];
    int layout_offset_y = layout_info[1];
    int layout_w = layout_info[2];
    int layout_h = layout_info[3];
    int layout_ori_w = layout_info[4];
    int layout_ori_h = layout_info[5];
    ImageRotateMode rotate_mode = IMAGE_ROTATE_0;
    if(input[0].dims()[0] > 6){
        rotate_mode = (ImageRotateMode)(layout_info[6]);
    }

    const Tensor position = input[1];
    Dim position_dim = position.dims();
    if(this->m_outputs[0].numel() != position.numel()){
        this->m_outputs[0] = Tensor(position_dim.data(), position.type(), position.format(), CPU_BUFFER);
    }

    int num = position_dim[0];
    int points_num = position_dim[1] / 2;
    const float* position_ptr = position.cpu<float>();
    float* inv_position_ptr = this->m_outputs[0].cpu<float>();
    for(int i=0; i<num; ++i){
        const float* position_row_ptr = position_ptr + i * position_dim[1];
        float* inv_position_row_ptr = inv_position_ptr + i * position_dim[1];
        for(int j=0; j<points_num; ++j){
            // x
            inv_position_row_ptr[j*2] = (position_row_ptr[j*2] - layout_offset_x) * ((float)layout_ori_w/(float)layout_w);
            // y
            inv_position_row_ptr[j*2+1] = (position_row_ptr[j*2+1] - layout_offset_y) * ((float)layout_ori_h/(float)layout_h);;
        }
    }

    if(rotate_mode == IMAGE_ROTATE_90){
        for(int i=0; i<num; ++i){
            float* inv_position_row_ptr = inv_position_ptr + i * position_dim[1];
            for(int j=0; j<points_num; ++j){
                // x
                float temp_x = inv_position_row_ptr[j*2];
                inv_position_row_ptr[j*2] = inv_position_row_ptr[j*2+1];
                // y
                inv_position_row_ptr[j*2+1] = layout_ori_w - temp_x;
            }
        }
    }
    else if(rotate_mode == IMAGE_ROTATE_180){
        for(int i=0; i<num; ++i){
            float* inv_position_row_ptr = inv_position_ptr + i * position_dim[1];
            for(int j=0; j<points_num; ++j){
                // x
                inv_position_row_ptr[j*2] = layout_ori_w-inv_position_row_ptr[j*2];
                // y
                inv_position_row_ptr[j*2+1] = layout_ori_h-inv_position_row_ptr[j*2+1];
            }
        }
    }
    else if(rotate_mode == IMAGE_ROTATE_270){
        for(int i=0; i<num; ++i){
            float* inv_position_row_ptr = inv_position_ptr + i * position_dim[1];
            for(int j=0; j<points_num; ++j){
                // x
                float temp_x = inv_position_row_ptr[j*2];
                inv_position_row_ptr[j*2] = layout_ori_h - inv_position_row_ptr[j*2+1];
                // y
                inv_position_row_ptr[j*2+1] = temp_x;
            }
        }
    }
    else if(rotate_mode == IMAGE_FLIP_H){
        for(int i=0; i<num; ++i){
            float* inv_position_row_ptr = inv_position_ptr + i * position_dim[1];
            for(int j=0; j<points_num; ++j){
                // x
                inv_position_row_ptr[j*2] = layout_ori_w - inv_position_row_ptr[j*2];
            }
        }
    }
    return 0;
}

int InvKeepRatioLayoutOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}