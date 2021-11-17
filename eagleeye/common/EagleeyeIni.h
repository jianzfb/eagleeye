#ifndef _EAGLEEYE_INI_H_
#define _EAGLEEYE_INI_H_
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <map>
 

//INI文件结点存储结构
namespace eagleeye{
class ININode{
public:
	ININode(std::string root, std::string key, std::string value){
		this->root = root;
		this->key = key;
		this->value = value;
	}
	std::string root;
	std::string key;
	std::string value;
};
 
//键值对结构体
class SubNode{
public:
	void InsertElement(std::string key, std::string value){
		sub_node.insert(std::pair<std::string, std::string>(key, value));
	}
	std::map<std::string, std::string> sub_node;
};
 
//INI文件操作类
class EagleeyeINI{
public:
	EagleeyeINI();
	~EagleeyeINI();
 
public:
	int readINI(std::string path);													//读取INI文件
	std::string getValue(std::string root, std::string key);									//由根结点和键获取值
	std::vector<ININode>::size_type getSize(){ return map_ini.size(); }				//获取INI文件的结点数
	std::vector<ININode>::size_type setValue(std::string root, std::string key, std::string value);	//设置根结点和键获取值
	int writeINI(std::string path);			//写入INI文件
	void clear(){ map_ini.clear(); }	    //清空
	void travel();						    //遍历打印INI文件
private:
	std::map<std::string, SubNode> map_ini;		//INI文件内容的存储变量
};

}
#endif