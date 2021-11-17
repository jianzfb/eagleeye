#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"

namespace eagleeye{
inline void testMatrix(){
    Matrix<float> left(3,4);
    for(int i=0; i<3; ++i){
        for(int j=0; j<4; ++j){
            left.at(i,j) = i*4+j;
        }
    }
    EAGLEEYE_LOGI("INPUT left");
    std::cout<<left;
    Matrix<float> right(1,4);
    for(int i=0; i<3; ++i){
        for(int j=0; j<4; ++j){
            right.at(i,j) = -(i*4+j);
        }
    }
    EAGLEEYE_LOGI("INPUT right");
    std::cout<<right;

    EAGLEEYE_LOGI("test ADD BROADCAST (operator +)");
    Matrix<float> add_result = left+right;
    std::cout<<add_result;

    EAGLEEYE_LOGI("test ADD BROADCAST (add_)");
    Matrix<float> add_2_result = left.clone();
    add_2_result.add_(right);
    std::cout<<add_2_result;

    EAGLEEYE_LOGI("test SUB BROADCAST (operator -)");
    Matrix<float> sub_result = left-right;
    std::cout<<sub_result;

    EAGLEEYE_LOGI("test SUB BROADCAST (sub_)");
    Matrix<float> sub_2_result = left.clone();
    sub_2_result.sub_(right);
    std::cout<<sub_2_result;

    EAGLEEYE_LOGI("test MUL BROADCAST (mul)");
    Matrix<float> mul_result = left.mul(right);
    std::cout<<mul_result;
    
    EAGLEEYE_LOGI("test MUL BROADCAST (mul_)");
    Matrix<float> mul_2_result = left.clone();
    mul_2_result.mul_(right);
    std::cout<<mul_2_result;

    EAGLEEYE_LOGI("test DIV BROADCAST (div)");
    Matrix<float> div_result = left.div(right);
    std::cout<<div_result;

    EAGLEEYE_LOGI("test DIV BROADCAST (div_)");
    Matrix<float> div_2_result = left.clone();
    div_2_result.div_(right);
    std::cout<<div_2_result;

    EAGLEEYE_LOGI("test GATHERND (gatherND)");
    Matrix<int> indices(2,2);
    indices.at(0,0) = 0;
    indices.at(0,1) = 2;
    indices.at(1,0) = 1;
    indices.at(1,1) = 0;
    Matrix<float> gathernd_result = left.gatherND(indices);
    std::cout<<gathernd_result;

    EAGLEEYE_LOGI("test GATHER (gather)");
    Matrix<int> index(1,2);
    index.at(0) = 0;
    index.at(1) = 1;
    Matrix<float> gather_result = left.gather(index);
    std::cout<<gather_result;
}
}
