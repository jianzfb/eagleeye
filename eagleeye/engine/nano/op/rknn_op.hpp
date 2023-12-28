#ifndef _EAGLEEYE_RKNN_OP_H_
#define _EAGLEEYE_RKNN_OP_H_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/rknn_run.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <string>
#include <vector>
#include <memory>
namespace eagleeye{
namespace dataflow{

template<std::size_t IN, std::size_t OUT>
class RknnOp: public BaseOp<IN, OUT>{
public:
    using BaseOp<IN, OUT>::init;
    RknnOp(std::string model_name, 
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
    RknnOp(){
        m_model_run = NULL;
        m_model_init = false;
    };

    virtual ~RknnOp(){
    };

    virtual int init(std::map<std::string, std::vector<float>> params){
        // init 可能与runOnCpu,runOnGpu不在同一个线程
        if(params.size() == 0){
            return 0;
        }

        // input_shapes, input_types, output_shapes, output_types, num_threads, model_power
        if(params.find("input_types") != params.end()){
            this->m_input_types.resize(params["input_types"].size());

            for(int i=0; i<params["input_types"].size(); ++i){
                this->m_input_types[i] = (EagleeyeType)(params["input_types"][i]);
            }
        }

        if(params.find("output_types") != params.end()){
            this->m_output_types.resize(params["output_types"].size());

            for(int i=0; i<params["output_types"].size(); ++i){
                this->m_output_types[i] = (EagleeyeType)(params["output_types"][i]);
            }
        }        
        if(params.find("num_threads") != params.end()){
            this->m_num_threads = (int)(params["num_threads"][0]);
        }
        this->m_model_power = HIGH_POWER;
     
        if(params.find("mean") != params.end()){
            this->m_mean = params["mean"];
        }
        if(params.find("std") != params.end()){
            this->m_std = params["std"];
        }
        if(params.find("reverse_channel") != params.end()){
            bool reverse_channel = (bool)(int(params["reverse_channel"][0]));
            if(reverse_channel){
                EAGLEEYE_LOGE("rknn engine dont support reverse_channel=true");
            }
        }        
        return 0;
    }
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){
        if(params.size() == 0){
            return 0;
        }

        if(params.find("input_shapes") != params.end()){
            this->m_input_shapes.resize(params["input_shapes"].size());
            for(int i=0; i<params["input_shapes"].size(); ++i){
                for(int j=0; j<params["input_shapes"][i].size(); ++j){
                    this->m_input_shapes[i].push_back(
                         (int64_t)(params["input_shapes"][i][j])
                    );
                }
            }
        }

        if(params.find("output_shapes") != params.end()){
            this->m_output_shapes.resize(params["output_shapes"].size());

            for(int i=0; i<params["output_shapes"].size(); ++i){
                for(int j=0; j<params["output_shapes"][i].size(); ++j){
                    this->m_output_shapes[i].push_back(
                         (int64_t)(params["output_shapes"][i][j])
                    );
                }
            }
        }   
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::string>> params){
        // init 可能与runOnCpu,runOnGpu不在同一个线程
        if(params.size() == 0){
            return 0;
        }
        if(params.find("model_name") != params.end()){
            this->m_model_name = params["model_name"][0];
        }
        if(params.find("input_names") != params.end()){
            this->m_input_names = params["input_names"];
        }

        if(params.find("output_names") != params.end()){
            this->m_output_names = params["output_names"];
        }
        if(params.find("model_folder") != params.end()){
            this->m_model_folder = params["model_folder"][0];
            EAGLEEYE_LOGD("Set RKNN model folder %s", this->m_model_folder.c_str());
        }
        if(params.find("writable_path") != params.end()){
            this->m_writable_path = params["writable_path"][0];
            EAGLEEYE_LOGD("Set RKNN Writable folder %s", this->m_writable_path.c_str());
        }        
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(!this->m_model_init){
            bool is_inner_preprocess = false;
            if(this->m_mean.size() > 0 && this->m_std.size() > 0){
                is_inner_preprocess = true;
            }
            m_model_run = std::shared_ptr<ModelEngine>(new ModelRun<RknnRun>(
                        m_model_name,
                        m_device, 
                        m_input_names,
                        m_input_shapes,
                        m_output_names,
                        m_output_shapes,
                        m_num_threads,
                        m_model_power,
                        m_writable_path,
                        is_inner_preprocess), [](ModelEngine* d) {delete d;});

            this->m_model_run->setModelFolder(m_model_folder);
            this->m_model_init = this->m_model_run->initialize();
        }
        if(!this->m_model_init){
            EAGLEEYE_LOGE("rknn model fail to initialize.");
            return -1;
        }

        // 运行
        if(input.size() == 0 || input[0].dims()[0] == 0){
            // do nothing
            return 0;
        }

        int batch_size = 1;
        if(input[0].dims().size() >= 4 && input[0].dims()[0] > 1){
            batch_size = input[0].dims()[0];
        }

        for(int output_i=0; output_i<m_output_names.size(); ++output_i){
            std::string output_name = this->m_output_names[output_i];
            std::vector<int64_t> output_shape = this->m_output_shapes[output_i];
            output_shape[0] = batch_size;
            if(this->m_outputs[output_i].empty() || this->m_outputs[output_i].dims()[0] != batch_size){
                this->m_outputs[output_i] = Tensor(
                    output_shape,
                    m_output_types[output_i],
                    DataFormat::AUTO,
                    CPU_BUFFER
                );
            }
        }

        for(int b_i=0; b_i<batch_size; ++b_i){
            std::map<std::string, const unsigned char*> inputs;
            std::map<std::string, unsigned char*> outputs;

            // 输入
            for(int input_i=0; input_i<m_input_names.size(); ++input_i){
                int slice_size = input[input_i].numel() / batch_size;
                inputs[m_input_names[input_i]] = input[input_i].cpu<unsigned char>() + b_i * slice_size * input[input_i].elemsize();
            }

            // 输出
            for(int output_i=0; output_i<m_output_names.size(); ++output_i){
                outputs[m_output_names[output_i]] = NULL;
            }

            this->m_model_run->run(inputs, outputs);

            // 导出
            for(int output_i=0; output_i<m_output_names.size(); ++output_i){
                int slice_size = this->m_outputs[output_i].numel() / batch_size;
                int elem_size = this->m_outputs[output_i].elemsize();
                char* output_ptr = this->m_outputs[output_i].template cpu<char>() + b_i * slice_size * elem_size;
                memcpy(output_ptr, outputs[m_output_names[output_i]], slice_size * elem_size);
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

    std::vector<float> m_mean;
    std::vector<float> m_std;    
};    

} // namespace dataflow
} // namespace eagleeye


#endif