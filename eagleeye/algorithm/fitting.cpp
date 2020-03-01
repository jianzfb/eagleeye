#include "eagleeye/algorithm/fitting.h"
#include <math.h>
namespace eagleeye{
Fitting::Fitting(FittingType fitting_type, int complexity, int max_length){
    this->m_fitting_type = fitting_type;
    this->m_complexity = complexity;
    this->m_current_time = 0;
    this->m_increment = 100;
    this->m_history_avg_error = 0.0f;
    this->m_error_thres = 0.13f;    
    this->m_history_trajectory = Matrix<float>(400, 2);
    this->m_start_at_new_seg = true;
}   

Fitting::~Fitting(){

}

void Fitting::smoothing(){

}

void Fitting::filtering(const Matrix<float> position, Matrix<float>& filtered_trajectory, bool& new_model){
    int rows = position.rows();
    int cols = position.cols();
    // only support one 2D point
    assert(cols == 2 && rows == 1);
    if(this->m_current_time < 3){
        this->updateHistory(position, false);
        new_model = false;
        filtered_trajectory = this->m_history_trajectory(Range(0, this->m_current_time),Range(0,2)).clone();
        return;
    }

    if(this->isChangePoint(position, filtered_trajectory)){
        this->updateHistory(position, true);
        filtered_trajectory = Matrix<float>(1, 2);
        filtered_trajectory.copy(position);
        new_model = true;
    }
    else{
        this->updateHistory(position, false);
        new_model = false;
    }
}

void Fitting::forecasting(){

}

bool Fitting::isChangePoint(const Matrix<float> position, Matrix<float>& filtered_trajectory){
    // temp trajectory
    this->m_history_trajectory(Range(this->m_current_time, this->m_current_time+1), Range(0,2)).copy(position);
    Matrix<float> temp_trajectory = this->m_history_trajectory(Range(0, this->m_current_time+1), Range(0,2));
    filtered_trajectory = this->fit(temp_trajectory);

    int t_n = this->m_current_time + 1;
    float new_error = 0.0f;
    for(int t_i=0; t_i<t_n; ++t_i){
        float dis_x = filtered_trajectory.at(t_i, 0)-temp_trajectory.at(t_i, 0);
        float dis_y = filtered_trajectory.at(t_i, 1)-temp_trajectory.at(t_i, 1);
        float error = sqrt((dis_x*dis_x)+(dis_y*dis_y));
        new_error += error;
    }
    new_error = new_error / float(t_n);
    float epsilon = (new_error - this->m_history_avg_error)/(this->m_history_avg_error + eagleeye_eps);
    if(!this->m_start_at_new_seg && epsilon > this->m_error_thres){
        this->m_history_avg_error = 0.0f;
        this->m_start_at_new_seg = true;
        return true;
    }
    else{
       this->m_history_avg_error = new_error;
    }
    this->m_start_at_new_seg = false;
    return false;
}

bool Fitting::updateHistory(const Matrix<float> position, bool flag){
    if(flag){
        // position belong a new model
        this->m_current_time = 1;
        this->m_history_trajectory(Range(0,1),Range(0,2)).copy(position);
    }
    else{
        // new trajectory doesnot belong a new model
        if(this->m_current_time + 2 > this->m_history_trajectory.rows()){
            Matrix<float> temp(this->m_history_trajectory.rows()+this->m_increment, 2);
            temp(Range(0, this->m_current_time),Range(0,2)).copy(this->m_history_trajectory(Range(0,this->m_current_time),Range(0,2)));

            this->m_history_trajectory = temp;

        }

        this->m_history_trajectory(Range(this->m_current_time, this->m_current_time+1), Range(0,2)).copy(position);
        this->m_current_time += 1;
    }

    return true;
}

Matrix<float> Fitting::fit(const Matrix<float> trajectory){
    if(this->m_fitting_type == PolynomialFitting){
        return this->polynomialFit(trajectory);
    }
    else if(this->m_fitting_type == LinearFitting){
        return this->linearFit(trajectory);
    }

    return this->m_history_trajectory;
}

Matrix<float> Fitting::polynomialFit(const Matrix<float> trajectory){
    // time length
    int tn = trajectory.rows();
    // solve x = a_1 + a_2 t + a_3 t^2
    // M * A = X
    Matrix<float> M(3,3);
    Matrix<float> X(3,1);
    float t_sum = 0.0;
    float t2_sum = 0.0;
    float t3_sum = 0.0;
    float t4_sum = 0.0;
    float x_sum = 0.0;
    float tx_sum = 0.0;
    float t2x_sum = 0.0;
    for(int t_i=0; t_i<tn; ++t_i){
        t_sum += t_i;
        t2_sum += t_i*t_i;
        t3_sum += t_i*t_i*t_i;
        t4_sum += t_i*t_i*t_i*t_i;

        float x = trajectory.at(t_i, 0);
        x_sum += x;
        tx_sum += t_i * x;
        t2x_sum += t_i*t_i*x;
    }
    M.at(0,0) = tn;
    M.at(0,1) = t_sum;
    M.at(0,2) = t2_sum;
    M.at(1,0) = t_sum;
    M.at(1,1) = t2_sum;
    M.at(1,2) = t3_sum;
    M.at(2,0) = t2_sum;
    M.at(2,1) = t3_sum;
    M.at(2,2) = t4_sum;
    X.at(0,0) = x_sum;
    X.at(1,0) = tx_sum;
    X.at(1,1) = t2x_sum;

    Matrix<float> inv_M = M.inv();
    Matrix<float> A = inv_M*X;

    // solve y = b_1 + b_2 t + b_3 t^2
    float y_sum = 0.0;
    float ty_sum = 0.0;
    float t2y_sum = 0.0;
    for(int t_i=0; t_i<tn; ++t_i){
        float y = trajectory.at(t_i, 1);
        y_sum += y;
        ty_sum += t_i * y;
        t2y_sum += t_i*t_i*y;
    }
    Matrix<float> Y(3,1);
    Y.at(0,0) = y_sum;
    Y.at(1,0) = ty_sum;
    Y.at(2,0) = t2y_sum;
    Matrix<float> B = inv_M*Y;

    Matrix<float> curve_trajectory(tn, 2);
    float a0 = A.at(0,0);
    float a1 = A.at(1,0);
    float a2 = A.at(2,0);
    float b0 = B.at(0,0);
    float b1 = B.at(1,0);
    float b2 = B.at(2,0);
    for(int t_i=0; t_i<tn; ++t_i){
        // x
        float x = a0 + a1 * t_i + a2 * t_i * t_i;
        curve_trajectory.at(t_i, 0) = x;
        // y
        float y = b0 + b1 * t_i + b2 * t_i * t_i;
        curve_trajectory.at(t_i, 1) = y;
    }

    return curve_trajectory;
}

Matrix<float> Fitting::linearFit(const Matrix<float> trajectory){
    return trajectory;
}
} // namespace eagleeye
