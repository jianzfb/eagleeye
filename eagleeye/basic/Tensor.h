#ifndef _EAGLEEYE_TENSOR_H_
#define _EAGLEEYE_TENSOR_H_
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
#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeRuntime.h"


namespace eagleeye{
class Tensor:public Blob{
public:
    /**
     * @brief create tensor
     */ 
    Tensor(std::vector<int64_t> shape, 
            EagleeyeType data_type, 
            DataFormat data_format,
            MemoryType memory_type,
            std::vector<int64_t> image_shape=std::vector<int64_t>(),
            Aligned aligned=Aligned(64));

    /**
     * @brief null tensor
     */ 
    Tensor();
    
    /**
     * @brief destructor
     */ 
    virtual ~Tensor();

    /**
     * @brief get element number
     */ 
    int64_t size() const;

    /**
     * @brief get dimension number
     */ 
	int64_t ndim() const;

    /**
     * @brief get index dimension size
     */ 
	int64_t dim(int index) const;

    /**
     * @brief get tensor format
     */ 
    DataFormat format() const;

    /**
     * @brief get tensor shape
     */ 
    std::vector<int64_t> shape() const;

protected:
    DataFormat m_format;
};
}
#endif