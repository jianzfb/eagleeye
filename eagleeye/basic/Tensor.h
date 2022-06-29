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
#include "eagleeye/basic/Dim.h"
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
            Aligned aligned=Aligned(64),
            std::vector<int64_t> image_shape=std::vector<int64_t>());

    Tensor(std::vector<int64_t> shape, 
            EagleeyeType data_type, 
            DataFormat data_format, 
            void* data);

    /**
     * @brief null tensor
     */ 
    Tensor();
    
    /**
     * @brief destructor
     */ 
    virtual ~Tensor();

    /**
     * @brief get tensor format
     */ 
    DataFormat format() const;

    /**
     * @brief clone blob
     * 
     */
    virtual Tensor clone();

    /**
     * @brief copy from t
     * 
     * @param t 
     */
    void copy(const Tensor& t);

    /**
     * @brief 
     * 
     * @param start 
     * @param end 
     * @return Tensor 
     */
    Tensor slice(int64_t start, int64_t end) const;

    /**
     * @brief squeeze shape in axis
     * 
     * @param axis 
     * @return Tensor 
     */
    Tensor squeeze(int64_t axis);

    /**
     * @brief unsqueeze shape in axis
     * 
     * @param axis 
     * @return Tensor 
     */
    Tensor unsqueeze(int64_t axis);

protected:
    DataFormat m_format;
};
}
#endif