# include "eagleeye/common/EagleeyeMacro.h"
namespace eagleeye{
template<typename T>    
T clip(T val, T min, T max){
    val = val > min ? val : min;
    val = val < max ? val : max;
    return val;
}

#include "eagleeye/common/EagleeyeUtil.hpp"
}