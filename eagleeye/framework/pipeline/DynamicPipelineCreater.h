#ifndef _EAGLEEYE_DYNAMICPIPELINECREATER_
#define _EAGLEEYE_DYNAMICPIPELINECREATER_
#include <typeinfo>
#include <cxxabi.h>
#include <functional>
#include <string>
#include <iostream>
#include <memory>
#include <unordered_map>
#include "eagleeye/framework/pipeline/AnyPipeline.h"


namespace eagleeye{
template<typename ...Targs>
class PipelineFactory{
public:
    static PipelineFactory* Instance(){
        if (nullptr == m_pOpFactory){
            m_pOpFactory = new PipelineFactory();
        }
        return(m_pOpFactory);
    }

    virtual ~PipelineFactory(){};

    // 将“实例创建方法（DynamicCreator的CreateObject方法）”注册到OpFactory，注册的同时赋予这个方法一个名字“类名”，后续可以通过“类名”获得该类的“实例创建方法”。这个实例创建方法实质上是个函数指针，在C++11里std::function的可读性比函数指针更好，所以用了std::function。
    bool Regist(const std::string& strTypeName, std::function<AnyPipeline*(Targs&&... args)> pFunc);

    // 传入“类名”和参数创建类实例，方法内部通过“类名”从m_mapCreateFunction获得了对应的“实例创建方法（DynamicCreator的CreateObject方法）”完成实例创建操作。
    AnyPipeline* Create(const std::string& strTypeName, Targs&&... args);

private:
    PipelineFactory(){};
    static PipelineFactory<Targs...>* m_pOpFactory;
    std::unordered_map<std::string, std::function<AnyPipeline*(Targs&&...)> > m_mapCreateFunction;
};

template<typename ...Targs>
PipelineFactory<Targs...>* PipelineFactory<Targs...>::m_pOpFactory = nullptr;

template<typename ...Targs>
bool PipelineFactory<Targs...>::Regist(const std::string& strTypeName, std::function<AnyPipeline*(Targs&&... args)> pFunc){
    if (nullptr == pFunc){
        return (false);
    }
    bool bReg = m_mapCreateFunction.insert(
                    std::make_pair(strTypeName, pFunc)).second;
    return (bReg);
}

template<typename ...Targs>
AnyPipeline* PipelineFactory<Targs...>::Create(const std::string& strTypeName, Targs&&... args){
    typename std::unordered_map<std::string, std::function<AnyPipeline*(Targs&&...)>>::iterator ii,jj(m_mapCreateFunction.end());

    auto iter = m_mapCreateFunction.find(std::string("eagleeye::")+strTypeName);
    if (iter == m_mapCreateFunction.end()){
        
        return (nullptr);
    }
    else{
        return (iter->second(std::forward<Targs>(args)...));
    }
}

template<typename T, typename...Targs>
class DynamicPipelineCreator{
public:
    struct Register{
        Register(){
            char* szDemangleName = nullptr;
            std::string strTypeName;
#ifdef __GNUC__
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#else
            // 注意：这里不同编译器typeid(T).name()返回的字符串不一样，需要针对编译器写对应的实现
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
            if (nullptr != szDemangleName)
            {
                strTypeName = szDemangleName;
                free(szDemangleName);
            }
            PipelineFactory<Targs...>::Instance()->Regist(strTypeName, CreateObject);
        }
        inline void do_nothing()const { };
    };

    DynamicPipelineCreator(){
        m_oRegister.do_nothing();   // 这里的函数调用虽无实际内容，却是在调用动态创建函数前完成m_oRegister实例创建的关键
    }
    virtual ~DynamicPipelineCreator(){};

    // 动态创建实例的方法，所有Base实例均通过此方法创建。这是个模板方法，实际上每个Base的派生类都对应了自己的CreateObject方法。
    static T* CreateObject(Targs&&... args){
        T* pT = nullptr;
        try{
            pT = new T(std::forward<Targs>(args)...);
        }
        catch(std::bad_alloc& e){
            return(nullptr);
        }
        return(pT);
    }

private:
    static Register m_oRegister;
};

template<typename T, typename ...Targs>
typename DynamicPipelineCreator<T, Targs...>::Register DynamicPipelineCreator<T, Targs...>::m_oRegister;


template<typename ...Targs>
AnyPipeline* CreatePipeline(const std::string& strTypeName, Targs&&... args){
    AnyPipeline* p = PipelineFactory<Targs...>::Instance()->Create(strTypeName, std::forward<Targs>(args)...);
    return(p);
}
}
#endif