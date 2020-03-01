#include "eagleeye/engine/tf_run.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"

#ifdef EAGLEEYE_TF_SUPPORT
namespace eagleeye{

ModelRun::ModelRun(std::string model_name, 
				   std::string device,
				   std::vector<std::string> input_names,
				   std::vector<std::vector<int64_t>> input_shapes,
				   std::vector<std::string> output_names,
				   std::vector<std::vector<int64_t>> output_shapes,
				   int omp_num_threads, 
				   int cpu_affinity_policy, 
				   std::string writable_path)
	:ModelEngine(model_name,
				 device,
				 input_names,
				 input_shapes,
				 output_names,
				 output_shapes,
				 omp_num_threads,
				 cpu_affinity_policy,
				 writable_path){
	// pb file path
	this->m_graph_path = model_name;
	if(!endswith(model_name, ".pb")){
		this->m_graph_path = this->m_graph_path+".pb";
	}
	if(!ispath(this->m_graph_path)){
		this->m_graph_path = this->getModelRoot() + std::string("/") + this->m_graph_path;
	}
	EAGLEEYE_LOGD("tensorflow pb %s", this->m_graph_path.c_str());
}

ModelRun::~ModelRun(){
	// close tensorflow session
	this->m_session->Close();
}

bool ModelRun::run(std::map<std::string, unsigned char*> inputs, 
				   std::map<std::string, unsigned char*>& outputs){
	// 1.step fill input tensor
	std::vector<std::pair<std::string, tensorflow::Tensor>> tensor_inputs;
	for(int index=0; index < this->m_input_names.size(); ++index){
		tensorflow::TensorShape input_img_shape;
	   	input_img_shape.AddDim(this->m_input_shapes[index][0]);
	   	input_img_shape.AddDim(this->m_input_shapes[index][1]);
	   	input_img_shape.AddDim(this->m_input_shapes[index][2]);
	   	input_img_shape.AddDim(this->m_input_shapes[index][3]);

	   	tensorflow::Tensor tensor_data(tensorflow::DT_FLOAT, input_img_shape);
	   	float* tensor_data_ptr = tensor_data.flat<float>().data();
	   	int count = this->m_input_shapes[index][0]*this->m_input_shapes[index][1]*this->m_input_shapes[index][2]*this->m_input_shapes[index][3];
	   	memcpy(tensor_data_ptr, inputs[this->m_input_names[index]], sizeof(float)*count);
	   	tensor_inputs.push_back({this->m_input_names[index], tensor_data});
	}

	if(this->isDynamicOutputShape()){
		this->m_outputs.clear();
		for(int index=0; index<this->m_output_names.size(); ++index){
			tensorflow::TensorShape output_img_shape;
		   	output_img_shape.AddDim(this->m_output_shapes[index][0]);
		   	output_img_shape.AddDim(this->m_output_shapes[index][1]);
		   	output_img_shape.AddDim(this->m_output_shapes[index][2]);
		   	output_img_shape.AddDim(this->m_output_shapes[index][3]);

		   	tensorflow::Tensor tensor_data(tensorflow::DT_FLOAT, output_img_shape);
		   	this->m_outputs.push_back(tensor_data);
		}
	}

	// 2.step execute model
	std::vector<tensorflow::Tensor> tensor_outputs;
	tensorflow::Status status = this->m_session->Run(tensor_inputs, this->m_output_names, {}, &this->m_outputs);

	if (!status.ok()) {
	   EAGLEEYE_LOGE(status.ToString().c_str());
	   return false;
	} else {
	   EAGLEEYE_LOGD("Tensorflow Run session successfully");
	}

	// 3.step fiil output
	for(int index=0; index<this->m_output_names.size(); ++index){
		outputs[this->m_output_names[index]] = (unsigned char*)this->m_outputs[index].flat<float>().data();
	}
	return true;
}

bool ModelRun::initialize(){
	if(!this->isDynamicOutputShape()){
		this->m_outputs.clear();
		for(int index=0; index<this->m_output_names.size(); ++index){
			tensorflow::TensorShape output_img_shape;
		   	output_img_shape.AddDim(this->m_output_shapes[index][0]);
		   	output_img_shape.AddDim(this->m_output_shapes[index][1]);
		   	output_img_shape.AddDim(this->m_output_shapes[index][2]);
		   	output_img_shape.AddDim(this->m_output_shapes[index][3]);

		   	tensorflow::Tensor tensor_data(tensorflow::DT_FLOAT, output_img_shape);
		   	this->m_outputs.push_back(tensor_data);
		}
	}

	// Initialize a tensorflow session
	tensorflow::Status status = tensorflow::NewSession(tensorflow::SessionOptions(), &this->m_session);

    if(!status.ok()) {
    	EAGLEEYE_LOGE(status.ToString().c_str());
    	return false;
	}else{
		EAGLEEYE_LOGD("Tensorflow Session created successfully");
	}

	// Load the protobuf graph
	tensorflow::GraphDef graph_def;
	std::string graph_path = this->m_graph_path;
	status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), graph_path, &graph_def);
	if (!status.ok()) {
	   EAGLEEYE_LOGE(status.ToString().c_str());
	   return false;
	} else {
	   EAGLEEYE_LOGD("Load graph protobuf successfully");
	}

	// Add the graph to the session
	status = this->m_session->Create(graph_def);
	if (!status.ok()) {
	   EAGLEEYE_LOGE(status.ToString().c_str());
	   return false;
	} else {
	   EAGLEEYE_LOGD("Add graph to session successfully");
	}
	return true;
}
}

#endif