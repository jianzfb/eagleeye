#ifndef _ANYMONITOR_H_
#define _ANYMONITOR_H_

#include "eagleeye/common/EagleeyeMacro.h"
#include <functional>
#include <memory>
#include <string>
#include <iostream>

namespace eagleeye
{
enum MonitorVarType
{
	EAGLEEYE_MONITOR_UNDEFINED = -1,
	EAGLEEYE_MONITOR_BOOL,
	EAGLEEYE_MONITOR_INT,
	EAGLEEYE_MONITOR_FLOAT,
	EAGLEEYE_MONITOR_STR
};

template<class T>
class MonitorVarTrait
{
public:
	static const MonitorVarType var_type = EAGLEEYE_MONITOR_UNDEFINED;
	static const int var_size = -1;
};
template<>
class MonitorVarTrait<bool>
{
public:
	static const MonitorVarType var_type = EAGLEEYE_MONITOR_BOOL;
	static const int var_size = sizeof(bool);
};
template<>
class MonitorVarTrait<int>
{
public:
	static const MonitorVarType var_type = EAGLEEYE_MONITOR_INT;
	static const int var_size = sizeof(int);
};
template<>
class MonitorVarTrait<float>
{
public:
	static const MonitorVarType var_type = EAGLEEYE_MONITOR_FLOAT;
	static const int var_size = sizeof(float);
};
template<>
class MonitorVarTrait<std::string>
{
public:
	static const MonitorVarType var_type = EAGLEEYE_MONITOR_STR;
	static const int var_size = -1;
};

enum MonitorCategory{
	MONITOR_DEFAULT = 0,
	MONITOR_MOUSE = 1
};

class AnyMonitor
{
public:
	AnyMonitor(MonitorVarType var_type, int var_size, char* monitor_text = "NON",char* min_v="", char* max_v="", char* description = "")
		:monitor_var_type(var_type),
		monitor_var_size(var_size),
		monitor_var_text(monitor_text),
		monitor_min_v(min_v),
		monitor_max_v(max_v),
		monitor_description(description){
			monitor_category = MONITOR_DEFAULT;
		}
	AnyMonitor(){
		monitor_var_type = EAGLEEYE_MONITOR_UNDEFINED;
		monitor_var_size = 0;
		monitor_var_text = "";
		monitor_min_v = 0;
		monitor_max_v = 0;
		monitor_description = "";
		monitor_category = MONITOR_DEFAULT;
	};
	virtual ~AnyMonitor(){};

	virtual void setVar(const void* var)=0;
	virtual void getVar(void* var)=0;

	/**
	 * @brief mouse 
	 * 
	 * @param mouse_x 
	 * @param mouse_y 
	 * @param mouse_action 
	 */
	virtual void run(int mouse_x, int mouse_y, int mouse_action) {}

	void setPrefixCallback(std::function<void()> func){
		this->prefix_func = func;
	}

	char* getMin(){
		return monitor_min_v;
	}
	char* getMax(){
		return monitor_max_v;
	}

	MonitorVarType monitor_var_type;
	int monitor_var_size;
	char* monitor_var_text;
	char* monitor_description;
	char* monitor_min_v;
	char* monitor_max_v;	

	std::function<void()> prefix_func;
	MonitorCategory monitor_category;
};

template<class HostT,class T>
class VarMonitor:public AnyMonitor
{
public:
	typedef void(HostT::*SetT)(const T);
	typedef void(HostT::*GetT)(T&);

	VarMonitor(HostT* host,SetT set_fun,GetT get_fun,char* text,char* min_v, char* max_v, char* def_text="")
		:AnyMonitor(MonitorVarTrait<T>::var_type, MonitorVarTrait<T>::var_size, text,min_v, max_v,def_text),
		monitor_host(host),
		set_var_fun(set_fun),
		get_var_fun(get_fun){}
	virtual ~VarMonitor(){};

	virtual void setVar(const void* var)
	{	
		if(this->prefix_func){
			this->prefix_func();
		}

		const T* v=static_cast<const T*>(var);
		(monitor_host->*set_var_fun)(*v);
	}
	virtual void getVar(void* var)
	{
		T* v=(T*)var;
		(monitor_host->*get_var_fun)(*v);
	}
	
	SetT set_var_fun;
	GetT get_var_fun;

	HostT* monitor_host;
};

class MouseMonitor:public AnyMonitor{
public:
	MouseMonitor(std::function<void(int,int,int)> func){
		m_mouse_func = func;
		this->monitor_category = MONITOR_MOUSE;
	}
	virtual ~MouseMonitor(){};

	virtual void run(int mouse_x, int mouse_y, int mouse_action){
		if(this->prefix_func){
			this->prefix_func();
		}

		m_mouse_func(mouse_x, mouse_y, mouse_action);
	}
	virtual void setVar(const void* var){};
	virtual void getVar(void* var){};


	std::function<void(int,int,int)> m_mouse_func;
};

#define EAGLEEYE_MONITOR_VAR(VarT,set_fun,get_fun,name,min_v,max_v) \
	this->m_unit_monitor_pool.push_back(new VarMonitor<Self,VarT>(this,&Self::set_fun,&Self::get_fun,name,min_v,max_v));

#define EAGLEEYE_MONITOR_VAR_EXT(VarT,set_fun,get_fun,name,min_v,max_v,description) \
	this->m_unit_monitor_pool.push_back(new VarMonitor<Self,VarT>(this,&Self::set_fun,&Self::get_fun,name,min_v,max_v,description));

#define EAGLEEYE_MOUSE_MONINTOR(func) \
	this->m_unit_monitor_pool.push_back(new MouseMonitor(func)); \
	AnyPipeline::getRenderContext()->registerListeningMouse(this->m_unit_monitor_pool[this->m_unit_monitor_pool.size()-1]);
}

#endif
