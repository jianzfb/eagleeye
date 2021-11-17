#include "eagleeye/algorithm/homographymodel.h"
#include "Eigen/Dense"
#include "Eigen/SVD"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/common/EagleeyeLog.h"
#ifdef EAGLEEYE_NEON_OPTIMIZATION
#include <arm_neon.h>
#endif

namespace eagleeye{
HomographyModel::HomographyModel(){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    EAGLEEYE_OPENCL_KERNEL_GROUP(homography, algorithm, homography_eva);
#endif
}    

HomographyModel::~HomographyModel(){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION   
    EAGLEEYE_OPENCL_RELEASE_KERNEL_GROUP(homography);
#endif   
}

Matrix<float> HomographyModel::build(const std::vector<std::shared_ptr<AbstractParameter>>& input_params){
    Eigen::MatrixXf A(2*4,9);
    for(int i=0; i<4; ++i){
        // auto p = std::dynamic_pointer_cast<HomographyParam>(input_params[i]);
        const HomographyParam* p = (HomographyParam*)input_params[i].get();
        float nx1 = p->m_p1.at(0,0)/p->m_p1.at(2,0);
        float ny1 = p->m_p1.at(1,0)/p->m_p1.at(2,0);
        
        float nx2 = p->m_p2.at(0,0)/p->m_p2.at(2,0);
        float ny2 = p->m_p2.at(1,0)/p->m_p2.at(2.0);

        A(i*2, 0) = -nx1;
        A(i*2, 1) = -ny1;
        A(i*2, 2) = -1;
        A(i*2, 3) = 0;
        A(i*2, 4) = 0;
        A(i*2, 5) = 0;
        A(i*2, 6) = nx2*nx1;
        A(i*2, 7) = nx2*ny1;
        A(i*2, 8) = nx2;

        A(i*2+1, 0) = 0;
        A(i*2+1, 1) = 0;
        A(i*2+1, 2) = 0;
        A(i*2+1, 3) = -nx1;
        A(i*2+1, 4) = -ny1;
        A(i*2+1, 5) =-1;
        A(i*2+1, 6) = ny2*nx1;
        A(i*2+1, 7) = ny2*ny1;
        A(i*2+1, 8) = ny2;
    }
    
    Eigen::JacobiSVD<Eigen::MatrixXf> svd(A, Eigen::ComputeFullV);
    Eigen::MatrixXf singular_val = svd.singularValues();
    Eigen::MatrixXf V = svd.matrixV();
    Matrix<float> H(3,3);
    float* H_ptr = H.dataptr();
    for(int i=0; i<3; ++i){
        for(int j=0; j<3; ++j){
            H_ptr[i*3+j] = V(i*3+j,8);
        }
    }
    return H;
}

Matrix<float> HomographyModel::evaluate(const std::vector<std::shared_ptr<AbstractParameter>>& evaluate_params, Matrix<float> MultiH, float threshold){
    int nTotalParams = evaluate_params.size();
    int nInliers = 0;
    int multi_h_num = MultiH.rows();
    Matrix<float> multi_h_inlier_fraction(1,multi_h_num);

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    Matrix<float> result_mat(multi_h_num, nTotalParams);
    size_t work_dims = 2;
    size_t global_size[2] = {size_t(multi_h_num), size_t(nTotalParams)};
    size_t local_size[2] = {1, 1024};
    Matrix<float> source_points_mat(nTotalParams, 3);
    Matrix<float> target_points_mat(nTotalParams, 3);
    for(int i=0; i<nTotalParams; ++i){
        // auto hp = std::dynamic_pointer_cast<HomographyParam>(evaluate_params[i]);
        const HomographyParam* hp = (HomographyParam*)evaluate_params[i].get();
        float* source_points_mat_ptr = source_points_mat.row(i);
        float* target_points_mat_ptr = target_points_mat.row(i);

        source_points_mat_ptr[0] = hp->m_p1.at(0,0);
        source_points_mat_ptr[1] = hp->m_p1.at(1,0);
        source_points_mat_ptr[2] = hp->m_p1.at(2,0);

        target_points_mat_ptr[0] = hp->m_p2.at(0,0);
        target_points_mat_ptr[1] = hp->m_p2.at(1,0);
        target_points_mat_ptr[2] = hp->m_p2.at(2,0);
    }

    EAGLEEYE_OPENCL_CREATE_READ_BUFFER(homography, source_points, sizeof(float)*nTotalParams*3);
    EAGLEEYE_OPENCL_CREATE_READ_BUFFER(homography, target_points, sizeof(float)*nTotalParams*3);
    EAGLEEYE_OPENCL_CREATE_READ_BUFFER(homography, H, sizeof(float)*9*multi_h_num);
    EAGLEEYE_OPENCL_CREATE_WRITE_BUFFER(homography, result, sizeof(float)*nTotalParams*multi_h_num);

    EAGLEEYE_OPENCL_COPY_TO_DEVICE(homography,source_points,source_points_mat.dataptr());
    EAGLEEYE_OPENCL_COPY_TO_DEVICE(homography,target_points,target_points_mat.dataptr());
    EAGLEEYE_OPENCL_COPY_TO_DEVICE(homography, H, MultiH.dataptr());

    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(homography, homography_eva, 0, source_points);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(homography, homography_eva, 1, H);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(homography, homography_eva, 2, target_points);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(homography, homography_eva, 3, result);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(homography, homography_eva, 4, multi_h_num);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(homography, homography_eva, 5, nTotalParams);
    EAGLEEYE_OPENCL_KERNEL_RUN(homography, homography_eva,work_dims,global_size, local_size);
    EAGLEEYE_OPENCL_COPY_TO_HOST(homography,result, result_mat.dataptr());

    for(int h_i=0; h_i<multi_h_num; ++h_i){
        nInliers = 0;
        float* result_mat_ptr = result_mat.row(h_i);
        for(int i=0; i<nTotalParams; ++i){
            if(result_mat_ptr[i] < threshold){
                nInliers++;
            }
        }    

        float InlierFraction = float(nInliers) / float(nTotalParams); // This is the inlier fraction
        multi_h_inlier_fraction.at(0, h_i) = InlierFraction;
    }
#else
    Matrix<float> bigger_h(1,12);
    for(int h_i=0; h_i<multi_h_num; ++h_i){
        memcpy(bigger_h.dataptr(), MultiH.row(h_i), sizeof(float)*9);
        for (auto& param : evaluate_params){
            if (computeDistanceMeasure(param, bigger_h) < threshold){
                nInliers++;
            }
        }

        float InlierFraction = float(nInliers) / float(nTotalParams); // This is the inlier fraction
        multi_h_inlier_fraction.at(0, h_i) = InlierFraction;
    }
#endif
    
    return multi_h_inlier_fraction;
}

float HomographyModel::computeDistanceMeasure(std::shared_ptr<AbstractParameter> param, Matrix<float> model){
    auto hp = std::dynamic_pointer_cast<HomographyParam>(param);

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    float32x4x3_t l = vld3q_f32(model.dataptr());
    float* p1_ptr = hp->m_p1.dataptr();
    float* p2_ptr = hp->m_p2.dataptr();

    float32x4_t r = vld1q_dup_f32(p1_ptr);
    float32x4_t q = vmulq_f32(l.val[0], r);
    r = vld1q_dup_f32(p1_ptr+1);
    q = vmlaq_f32(q, l.val[1], r);    
    r = vld1q_dup_f32(p1_ptr+2);
    q = vmlaq_f32(q, l.val[2], r);
    float res[4];
    vst1q_f32(res,q);
    return (abs(res[0]/res[2] - p2_ptr[0])+abs(res[1]/res[2] - p2_ptr[1]))/2.0f;
#else
    // Matrix<float> projected_p = Matrix<float>(3,3,this->M.dataptr()) * hp->m_p1;
    // // projected_p = projected_p / projected_p.at(2,0);
    // Matrix<float> res = projected_p - hp->m_p2;
    // return res.at(0,0)+res.at(1,0)+res.at(2,0);
    return 0.0f;
#endif
}

bool HomographyModel::decompose(const Matrix<float> model, Matrix<float>& translation, Matrix<float>& scale, float& rotation){
    Matrix<float> normalized_model = model.clone();
    float h33 = normalized_model.at(2,2);
    if(abs(h33) < 0.00001){
        EAGLEEYE_LOGD("model is abnormal, decompose dont process");
        return false;
    }
    normalized_model /= h33;

    // get translation matrix
    translation = Matrix<float>(2,1);
    translation.at(0,0) = normalized_model.at(0,2);
    translation.at(1,0) = normalized_model.at(1,2);

    // get scale matrix
    scale = Matrix<float>(2,1);
    scale.at(0,0) = sqrt(normalized_model.at(0,0)*normalized_model.at(0,0)+normalized_model.at(0,1)*normalized_model.at(0,1));
    if(normalized_model.at(0,0) < 0){
        scale.at(0,0) = -scale.at(0,0);
    }

    scale.at(1,0) = sqrt(normalized_model.at(1,0)*normalized_model.at(1,0)+normalized_model.at(1,1)*normalized_model.at(1,1));
    if(normalized_model.at(1,1) < 0){
         scale.at(1,0) = - scale.at(1,0);
    }

    // get rotation matrix
    rotation = atan2(-normalized_model.at(0,1),normalized_model.at(0,0));
    return true;
}
}