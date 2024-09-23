#include "eagleeye/engine/nano/op/person_id_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "Eigen/Dense"
#include <fstream>
#include <sys/time.h>

namespace eagleeye{
namespace dataflow{
typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> EigenComMatrixXf;
PersonIdOp::PersonIdOp(){
    m_score_thres = 0.6f;
}

PersonIdOp::PersonIdOp(const PersonIdOp& op){}
PersonIdOp::~PersonIdOp(){}

int PersonIdOp::init(std::map<std::string, std::vector<std::string>> params){
    return 0;
}

int PersonIdOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("thres") != params.end()){
        m_score_thres = params["thres"][0];
    }
    return 0;
}

int PersonIdOp::runOnCpu(const std::vector<Tensor>& input){
    // input: NxD person feature
    // output: N person id 
    if(input[0].empty() || input[0].dims()[0] == 0){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{0, 16},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        return 0;
    }

    int query_person_num = input[0].dims()[0];
    this->m_outputs[0] = Tensor(
        std::vector<int64_t>{query_person_num, 16},
        EAGLEEYE_UCHAR,
        DataFormat::AUTO,
        CPU_BUFFER
    );

    // 初始化
    for(int person_i=0; person_i<query_person_num; ++person_i){
        unsigned char* person_id_ptr = this->m_outputs[0].cpu<unsigned char>() + person_i*16;
        memset(person_id_ptr, '\0', 16);
    }
    int query_person_feature_dim = input[0].dims().production()/query_person_num;

    // 发现人体ID
    std::vector<std::string> selected_person_id;
    std::vector<float> selected_person_score;
    for(int face_i=0; face_i<query_person_num; ++face_i){
        selected_person_id.push_back("");
        selected_person_score.push_back(0.0f);
    }

    Eigen::Map<EigenComMatrixXf> query_face_features_mat(const_cast<float*>(input[0].cpu<float>()), query_person_num, query_person_feature_dim);
    std::map<std::string, Tensor>::iterator iter, iend(m_person_gallery.end());
    for(iter=m_person_gallery.begin(); iter != iend; ++iter){
        std::string person_name = iter->first;
        Tensor person_face_tensor = iter->second;
        int person_face_tensor_n = person_face_tensor.dims()[0];

        const float* gallery_face_features_ptr = person_face_tensor.cpu<float>();
        Eigen::Map<EigenComMatrixXf> gallery_face_features_mat(const_cast<float*>(gallery_face_features_ptr), person_face_tensor_n, query_person_feature_dim);
        EigenComMatrixXf gallery_face_features_mat_t = gallery_face_features_mat.transpose();
        EigenComMatrixXf sim_score_mat = query_face_features_mat * gallery_face_features_mat_t;  // query_face_num x gallery_face_num

        for(int query_person_i=0; query_person_i<query_person_num; ++query_person_i){
            float max_score = 0.0f;
            for(int gallery_person_i=0; gallery_person_i<person_face_tensor_n; ++gallery_person_i){
                if(sim_score_mat(query_person_i, gallery_person_i) > max_score){
                    max_score = sim_score_mat(query_person_i, gallery_person_i);
                }
            }
            if(max_score > m_score_thres && selected_person_score[query_person_i] < max_score){
                selected_person_score[query_person_i] = max_score;
                selected_person_id[query_person_i] = person_name;
            }
        }
    }

    for(int person_i=0; person_i<query_person_num; ++person_i){
        unsigned char* person_id_ptr = this->m_outputs[0].cpu<unsigned char>() + person_i*16;
        const float* person_i_feature = input[0].cpu<float>() + person_i * query_person_feature_dim;

        if(selected_person_id[person_i] != ""){
            // 发现ID
            std::string person_name = selected_person_id[person_i];
            memcpy(person_id_ptr, person_name.c_str(), 16);

            // 循坏更新
            Tensor person_tensor = m_person_gallery[person_name];
            int offset_i = m_person_offset[person_name];
            memcpy(person_tensor.cpu<float>()+offset_i*query_person_feature_dim, person_i_feature, sizeof(float)*query_person_feature_dim);
            m_person_offset[person_name] = (offset_i + 1) % 10;
        }
        else{
            // 没有发现ID， 自动创建新ID， 更新特征库
            struct timeval now_tv;
            gettimeofday(&now_tv,NULL);
            std::string person_name = std::to_string(now_tv.tv_sec * 1000000 + now_tv.tv_usec);
            memcpy(person_id_ptr, person_name.c_str(), 16);

            Tensor person_tensor(
                std::vector<int64_t>{10, query_person_feature_dim},
                EAGLEEYE_FLOAT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );
            memset(person_tensor.cpu(), 0, sizeof(float)*10*query_person_feature_dim);
            memcpy(person_tensor.cpu(), person_i_feature, sizeof(float)*query_person_feature_dim);
            m_person_gallery[person_name] = person_tensor;
            m_person_offset[person_name] = 0;
        }
    }
    return 0;
}

int PersonIdOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}