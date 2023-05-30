#include "eagleeye/engine/nano/op/arange_op.h"
namespace eagleeye{
namespace dataflow{
ArangeOp::ArangeOp(int64_t start, int64_t stop, int64_t step, EagleeyeType data_type)
    :m_start(start),
     m_stop(stop),
     m_step(step),
     m_data_type(data_type){

}
ArangeOp::ArangeOp(const ArangeOp& op)
    :m_start(op.m_start),
     m_stop(op.m_stop),
     m_step(op.m_step),
     m_data_type(op.m_data_type){

}

int ArangeOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template<typename T>
void _arange_func(int64_t s, int64_t e, int64_t step, T* data){
    int64_t col_i = 0;
	for(int i=s; i<e; i+=step){
		data[col_i++] = i;
	}
}
int ArangeOp::runOnCpu(const std::vector<Tensor>& input){
    if(!(this->m_data_type == EAGLEEYE_FLOAT || this->m_data_type == EAGLEEYE_INT)){
        EAGLEEYE_LOGE("only support float,int.");
        return -1;
    }

    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0){
        int64_t num = (this->m_stop - this->m_start)/this->m_step;
        this->m_outputs[0] =             
            Tensor(std::vector<int64_t>{num},
                    m_data_type,
                    DataFormat::AUTO,
                    CPU_BUFFER); 

        if(m_data_type == EAGLEEYE_FLOAT){
            float* out_data = this->m_outputs[0].cpu<float>();
            _arange_func<float>(this->m_start, this->m_stop, this->m_step, out_data);
        }
        else{
            int32_t* out_data = this->m_outputs[0].cpu<int32_t>();
            _arange_func<int32_t>(this->m_start, this->m_stop, this->m_step, out_data);
        }

    }
    return 0;
}
int ArangeOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

} // namespace dataflow
} // namespace eagleeye