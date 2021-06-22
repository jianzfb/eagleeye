#ifndef _EAGLEEYE_ANYPIPELINE_H_
#define _EAGLEEYE_ANYPIPELINE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderContext.h"
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
     * @brief Check pipeline exist
     */ 
    static bool isExist(const char* pipeline_name);

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
     * @brief register signal in background
     */ 
    static bool registerSignal(const char* pipeline_name, const char* node);

    /**
     * @brief check signal matching
     */ 
    static bool checkSignalMatching(const char* pipeline_name, const char* node, const char* register_node);

    /**
     * @brief release pipelin resource
     * 
     * @param pipeline_name 
     */
    static void releasePipeline(const char* pipeline_name);

    /**
     * @brief set plugin root
     */ 
    static void setPluginRoot(const char* root);

    /**
     * @brief get plugin root
     */ 
    static std::string getPluginRoot(){return AnyPipeline::m_plugin_root;};

    /**
     * @brief approve pipeline
     */
    void approve(bool approve=true);

    /**
     * @brief set dependent pipelines
     */ 
    void setDependentPipelines(std::string info);

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
     * @brief add dependent node manually (only useful for output nodes)
     */
    void dependent(const char* node, const char* dependent_node);

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
    void initialize(const char* configure_folder, std::function<bool()> init_func=nullptr, bool ignore=false);

    /**
     * @brief check is initialize
     */ 
    bool isInitialized();

    /**
     * @brief create render context
     */ 
    static void onRenderSurfaceCreate();

    /**
     * @brief surface change (render)
     */ 
    static void onRenderSurfaceChange(int width, int height);

    /*
    * @brief mouse event
    */
    static void onRenderSurfaceMouse(int mouse_x, int mouse_y, int mouse_flag);

    /**
     * @brief surface w (render)
     */ 
    static int getRenderSurfaceW();

    /**
     * @brief surface h (render)
     */ 
    static int getRenderSurfaceH();

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
    bool start(const char* node_name=NULL, const char* ignore_prefix=NULL);

    /**
     * @brief run pipline
     * 
     * @param node_name 
     */
    void wait(const char* node_name=NULL, const char* ignore_prefix=NULL);

    /**
     * @brief refresh all render node
     */ 
    void refresh();

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
     * @param data_rotation
     * @param data_type
     */
    void setInput(const char* node_name, void* data, const int* data_size, const int data_dims, const int data_rotation, const int data_type);
    
    /**
     * @brief Set the Pipeline Input
     * 
     * @param node_name
     * @param data
     * @param meta
     */ 
    void setInput(const char* node_name, void* data, MetaData meta);

    /**
     * @brief Set the Pipeline Input (from register node)
     */ 
    void setInput(const char* node_name, std::string from_register_node);

    /**
     * @brief Set the Pipeline Input
     */ 
    void setInput(const char* node_name, std::string from_pipeline_name, std::string from_node_name);

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
     * @brief Get the Node object
     * 
     * @param node_name 
     * @return AnyNode* 
     */
    AnyNode* getNode(std::string node_name);

    /**
     * @brief branch from 3rd pipleline
     * 
     * @param pipeline_name 
     * @param node 
     * @param port 
     * @return AnyNode* 
     */
    AnyNode* branch(std::string pipeline_name, std::string node, int port, bool inplace=false);

    /**
     * @brief insert/restore placeholder
     * 
     * @param node_name 
     * @param port 
     */
    void replaceAt(std::string node_name, int port);
    void restoreAt(std::string node_name, int port);

    /**
     * @brief Destroy the Any Pipeline object
     * 
     */
    virtual ~AnyPipeline();

    /**
     * @brief placeholder
     * 
     * @return AnyNode* 
     */
    AnyNode* placeholder(SignalCategory category, EagleeyeType type);

    /**
     * @brief get resource folder
     */
    std::string resourceFolder();
    
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
    std::vector<std::string> m_replace_nodes;
    std::vector<int> m_replace_ports;
    std::vector<AnyNode*> m_using_placeholders;
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> m_dependent_pipelines; 
    
    std::map<std::string, std::vector<std::string>> m_dependent_nodes;
    std::vector<std::string> m_order_nodes;

    bool m_is_initialize;
    bool m_is_approved;
    INITIALIZE_PLUGIN_PIPELINE_FUNC m_init_func;
    static std::map<std::string, std::shared_ptr<AnyPipeline>> m_pipeline_map;
    static std::map<std::string, std::string> m_pipeline_version;
    static std::map<std::string, std::string> m_pipeline_signature;    
    static std::string m_plugin_root;

    static std::shared_ptr<RenderContext> m_render_context;
};
}
#endif