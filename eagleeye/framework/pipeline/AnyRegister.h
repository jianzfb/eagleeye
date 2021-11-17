#ifndef _ANYREGISTER_H_
#define _ANYREGISTER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include <map>
#include <string>
#include <memory>

namespace eagleeye{
class AnyRegister{
public:
    /**
     * @brief get handler
     */ 
    static std::shared_ptr<AnyRegister> get();

    /**
     * @brief register signal in center
     */ 
    bool registerSignal(std::string name, AnySignal* sig);

    /**
     * @brief get signal from register center
     */ 
    AnySignal* signal(std::string name);

    /**
     * @brief clear signal
     */ 
    bool clear(std::string name="");

private:
    AnyRegister();
    virtual ~AnyRegister();

    static std::shared_ptr<AnyRegister> m_register;
    std::map<std::string, AnySignal*> m_register_map;
};    
}
#endif