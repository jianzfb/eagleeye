#ifndef _EAGLEEYE_TENSORRT_OP_H_
#define _EAGLEEYE_TENSORRT_OP_H_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/engine/tensorrt_run.h"
#include <string>
#include <vector>
#include <memory>
namespace eagleeye{
namespace dataflow{

template<std::size_t IN, std::size_t OUT>
class TensorrtOp: public BaseOp<IN, OUT>{
public:
    TensorrtOp(std::string model_name, 
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
        m_rgb2bgr = false;
        m_model_init = false;

        m_preprocess_tensors.resize(input_names.size());
    };
    
    TensorrtOp()=default;
    virtual ~TensorrtOp(){};

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

        if(params.find("mean") != params.end()){
            this->m_mean = params["mean"];
        }
        if(params.find("std") != params.end()){
            this->m_std = params["std"];
            for(int i=0; i<this->m_std.size(); ++i){
                this->m_std[i] = 1.0f/this->m_std[i];
            }
        }
        if(params.find("rgb2bgr") != params.end()){
            this->m_rgb2bgr = bool(params["rgb2bgr"][0]);
        }

        if(params.find("num_threads") != params.end()){
            this->m_num_threads = (int)(params["num_threads"][0]);
        }
        this->m_model_power = HIGH_POWER;
        return 0;
    }

    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){
        // init 可能与runOnCpu,runOnGpu不在同一个线程
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
    }

    virtual int init(std::map<std::string, std::vector<std::string>> params){
        // init 可能与runOnCpu,runOnGpu不在同一个线程
        if(params.size() == 0){
            return 0;
        }

        // model_name, device, input_names, output_names, model_folder,writable_path
        if(params.find("model_name") != params.end()){
            this->m_model_name = params["model_name"][0];
        }
        if(params.find("device") != params.end()){
            this->m_device = params["device"][0];
        }
        if(params.find("input_names") != params.end()){
            this->m_input_names = params["input_names"];
        }

        if(params.find("output_names") != params.end()){
            this->m_output_names = params["output_names"];
        }
        if(params.find("model_folder") != params.end()){
            this->m_model_folder = params["model_folder"][0];
        }
        if(params.find("writable_path") != params.end()){
            this->m_writable_path = params["writable_path"][0];
        }

        m_preprocess_tensors.resize(this->m_input_names.size());
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(!this->m_model_init){
            m_model_run = std::shared_ptr<ModelEngine>(new ModelRun<TensorrtRun>(
                        m_model_name,
                        m_device, 
                        m_input_names,
                        m_input_shapes,
                        m_output_names,
                        m_output_shapes,
                        m_num_threads,
                        m_model_power,
                        m_writable_path), [](ModelEngine* d) {delete d;});

            // 设置模型根目录，（将在此目录下寻找模型文件）
            this->m_model_run->setModelFolder(m_model_folder);
            // 初始化模型
            this->m_model_init = this->m_model_run->initialize();
        }
        if(!this->m_model_init){
            EAGLEEYE_LOGE("tensor model fail to initialize.");
            return -1;
        }

        // 输入数据
        for(int input_i=0; input_i<m_input_names.size(); ++input_i){
            // 检查数据格式
            const Tensor x = input[input_i];
            if(x.type() != EAGLEEYE_FLOAT && x.type() != EAGLEEYE_UCHAR && x.type() != EAGLEEYE_CHAR && x.type() != EAGLEEYE_INT){
                EAGLEEYE_LOGE("x type only support float/uchar/int.");
                return -1;
            }

            // 获得模型引擎分配的内部空间
            if(m_preprocess_tensors[input_i].empty() || m_preprocess_tensors[input_i].dims().production() != x.dims().production()){
                m_preprocess_tensors[input_i] = Tensor(
                        x.dims().data(),
                        EAGLEEYE_FLOAT,
                        DataFormat::AUTO,
                        MemoryType::CPU_BUFFER
                    );
            }

            float* preprocessed_data = m_preprocess_tensors[input_i].cpu<float>();
            // continue;
            Dim x_dims = x.dims();
            if(x.type() == EAGLEEYE_UCHAR || x.type() == EAGLEEYE_CHAR){
                // 需要进行预处理流程
                // NxHxWx3 或 HxWx3 格式
                if(x_dims.size() == 4){
                    // NxHxWx3
                    int batch_size = x_dims[0];
                    int offset_size = x_dims[1] * x_dims[2] * x_dims[3];
                    int x_width = x_dims[2];
                    int x_height = x_dims[1];

                    for(int b_i=0; b_i<batch_size; ++b_i){
                        if(this->m_rgb2bgr){
                            this->m_model_run->bgrToRgbTensorCHW(
                                x.cpu<unsigned char>() + b_i * x_width * x_height * 3, 
                                preprocessed_data + b_i * x_width * x_height * 3, 
                                x_width, 
                                x_height, 
                                &(this->m_mean[0]),
                                &(this->m_std[0])
                            );
                        }
                        else{
                            this->m_model_run->bgrToTensorCHW(
                                x.cpu<unsigned char>() + b_i * x_width * x_height * 3, 
                                preprocessed_data + b_i * x_width * x_height * 3, 
                                x_width, 
                                x_height, 
                                &(this->m_mean[0]), 
                                &(this->m_std[0])
                            );
                        }     
                    }
                }
                else{
                    // HxWx3
                    int x_width = x_dims[1];
                    int x_height = x_dims[0];
                    if(this->m_rgb2bgr){
                        this->m_model_run->bgrToRgbTensorCHW(
                            x.cpu<unsigned char>(), 
                            preprocessed_data, 
                            x_width, 
                            x_height, 
                            &(this->m_mean[0]), 
                            &(this->m_std[0])
                        );
                    }
                    else{
                        this->m_model_run->bgrToTensorCHW(
                            x.cpu<unsigned char>(), 
                            preprocessed_data, 
                            x_width, 
                            x_height, 
                            &(this->m_mean[0]), 
                            &(this->m_std[0])
                        );
                    }                       
                }

                continue;
            }

            // 输入数据直接是浮点数据
            m_preprocess_tensors[input_i] = x;
        }

        // 运行
        std::map<std::string, const unsigned char*> inputs;
        for(int input_i=0; input_i<m_input_names.size(); ++input_i){
            inputs[m_input_names[input_i]] = m_preprocess_tensors[input_i].cpu<unsigned char>();
        }

        std::map<std::string, unsigned char*> outputs;
        for(int output_i=0; output_i<m_output_names.size(); ++output_i){
            outputs[m_output_names[output_i]] = NULL;
        }
        this->m_model_run->run(inputs, outputs);

        // 输出数据
        for(int output_i=0; output_i<m_output_names.size(); ++output_i){
            std::string output_name = this->m_output_names[output_i];

            this->m_outputs[output_i] = Tensor(
                        m_output_shapes[output_i],
                        m_output_types[output_i],
                        DataFormat::AUTO,
                        outputs[output_name]
                    );
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
    std::map<std::string, std::string> m_alias_output_names;
    std::vector<std::vector<int64_t>> m_output_shapes;
    std::vector<EagleeyeType> m_input_types;
    std::vector<EagleeyeType> m_output_types;

    std::vector<Tensor> m_preprocess_tensors;
    std::vector<float> m_mean;
    std::vector<float> m_std;
    bool m_rgb2bgr;
	int m_num_threads;
	RunPower m_model_power;
    std::string m_model_folder;
	std::string m_writable_path;
    bool m_model_init;
};    

} // namespace dataflow
} // namespace eagleeye


#endif