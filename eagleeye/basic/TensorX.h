#ifndef _EAGLEEYE_TENSORX_H_
#define _EAGLEEYE_TENSORX_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <assert.h>
#include <stdio.h>
#include <ostream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <numeric>
#include <functional>
#include <iostream>
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeRuntime.h"


namespace eagleeye{
class TensorX:public Blob{
public:
    TensorX(std::vector<int64_t> shape, 
            EagleeyeType data_type, 
            DataFormat data_format,
            MemoryType memory_type,
            std::vector<int64_t> image_shape=std::vector<int64_t>(),
            std::string buffer_to_image_transform="",
            std::string image_to_buffer_transform="",
            Aligned aligned=Aligned(64));
    TensorX();
    virtual ~TensorX();

    int64_t size() const;
	int64_t ndim() const;
	int64_t dim(int index) const;
    DataFormat format() const;
    std::vector<int64_t> shape() const;

protected:
    DataFormat m_format;
};
}
#endif