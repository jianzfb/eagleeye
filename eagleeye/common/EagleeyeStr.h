#ifndef _EAGLEEYESTR_H_
#define _EAGLEEYESTR_H_
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

namespace eagleeye{
template<typename T>	
T tof(std::string s){
	std::stringstream ss(s);
	T f = 0;
	ss>>f;
	return f;
}

template<typename T>
std::string tos(T t){
	std::stringstream ss;
	ss<<t;
	return ss.str();
}

/**
 * 
 */
std::vector<std::string> split(const std::string& src, const std::string& separator);

bool startswith(std::string str, std::string prefix);
bool endswith(std::string str, std::string suffix);
bool ispath(std::string str);
}
#endif