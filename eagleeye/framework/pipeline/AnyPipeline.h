#ifndef _EAGLEEYE_ANYPIPELINE_H_
#define _EAGLEEYE_ANYPIPELINE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <memory>
#include <map>

namespace eagleeye{

typedef void (*INITIALIZE_PLUGIN_PIPELINE_FUNC)();

class EAGLEEYE_API AnyPipeline{
public:
    /**
     * @brief Get the Instance object
     * 
     * @return AnyPipeline* 
     */
    static AnyPipeline* getInstance(const char* pipeline_name);

    /**
     * @brief Get the Registed Pipeline Names object
     * 
     * @param pipeline_names 
     */
    static void getRegistedPipelines(std::vector<std::string>& pipeline_names);

    /**
     * @brief Create a Pipeline object
     * 
     * @param pipeline_name 
     */
    static bool registerPipeline(const char* pipeline_name, const char* version, const char* key);

    /**
     * @brief release pipelin resource
     * 
     * @param pipeline_name 
     */
    static void releasePipeline(const char* pipeline_name);

    /**
     * @brief Set the Pipeline Name 
     * 
     * @param name 
     */
    void setPipelineName(const char* name);

    /**
     * @brief Get the Pipeline Name
     * 
     * @param name 
     */
    void getPipelineName(char* name);

    /**
     * @brief Get the Pipeline Version object
     * 
     * @return const char* 
     */
    const char* getPipelineVersion();

    /**
     * @brief get node in pipeline
     * 
     * @param node_name 
     * @return AnyNode* 
     */
    AnyNode* get(const char* node_name);

    /**
     * @brief add node into pipeline
     * 
     * @param node 
     * @param node_name 
     * @return true 
     * @return false 
     */
    bool add(AnyNode* node, const char* node_name);

    /**
     * @brief link node a to node b
     * 
     * @param node_a 
     * @param port_a 
     * @param node_b 
     * @param port_b 
     */
    void bind(const char* node_a, int port_a, const char* node_b, int port_b);

    /**
     * @brief add feadback action
     * 
     * @param trigger_node 
     * @param trigger_node_state 
     * @param response_action 
     */
    void addFeadbackRule(const char* trigger_node, int trigger_node_state, const char* response_node, const char* response_action);

    /**
     * @brief load/save pipeline from file
     * 
     * @param config_file 
     */
    void loadConfigure(std::string config_file);
    void saveConfigure(std::string config_file);

    /**
     * @brief analyze pieline structure
     * 
     */
    void initialize(const char* configure_folder);

    /**
     * @brief Set the Ini Func object
     * 
     */
    void setInitFunc(INITIALIZE_PLUGIN_PIPELINE_FUNC func);

    /**
     * @brief run pipline
     * 
     * @param node_name 
     */
    bool start(const char* node_name=NULL);

    /**
     * @brief whether pipeline is ready to run
     * 
     * @param is_ready 
     */
    void isReady(int& is_ready);

    /**
     * @brief reset pipeline
     * 
     */
    void reset();

    /**
     * @brief Set the Node Parameter in pipeline
     * 
     * @param node_name 
     * @param param_name 
     * @param value 
     */
    void setParameter(const char* node_name, const char* param_name, const void* value);
    
    /**
     * @brief Get the Node Parameter in pipeline
     * 
     * @param node_name 
     * @param param_name 
     * @param value 
     */
    void getParameter(const char* node_name, const char* param_name, void* value);
    
    /**
     * @brief Set the Pipeline Input 
     * 
     * @param node_name 
     * @param data 
     * @param data_size 
     * @param data_dims 
     */
    void setInput(const char* node_name, void* data, const int* data_size, const int data_dims);
    
    /**
     * @brief Get the Node Output object
     * 
     * @param node_name 
     * @param data 
     * @param data_size 
     * @param data_dims 
     * @param data_type 
     */
    void getNodeOutput(const char* node_name, void*& data, int* data_size, int& data_dims, int& data_type);

    /**
     * @brief Get the Pipeline Output
     * 
     * @param node_name 
     * @param data 
     * @param data_size 
     * @param data_dims 
     * @param data_type
     */
    void getOutput(const char* node_name, void*& data, int* data_size, int& data_dims, int& data_type);

    /**
     * @brief Get the Pipeline Inputs
     * 
     * @param input_nodes 
     * @param input_types 
     * @param input_sources 
     */
    void getPipelineInputs(std::vector<std::string>& input_nodes, 
                           std::vector<std::string>& input_types, 
                           std::vector<std::string>& input_sources);
    
    /**
     * @brief Get the Pipeline Outputs object
     * 
     * @param output_nodes 
     * @param output_types 
     * @param output_targets 
     */
    void getPipelineOutputs(std::vector<std::string>& output_nodes,
                            std::vector<std::string>& output_types,
                            std::vector<std::string>& output_targets);
    
    /**
     * @brief Get the Pipeline Monitors object
     * 
     * @param monitor_names 
     * @param monitor_types 
     */
    void getPipelineMonitors(std::vector<std::string>& monitor_names,
                             std::vector<int>& monitor_types,
                             std::vector<std::string>& monitor_range);

    /**
     * @brief Destroy the Any Pipeline object
     * 
     */
    virtual ~AnyPipeline();

protected:
    /**
     * @brief Construct a new Any Pipeline object
     * 
     * @param pipeline_name 
     */
    AnyPipeline(const char* pipeline_name="");
    
private:
    std::map<std::string, AnyMonitor*> m_monitor_params;
    std::map<std::string, AnyNode*> m_input_nodes;
    std::map<std::string, AnyNode*> m_output_nodes;

    std::string m_name;
    std::string m_version;
    std::string m_signature;
    std::map<std::string, AnyNode*> m_nodes;
    std::map<std::string, bool> m_is_output_nodes;

    bool m_is_initialize;
    INITIALIZE_PLUGIN_PIPELINE_FUNC m_init_func;
    static std::map<std::string, std::shared_ptr<AnyPipeline>> m_pipeline_map;
    static std::map<std::string, std::string> m_pipeline_version;
    static std::map<std::string, std::string> m_pipeline_signature;    
};
}
#endif