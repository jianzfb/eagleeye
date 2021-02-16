#ifndef _EAGLEEYESTR_H_
#define _EAGLEEYESTR_H_
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>

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
 * @brief split src by separator
 */
std::vector<std::string> split(const std::string& src, const std::string& separator);

/**
 * @brief check whether str starts with prefix
 */ 
bool startswith(std::string str, std::string prefix);

/**
 * @brief check whether str ends with prefix
 */ 
bool endswith(std::string str, std::string suffix);

/**
 * @brief check str is path 
 */ 
bool ispath(std::string str);


inline void MakeStringInternal(std::stringstream & /*ss*/) {}

template <typename T>
inline void MakeStringInternal(std::stringstream &ss, const T &t) {
  ss << t;
}

template <typename T, typename... Args>
inline void MakeStringInternal(std::stringstream &ss,
                               const T &t,
                               const Args &... args) {
  MakeStringInternal(ss, t);
  MakeStringInternal(ss, args...);
}

class StringFormatter {
 public:
  static std::string Table(const std::string &title,
                           const std::vector<std::string> &header,
                           const std::vector<std::vector<std::string>> &data);
};

template <typename... Args>
std::string makeString(const Args &... args) {
  std::stringstream ss;
  MakeStringInternal(ss, args...);
  return ss.str();
}

template <typename T>
std::string makeListString(const T *args, size_t size) {
  std::stringstream ss;
  ss << "[";
  for (size_t i = 0; i < size; ++i) {
    ss << args[i];
    if (i < size - 1) {
      ss << ", ";
    }
  }
  ss << "]";
  return ss.str();
}

/**
 * @brief conver vector<T> to string
 */ 
template <typename T>
std::string makeString(const std::vector<T> &args) {
  return makeListString(args.data(), args.size());
}

// Specializations for already-a-string types.
template <>
inline std::string makeString(const std::string &str) {
  return str;
}

/**
 * @brief convert char* to std::string
 */ 
inline std::string makeString(const char *c_str) { return std::string(c_str); }


/**
 * @brief convert set to string
 */ 
inline std::string makeString(std::set<std::string> args){
    std::string str;
    for (auto &option : args) {
        str += " " + option;
    }

    return str;
}

/**
 * @brief to lower string
 */ 
inline std::string toLower(const std::string &src) {
  std::string dest(src);
  std::transform(src.begin(), src.end(), dest.begin(), ::tolower);
  return dest;
}

/**
 * @brief to upper string
 */ 
inline std::string toUpper(const std::string &src) {
  std::string dest(src);
  std::transform(src.begin(), src.end(), dest.begin(), ::toupper);
  return dest;
}


std::string obfuscateString(const std::string &src,
                            const std::string &lookup_table);
std::string obfuscateString(const std::string &src);                           

}
#endif