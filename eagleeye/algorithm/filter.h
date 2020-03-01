#ifndef _EAGLEEYE_FILTER_H_
#define _EAGLEEYE_FILTER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"
namespace eagleeye{
class Filter{
public:
    Filter();
    virtual ~Filter();

    /**
     * @brief Set the Sigma Color
     * 
     * @param sigmai 
     */
    void setSigmaI(float sigmai);

    /**
     * @brief Set the Sigma Spatial object
     * 
     * @param sigmas 
     */
    void setSigmaS(float sigmas);


    void crossFastBF(Matrix<float> data, Matrix<float> reference, Matrix<float>& filted_data);
    void fastBF(Matrix<float> data, Matrix<float>& filted_data);

protected:
    /**
     * @brief fast bilateral filter
     * reference: A Fast Approximation of the Bilateral Filter using a Signal Processing Approach
     * 
     * @param src 
     * @param dst 
     * @param sigma_color 
     * @param sigma_space 
     */
    void fastLBF(Matrix<float> src, 
                         Matrix<float> reference,
                         Matrix<float>& dst,
                         float sigma_color, 
                         float sigma_space);

private:
    float m_sigmaI;
    float m_sigmaS;
};

}
#endif