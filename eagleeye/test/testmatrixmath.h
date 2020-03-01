#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"


namespace eagleeye{
inline void testMatrixMath(){
    EAGLEEYE_LOGI("test WHERE (where >= 3)");
    Matrix<float> data(3,4);
    for(int i=0; i<3; ++i){
        for(int j=0; j<4; ++j){
            data.at(i,j) = i*4+j;
        }
    }
    EAGLEEYE_LOGI("INPUT DATA");
    std::cout<<data;
    Matrix<int> indices = where<gt<float>>(data, 3);
    EAGLEEYE_LOGI("OUTPUT ");
    std::cout<<indices;
}    
}