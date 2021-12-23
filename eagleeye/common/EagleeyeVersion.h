#ifndef _EAGLEEYE_EAGLEEYEVERSION_H_
#define _EAGLEEYE_EAGLEEYEVERSION_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <string>
#include "eagleeye/common/EagleeyeStr.h"

namespace eagleeye{
#define EAGLEEYE_VERSION "1.0.0.0"

std::string getVersion(){
#ifdef EAGLEEYE_VERSION
    return EAGLEEYE_VERSION;
#else
    return "0.0.0.0";
#endif
}

bool minimumVersionRequired(std::string min_version){
    std::vector<std::string> core_version_terms = split(getVersion(), ".");
    std::vector<std::string> min_version_terms = split(min_version,".");

    bool is_ok = true;
    for(int i=0; i<4; ++i){
        if(tof<int>(core_version_terms[i]) < tof<int>(min_version_terms[i])){
            is_ok = false;
            break;
        }
    }

    return is_ok;
}
}
#endif