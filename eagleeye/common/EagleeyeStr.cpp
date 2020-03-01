#include "eagleeye/common/EagleeyeStr.h"

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
    if(str.substr(0,prefix.size()) == prefix){
        return true;
    }
    else{
        return false;
    }
}

bool endswith(std::string str, std::string suffix){
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
}