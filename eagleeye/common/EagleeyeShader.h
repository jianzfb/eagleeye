#ifndef _EAGLEEYE_SHADER_H_
#define _EAGLEEYE_SHADER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <memory>
#include <map>
#include <string>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>


namespace eagleeye
{
struct ShaderProgram{
    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;
};

class ShaderManager{
public:
    virtual ~ShaderManager();
    static std::shared_ptr<ShaderManager> getInstance();

    /**
     * @brief retrun shader program
     * 
     * @return GLuint 
     */
    GLuint create(std::string shader_name, const char* pvertex_shader_source=NULL, const char* pfrag_shader_source=NULL);

    /**
     * @brief release shader resource
     * 
     * @param shader_name 
     * @return true 
     * @return false 
     */
    bool release(std::string shader_name);

    /**
     * @brief get shader program info
     * 
     * @return ShaderProgram 
     */
    ShaderProgram program(std::string shader_name);

    /**
     * @brief clear shader
     * 
     */
    void clear();

private:
    ShaderManager();

    std::map<std::string, ShaderProgram> m_shader_map;
    static std::shared_ptr<ShaderManager> m_shader_manager;
};    
} // namespace eagleeye

#endif