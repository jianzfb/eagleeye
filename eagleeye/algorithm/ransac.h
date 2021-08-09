#ifndef _EAGLEEYE_RANSAC_H_
#define _EAGLEEYE_RANSAC_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include <iostream>
#include <cmath>
#include <string>
#include <random>
#include <memory>
#include <algorithm>
#include <vector>
#include <omp.h>

namespace eagleeye{
// Each abstract model is made of abstract parameters
// Could be anything from a point (that make a 2D line or 3D plane or image correspondences) to a line
class AbstractParameter{
public:
    virtual ~AbstractParameter(void) {}; // To make this polymorphic we add dummy destructor
};

// Abstract model type for generic RANSAC model fitting
template <int t_NumParams, int t_ModelSize> /* Minimum number of parameters required to define this model*/
class AbstractModel{
public:
    static const int ModelParamNum			= t_NumParams;
    static const int ModelSize              = t_ModelSize;

    AbstractModel(){
    }

protected:
    std::array<std::shared_ptr<AbstractParameter>, t_NumParams> m_MinModelParams;

public:
    virtual Matrix<float> build(const std::vector<std::shared_ptr<AbstractParameter>> &InputParams) = 0;
    virtual Matrix<float> evaluate(const std::vector<std::shared_ptr<AbstractParameter>> &EvaluateParams, Matrix<float> MultiH,float Threshold) = 0;
    virtual std::array<std::shared_ptr<AbstractParameter>, t_NumParams> getModelParams(void) { return m_MinModelParams; };
};

// T - AbstractModel
template <class T>
class RANSAC{
private:
    const int t_NumParams = T::ModelParamNum;
    std::vector<std::shared_ptr<AbstractParameter>> m_Data; // All the data
    Matrix<float> m_BestModel;                          // Pointer to the best model, valid only after Estimate() is called
    std::vector<std::shared_ptr<AbstractParameter>> m_BestInliers;

    int m_MaxIterations;    // Number of iterations before termination
    float m_Threshold;      // The threshold for computing model consensus
    float m_BestModelScore; // The score of the best model
    int m_BestModelIdx;

    std::vector<std::mt19937> m_RandEngines; // Mersenne twister high quality RNG that support *OpenMP* multi-threading
    std::shared_ptr<T> m_RandomModel;
    int m_nThreads;

public:
    RANSAC(void){
        m_nThreads = std::max(1, omp_get_max_threads());   
        EAGLEEYE_LOGD("RANSAC Maximum usable threads: %d", m_nThreads);
        for (int i = 0; i < m_nThreads; ++i){
            std::random_device SeedDevice;
            m_RandEngines.push_back(std::mt19937(SeedDevice()));
        }
        m_RandomModel = std::make_shared<T>();
        reset();
    };

    virtual ~RANSAC(void) {};

    void reset(void){
        // Clear sampled models, etc. and prepare for next call. Reset RANSAC estimator state
        m_Data.clear();
        m_BestModelIdx = -1;
        m_BestModelScore = 0.0;
    };

    void initialize(float Threshold, int MaxIterations = 1000){
        m_Threshold = Threshold;
        m_MaxIterations = MaxIterations;
    };

    Matrix<float> getBestModel() { return m_BestModel; };
    const std::vector<std::shared_ptr<AbstractParameter>>& getBestInliers(void) { return m_BestInliers; };

    bool estimate(const std::vector<std::shared_ptr<AbstractParameter>> &Data){
        if (Data.size() <= t_NumParams){
            EAGLEEYE_LOGE("RANSAC - Number of data points is too less. Not doing anything.");
            return false;
        }
        m_Data = Data;
        int DataSize = m_Data.size();
        std::vector<Matrix<float>> ransac_models(m_MaxIterations);

        EAGLEEYE_TIME_START(ini_ransac_models);      
        omp_set_dynamic(0); // Explicitly disable dynamic teams
        omp_set_num_threads(m_nThreads);
#pragma omp parallel for
        for(int i=0; i<m_MaxIterations; ++i){ 
            int thread_id = omp_get_thread_num();
            // Select t_NumParams random samples
            std::vector<std::shared_ptr<AbstractParameter>> RandomSamples(t_NumParams);
            std::vector<std::shared_ptr<AbstractParameter>> RemainderSamples = m_Data; // Without the chosen random samples
            std::shuffle(RemainderSamples.begin(), RemainderSamples.end(), m_RandEngines[thread_id]); // To avoid picking the same element more than once
            std::copy(RemainderSamples.begin(), RemainderSamples.begin() + t_NumParams, RandomSamples.begin());

            Matrix<float> M = m_RandomModel->build(RandomSamples);
            ransac_models[i] = M.flatten();
        }
        EAGLEEYE_TIME_END(ini_ransac_models);

        Matrix<float> ransac_models_mat = concat(ransac_models, 0);
        Matrix<float> ransac_inlier_fractions = m_RandomModel->evaluate(m_Data, ransac_models_mat, m_Threshold);

        for (int i = 0; i < m_MaxIterations; ++i){
            // Check if the sampled model is the best so far 
            if (ransac_inlier_fractions.at(0,i) > m_BestModelScore){
                m_BestModelScore = ransac_inlier_fractions.at(0,i);
                m_BestModel = ransac_models_mat(Range(i,i+1),Range(0,9));
            }
        }

        m_BestModel = m_BestModel.clone();
        EAGLEEYE_LOGD("BestInlierFraction: %f",m_BestModelScore);
        reset();
        return true;
    };
};
}
#endif