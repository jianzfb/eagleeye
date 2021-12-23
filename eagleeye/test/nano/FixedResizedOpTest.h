#ifndef _EAGLEEYE_FIXEDRESIZEDOPTEST_H_
#define _EAGLEEYE_FIXEDRESIZEDOPTEST_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/FixedResizeOp.h"
#include <iostream>

namespace eagleeye{
namespace nano{
void test_FixedResizedOp_on_CPU(){
    Tensor<short> t(std::vector<int64_t>{1,3,20,20});
    short* t_ptr = t.dataptr();
    for(int i=-10; i<10; ++i){
        int count = 0;
        for(int j=-10; j<10; ++j){
            int y = i+10;
            int x = j+10;
            int c = 0;
            t_ptr[c*20*20 + y*20+x] = (count % 255 - 127)/float(127) * 32768;
            count += 1;

            c = 1;
            t_ptr[c*20*20 + y*20+x] = (count % 255 - 127)/float(127) * 32768;
            count += 1;

            c = 2;
            t_ptr[c*20*20 + y*20+x] = (count % 255 - 127)/float(127) * 32768;
            count += 1;
        }
    }

    FixedResizeOp* resized_op = new FixedResizeOp(1,1,"resize");
    Tensor<short> output(std::vector<int64_t>{1,3,40,40});
    std::vector<Tensor<short>> input_list={t};
    std::vector<Tensor<short>> output_list={output};

    resized_op->foward_on_cpu(output_list, input_list);

}    
}    
}
#endif