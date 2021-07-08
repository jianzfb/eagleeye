#include "eagleeye/common/EagleeyeShader.h"
#include "eagleeye/render/GLUtils.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye
{
std::shared_ptr<ShaderManager> ShaderManager::m_shader_manager;    
ShaderManager::ShaderManager(){

}   

ShaderManager::~ShaderManager(){
    std::map<std::string, ShaderProgram>::iterator iter, iend(m_shader_map.end());
    for(iter = m_shader_map.begin(); iter != iend; ++iter){
        GLUtils::DeleteProgram(iter->second.program);
    }
    m_shader_map.clear();
}   


std::shared_ptr<ShaderManager> ShaderManager::getInstance(){
    if(m_shader_manager.get() == NULL){
        m_shader_manager = 
            std::shared_ptr<ShaderManager>(new ShaderManager(), 
                                [](ShaderManager* ptr){delete ptr;});
    }

    return m_shader_manager;
}

GLuint ShaderManager::create(std::string shader_name, const char* pvertex_shader_source, const char* pfrag_shader_source){
    if(m_shader_map.find(shader_name) == m_shader_map.end()){
        // 创建program
        if(pvertex_shader_source != NULL && pfrag_shader_source != NULL){
            GLuint vertext_shader;
            GLuint fragment_shader;
            GLuint program = GLUtils::CreateProgram(pvertex_shader_source, 
                                    pfrag_shader_source,
                                    vertext_shader,
									fragment_shader);

            if (!program){
                GLUtils::CheckGLError("Create Program");
                EAGLEEYE_LOGE("CreateProgram Could not create program.");
                return -1;
            }

            ShaderProgram sp;
            sp.program = program;
            sp.vertex_shader = vertext_shader;
            sp.fragment_shader = fragment_shader;
            m_shader_map[shader_name] = sp;
        }
        else{
            // 从文件加载
            EAGLEEYE_LOGE("TODO, would support compile shader from file");
            return -1;
        }
    }

    return m_shader_map[shader_name].program;
}

bool ShaderManager::release(std::string shader_name){
    if(m_shader_map.find(shader_name) == m_shader_map.end()){
        return false;
    }

    GLUtils::DeleteProgram(m_shader_map[shader_name].program);
    m_shader_map.erase(m_shader_map.find(shader_name));
    return true;
}

ShaderProgram ShaderManager::program(std::string shader_name){
    if(m_shader_map.find(shader_name) == m_shader_map.end()){
        ShaderProgram sp;
        sp.program = -1;
        sp.vertex_shader = -1;
        sp.fragment_shader = -1;
        return sp;
    }

    return m_shader_map[shader_name];
}

void ShaderManager::clear(){
    std::map<std::string, ShaderProgram>::iterator iter, iend(m_shader_map.end());
    for(iter = m_shader_map.begin(); iter != iend; ++iter){
        GLUtils::DeleteProgram(iter->second.program);
    }
    m_shader_map.clear();
}
} // namespace eagleeye
