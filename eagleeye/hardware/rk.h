#ifndef _EAGLEEYE_RK_H_
#define _EAGLEEYE_RK_H_

#include <string.h>
#include <vector>
#include "eagleeye/basic/Matrix.h"
namespace eagleeye{


class RKH264Decoder{
public:
    RKH264Decoder();
    virtual ~RKH264Decoder();

    int decode(uint8_t* package_data, int package_size, std::vector<Matrix<Array<unsigned char, 3>>>& image_list);

private:
    int initial();
    void destroy();

    void* m_mpp_ctx;
    void* m_mpp_api;
    void* m_frm_grp;

    uint8_t* m_cache_buf;
    int m_cache_offset;
};
}
#endif // RKH264Decoder.h
