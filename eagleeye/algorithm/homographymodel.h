#ifndef _EAGLEEYE_HOMOGRAPHYMODEL_H_
#define _EAGLEEYE_HOMOGRAPHYMODEL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/algorithm/ransac.h"
#include "eagleeye/algorithm/llsp.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <memory>
#include "eagleeye/common/EagleeyeOpenCL.h"

namespace eagleeye{
class HomographyParam:public AbstractParameter{
public:
    HomographyParam(float x1,float y1,float z1,float x2,float y2,float z2){
        this->m_p1 = Matrix<float>(4,1);
        this->m_p2 = Matrix<float>(4,1);
        this->m_p1.at(0,0) = x1;
        this->m_p1.at(1,0) = y1;
        this->m_p1.at(2,0) = z1;

        this->m_p2.at(0,0) = x2;
        this->m_p2.at(1,0) = y2;
        this->m_p2.at(2,0) = z2;
    }

    Matrix<float> m_p1;
    Matrix<float> m_p2;
};

class HomographyModel:public AbstractModel<4,9>{
public:
    HomographyModel();
    ~HomographyModel();

    virtual float computeDistanceMeasure(std::shared_ptr<AbstractParameter> param, Matrix<float> model);
    virtual Matrix<float> build(const std::vector<std::shared_ptr<AbstractParameter>>& input_params);
    virtual Matrix<float> evaluate(const std::vector<std::shared_ptr<AbstractParameter>>& evaluate_params, Matrix<float> MultiH, float threshold);

    /**
     * @brief decompose homography matrix to translation, scale and rotation
     * 
     * @param model 
     * @param translation 
     * @param scale 
     * @param rotation 
     */
    static bool decompose(const Matrix<float> model, Matrix<float>& translation, Matrix<float>& scale, float& rotation);

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    EAGLEEYE_OPENCL_DECLARE_KERNEL_GROUP(homography);
#endif
};
}
#endif