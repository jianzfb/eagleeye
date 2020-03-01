#ifndef _EAGLEEYE_FITTING_H_
#define _EAGLEEYE_FITTING_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"

namespace eagleeye{
enum FittingType{
    PolynomialFitting=0,
    LinearFitting=1,
};

class Fitting{
public:
    Fitting(FittingType fitting_type, int complexity=2, int max_length=100);
    virtual ~Fitting();

    /**
     * @brief fit history info
     * 
     */
    void smoothing();

    /**
     * @brief fit current info
     * 
     */
    void filtering(const Matrix<float> trajectory, Matrix<float>& filtered_trajectory, bool& new_seg);

    /**
     * @brief predict future info
     * 
     */
    void forecasting();

protected:
    bool isChangePoint(const Matrix<float> position, Matrix<float>& filtered_trajectory);
    bool updateHistory(const Matrix<float> position, bool flag);
    Matrix<float> fit(const Matrix<float> trajectory);
    Matrix<float> polynomialFit(const Matrix<float> trajectory);
    Matrix<float> linearFit(const Matrix<float> trajectory);

    FittingType m_fitting_type;
    int m_complexity;
    int m_increment;
    int m_current_time;
    Matrix<float> m_history_trajectory;
    float m_history_avg_error;
    float m_error_thres;
    bool m_start_at_new_seg;
};    
}
#endif