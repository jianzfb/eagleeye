#ifndef _EAGLEEYE_TNN_OP_H_
#define _EAGLEEYE_TNN_OP_H_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/tnn_run.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <string>
#include <vector>
#include <memory>
namespace eagleeye{
namespace dataflow{

template<std::size_t IN, std::size_t OUT>
class TnnOp: public BaseOp<IN, OUT>{
public:
    using BaseOp<IN, OUT>::init;
    TnnOp(std::string model_name, 
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
        m_reverse_channel = false;
    };
    TnnOp(){
        m_model_run = NULL;
        m_model_init = false;
        m_reverse_channel = false;  
        m_device = "CPU";      
    };
    virtual ~TnnOp(){
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
            this->m_reverse_channel = (bool)(int(params["reverse_channel"][0]));
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
        if(params.find("device") != params.end()){
            this->m_device = params["device"][0];
        }
        if(params.find("output_names") != params.end()){
            this->m_output_names = params["output_names"];
        }
        if(params.find("model_folder") != params.end()){
            this->m_model_folder = params["model_folder"][0];
            EAGLEEYE_LOGD("Set TNN model folder %s", this->m_model_folder.c_str());
        }
        if(params.find("writable_path") != params.end()){
            this->m_writable_path = params["writable_path"][0];
            EAGLEEYE_LOGD("Set TNN Writable folder %s", this->m_writable_path.c_str());
        }
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(!this->m_model_init){
            std::vector<bool> is_inner_preprocess;
            for(int input_i=0; input_i<input.size(); ++input_i){
                if(this->m_mean.size() > 0 && this->m_std.size() > 0 && (input[input_i].type() == EAGLEEYE_CHAR || input[input_i].type() == EAGLEEYE_UCHAR)){
                    // 输入图像并且类型是uint8时，需要将减均值除方差置于NPU内部处理
                    is_inner_preprocess.push_back(true);
                }
                else{
                    // 直接穿透，rknn不对输入进行均值方差处理
                    is_inner_preprocess.push_back(false);
                }
            }

            m_model_run = std::shared_ptr<ModelEngine>(new ModelRun<TNNRun>(
                        m_model_name,
                        m_device, 
                        m_input_names,
                        m_input_shapes,
                        m_output_names,
                        m_output_shapes,
                        m_num_threads,
                        m_model_power,
                        m_writable_path, is_inner_preprocess), [](ModelEngine* d) {delete d;});

            for(int input_i=0; input_i<input.size(); ++input_i){
                if(is_inner_preprocess[input_i]){
                    ConvertParam cp;
                    // TODO, 这里默认所有输入的均值和方差是一样的
                    for(int i=0; i<this->m_std.size(); ++i){
                        cp.scale[i] = 1.0f/this->m_std[i];
                        cp.bias[i] = -(this->m_mean[i]/this->m_std[i]);
                    }

                    cp.reverse_channel = this->m_reverse_channel;
                    m_model_run->setInputConvertParam(
                        this->m_input_names[input_i],
                        cp
                    );
                }
            }

            this->m_model_run->setModelFolder(m_model_folder);
            this->m_model_init = this->m_model_run->initialize();
        }
        if(!this->m_model_init){
            EAGLEEYE_LOGE("tnn model fail to initialize.");
            return -1;
        }

        // 运行
        std::map<std::string, const unsigned char*> inputs;
        std::map<std::string, unsigned char*> outputs;

        // 输入
        bool input_is_ready = true;
        for(int input_i=0; input_i<m_input_names.size(); ++input_i){
            if(input[input_i].empty() || input[input_i].dims()[0] == 0){
                input_is_ready = false;
                break;
            }

            inputs[m_input_names[input_i]] = input[input_i].cpu<unsigned char>();
        }
        if(!input_is_ready){
            for(int output_i=0; output_i<m_output_names.size(); ++output_i){
                std::vector<int64_t> output_shape = this->m_output_shapes[output_i];
                output_shape[0] = 0;
                this->m_outputs[output_i] = Tensor(
                            output_shape,
                            m_output_types[output_i],
                            DataFormat::AUTO,
                            NULL
                        );
            }
            return 0;
        }

        // 输出
        for(int output_i=0; output_i<m_output_names.size(); ++output_i){
            outputs[m_output_names[output_i]] = NULL;
        }

        // 运行
        this->m_model_run->run(inputs, outputs);

        // 导出
        for(int output_i=0; output_i<m_output_names.size(); ++output_i){
            std::string output_name = this->m_output_names[output_i];
            std::vector<int64_t> output_shape = this->m_output_shapes[output_i];

            this->m_outputs[output_i] = Tensor(
                        output_shape,
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
    std::vector<std::vector<int64_t>> m_output_shapes;
    std::vector<EagleeyeType> m_input_types;
    std::vector<EagleeyeType> m_output_types;

    std::vector<float> m_mean;
    std::vector<float> m_std;
    bool m_reverse_channel;

	int m_num_threads;
	RunPower m_model_power;
    std::string m_model_folder;
	std::string m_writable_path;
    bool m_model_init;
};    

} // namespace dataflow
} // namespace eagleeye


#endif