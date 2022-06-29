#include "eagleeye/engine/nano/op/meshgrid_op.h"

namespace eagleeye{
namespace dataflow{
int MeshgridOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template<typename T>
void _meshgrid(const T* x, const T* y, int x_size, int y_size, T* xg, T* yg){
    for(unsigned int y_i=0; y_i<y_size; ++y_i){
		for(unsigned int x_i=0; x_i<x_size; ++x_i){
			xg[y_i*x_size+x_i] = x[x_i];
			yg[y_i*x_size+x_i] = y[y_i];
		}
	}
}
int MeshgridOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    const Tensor y = input[1];

    if(!(x.dims().size() == 1 && y.dims().size() == 1)){
        EAGLEEYE_LOGE("x and y size must be 1.");
        return -1;
    }
    if(!(x.type() == EAGLEEYE_FLOAT && y.type() == EAGLEEYE_FLOAT) &&
        !(x.type() == EAGLEEYE_INT && y.type() == EAGLEEYE_INT)){
        EAGLEEYE_LOGE("x.type() and y.type() must be float or int.");
        return -1;
    }

    Dim x_dims = x.dims();
    Dim y_dims = y.dims();
    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0 || out_dim.production() != x_dims[0]*y_dims[0]){
        this->m_outputs[0] =             
            Tensor(std::vector<int64_t>{y_dims[0], x_dims[0]},
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
        this->m_outputs[1] =             
            Tensor(std::vector<int64_t>{y_dims[0], x_dims[0]},
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER);                     
    }

    if(x.type() == EAGLEEYE_FLOAT){
        const float* x_data = x.cpu<float>();
        const float* y_data = y.cpu<float>();
        float* xg_data = this->m_outputs[0].cpu<float>();
        float* yg_data = this->m_outputs[1].cpu<float>();
        _meshgrid<float>(x_data,y_data, x_dims[0], y_dims[0], xg_data, yg_data);
    }
    else{
        const int32_t* x_data = x.cpu<int32_t>();
        const int32_t* y_data = y.cpu<int32_t>();
        int32_t* xg_data = this->m_outputs[0].cpu<int32_t>();
        int32_t* yg_data = this->m_outputs[1].cpu<int32_t>();
        _meshgrid<int32_t>(x_data,y_data, x_dims[0], y_dims[0], xg_data, yg_data);
    }
    return 0;
}

int MeshgridOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
