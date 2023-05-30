#ifndef _EAGLEEYE_PADDLE_OP_H_
#define _EAGLEEYE_PADDLE_OP_H_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/paddle_run.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <string>
#include <vector>
#include <memory>
namespace eagleeye{
namespace dataflow{

template<std::size_t IN, std::size_t OUT>
class PaddleOp: public BaseOp<Tensor, IN, OUT>{
public:
    PaddleOp(std::string model_name, 
			   std::string device,
			   std::vector<std::string> input_names,
			   std::vector<std::vector<int64_t>> input_shapes,
               std::vector<EagleeyeType> input_types,
			   std::vector<std::string> output_names,
			   std::vector<std::vector<int64_t>> output_shapes,
               std::vector<EagleeyeType> output_types,
			   int num_threads, 
			   RunPower model_power,
               std::string model_folder,
			   std::string writable_path){
        if(IN != input_names.size() || OUT != output_names.size()){
            EAGLEEYE_LOGE("Input_node/output_node number not consistent with IN/OUT.");
            return;
        }

        m_model_run = NULL;        
        m_model_name = model_name;
        m_device = device;
        m_input_names = input_names;
        m_input_shapes = input_shapes;
        m_input_types = input_types;
        m_output_names = output_names;
        m_output_shapes = output_shapes;
        m_output_types = output_types;
        m_num_threads = num_threads;
        m_model_power = model_power;
        m_model_folder = model_folder;
        m_writable_path = writable_path;
        m_model_init = false;
    };
    virtual ~PaddleOp(){
    };

    virtual int init(std::map<std::string, std::vector<float>> params){
        // init 可能与runOnCpu,runOnGpu不在同一个线程
        return 0;
    }
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(!this->m_model_init){
            m_model_run = std::shared_ptr<ModelEngine>(new ModelRun<PaddleRun>(
                        m_model_name,
                        m_device, 
                        m_input_names,
                        m_input_shapes,
                        m_output_names,
                        m_output_shapes,
                        m_num_threads,
                        m_model_power,
                        m_writable_path), [](ModelEngine* d) {delete d;});
            
            this->m_model_run->setModelFolder(m_model_folder);
            this->m_model_init = this->m_model_run->initialize();
        }
        if(!this->m_model_init){
            EAGLEEYE_LOGE("paddle model fail to initialize.");
            return -1;
        }

        int batch_size = input[0].dims()[0];
        // 分配输出内存
        for(int output_i=0; output_i<m_output_names.size(); ++output_i){
            std::vector<int64_t> output_shape = this->m_output_shapes[output_i];
            output_shape[0] = batch_size;
            this->m_outputs[output_i] = Tensor(
                        output_shape,
                        m_output_types[output_i],
                        DataFormat::AUTO,
                        CPU_BUFFER
                    );
        }

        // 逐batch计算
        for(int b_i = 0; b_i < batch_size; ++b_i){
            // 输入数据
            for(int input_i=0; input_i<m_input_names.size(); ++input_i){
                // 检查数据格式
                const Tensor x = input[input_i];
                int input_size = x.dims().count(1,x.dims().size());
                const float* x_batch_data = x.cpu<float>() + b_i*input_size;

                if(x.type() != EAGLEEYE_FLOAT && x.type() != EAGLEEYE_UCHAR && x.type() != EAGLEEYE_CHAR && x.type() != EAGLEEYE_INT){
                    EAGLEEYE_LOGE("x type only support float/uchar/int.");
                    return -1;
                }
                void* preprocessed_data = this->m_model_run->getInputPtr(m_input_names[input_i]);
                memcpy(preprocessed_data, x_batch_data, input_size*sizeof(float));
            }

            // 运行
            std::map<std::string, unsigned char*> inputs;
            std::map<std::string, unsigned char*> outputs;
            for(int output_i=0; output_i<m_output_names.size(); ++output_i){
                outputs[m_output_names[output_i]] = NULL;
            }
            this->m_model_run->run(inputs, outputs);

            // 输出数据
            for(int output_i=0; output_i<m_output_names.size(); ++output_i){
                Tensor output = this->m_outputs[output_i];
                int output_size = output.dims().count(1,output.dims().size());
                float* out_batch_data = output.cpu<float>() + b_i*output_size; 
                memcpy(out_batch_data, outputs[m_output_names[output_i]], sizeof(float)*output_size);
            }
        }

        return 0;
    }

    virtual int runOnGpu(const std::vector<Tensor>& input){
        EAGLEEYE_LOGE("Dont implement (GPU)");
        return 0;
    }

private:
    std::shared_ptr<ModelEngine> m_model_run;
    std::string m_model_name;
    std::string m_device;
    std::vector<std::string> m_input_names;
	std::vector<std::vector<int64_t>> m_input_shapes;
	std::vector<std::string> m_output_names;
    std::vector<std::vector<int64_t>> m_output_shapes;
    std::vector<EagleeyeType> m_input_types;
    std::vector<EagleeyeType> m_output_types;
	int m_num_threads;
	RunPower m_model_power;
    std::string m_model_folder;
	std::string m_writable_path;
    bool m_model_init;
};    

} // namespace dataflow
} // namespace eagleeye


#endif