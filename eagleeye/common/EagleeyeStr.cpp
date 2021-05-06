#include "eagleeye/common/EagleeyeStr.h"
#include <stdio.h>
#include <time.h>

namespace eagleeye{
std::vector<std::string> split(const std::string& src, const std::string& separator){
    std::string str = src;
    std::string substring;
    std::string::size_type start = 0, index;

    std::vector<std::string> dest;
    
    do{
        index = str.find_first_of(separator,start);
        if (index != std::string::npos){    
            substring = str.substr(start,index-start);
            dest.push_back(substring);
            start = str.find_first_not_of(separator,index);
            if (start == std::string::npos) return dest;
        }
    }while(index != std::string::npos);
    
    //the last token
    substring = str.substr(start);
    dest.push_back(substring);
    return dest;
}

bool startswith(std::string str, std::string prefix){
    if(prefix == ""){
        return true;
    }

    if(str.substr(0,prefix.size()) == prefix){
        return true;
    }
    else{
        return false;
    }
}

bool endswith(std::string str, std::string suffix){
    if(suffix == ""){
        return true;
    }
    
    if(str.size() < suffix.size()){
        return false;
    }

    if(str.substr(str.size()-suffix.size(),str.size()) == suffix){
        return true;
    }
    else{
        return false;
    }
}

bool ispath(std::string str){
    if(str.front() == '.' || str.front() == '\\' || str.front() == '/'){
        return true;
    }

    return false;
}

std::string obfuscateString(const std::string &src,
                            const std::string &lookup_table) {
  std::string dest;
  dest.resize(src.size());
  for (size_t i = 0; i < src.size(); i++) {
    dest[i] = src[i] ^ lookup_table[i % lookup_table.size()];
  }
  return dest;
}

// ObfuscateString(ObfuscateString(str)) ==> str
std::string obfuscateString(const std::string &src) {
  // Keep consistent with obfuscation in python tools
  return obfuscateString(src, "EAGLEEYE-Compute-Engine");
}

std::string timeStamp(){
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::string strtime;
    if(ltm->tm_hour<=12){
        strtime = 
            formatString("%d-%d-%d AM %d.%d.%d", 
                1900+ltm->tm_year, ltm->tm_mon, ltm->tm_mday,ltm->tm_hour,ltm->tm_min,ltm->tm_sec);
    }
    else{
        strtime = 
            formatString("%d-%d-%d PM %d.%d.%d", 
                1900+ltm->tm_year, ltm->tm_mon, ltm->tm_mday,ltm->tm_hour-12,ltm->tm_min,ltm->tm_sec);

    }

    return strtime;
}

std::string pathJoin(std::vector<std::string> terms){
    std::string path="";
    if(terms.size() == 0){
        return path;
    }
    path = terms[0];
    char end_c = path.at(path.size() - 1);
    if(end_c == '/' || end_c == '\\'){
        path = path.substr(0, path.size() - 1);
    }

    for(int index=1; index<terms.size(); ++index){
        path = path + "/" + terms[index];

        char end_c = path.at(path.size() - 1);
        if(end_c == '/' || end_c == '\\'){
            path = path.substr(0, path.size() - 1);
        }
    }

    return path;
}
}