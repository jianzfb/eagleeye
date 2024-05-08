#include "eagleeye/engine/nano/op/face_id_op.h"
#include "eagleeye/engine/nano/op/kvmemory_w_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "Eigen/Dense"
#include <fstream>

namespace eagleeye{
namespace dataflow{
typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> EigenComMatrixXf;

FaceIdOp::FaceIdOp(){
    m_memory_name = "memory";
    m_cache_folder = "./cache";
    m_cache_memory_folder = "";
    m_score_thres = 0.45f;
}

FaceIdOp::FaceIdOp(const FaceIdOp& op){
}

FaceIdOp::~FaceIdOp(){
}

int FaceIdOp::init(std::map<std::string, std::vector<std::string>> params){
    if(params.find("name") != params.end()){
        m_memory_name = params["name"][0];
    }
    if(params.find("model_folder") != params.end()){
        m_cache_folder = params["model_folder"][0];
    }
    return 0;
}

int FaceIdOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("thres") != params.end()){
        m_score_thres = params["thres"][0];
    }
    return 0;
}

int FaceIdOp::runOnCpu(const std::vector<Tensor>& input){
    // input: NxD face feature, 0/1 update face gallery
    // output: N face id 
    int is_update_face_gallery = input[1].cpu<int>()[0];
    if(m_face_gallery.size() == 0 || is_update_face_gallery){
        if(m_cache_memory_folder == ""){
            if(endswith(m_cache_folder, "/")){
                m_cache_memory_folder = m_cache_folder + m_memory_name;
            }
            else{
                m_cache_memory_folder = m_cache_folder + "/" + m_memory_name;
            }
        }

        if(!isdirexist(m_cache_memory_folder.c_str())){
            createdirectory(m_cache_memory_folder.c_str());
        }

        std::map<std::string, std::vector<std::string>>::iterator iter, iend(KVMemoryWOp::m_g_info[m_memory_name].end());
        for(iter = KVMemoryWOp::m_g_info[m_memory_name].begin(); iter != iend; ++iter){
            std::vector<std::string> key_name_list = iter->second;
            std::vector<Tensor> tensor_list;
            for(int k=0; k<key_name_list.size(); ++k){
                std::string key_name = key_name_list[k];
                tensor_list.push_back(KVMemoryWOp::m_g_memory[m_memory_name][key_name]);
            }
            if(tensor_list.size() == 0){
                continue;
            }

            int tensor_n = tensor_list.size();
            Tensor group_tensor(
                std::vector<int64_t>{tensor_n, tensor_list[0].dims()[1]},
                EAGLEEYE_FLOAT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );
            for(int k=0; k<tensor_list.size(); ++k){
                float* offset_ptr = group_tensor.cpu<float>() + k * group_tensor.dims()[1];
                float* src_ptr = tensor_list[k].cpu<float>();
                memcpy(offset_ptr, src_ptr, sizeof(float)*group_tensor.dims()[1]);
            }

            std::string group_name = iter->first;
            m_face_gallery[group_name] = group_tensor;
        }
    }

    int query_face_num = input[0].dims()[0];
    int query_face_feature_dim = input[0].dims()[1];
    this->m_outputs[0] = Tensor(
        std::vector<int64_t>{query_face_num,13},
        EAGLEEYE_UCHAR,
        DataFormat::AUTO,
        CPU_BUFFER
    );

    if(query_face_num == 0){
        return 0;
    }

    if(m_face_gallery.size() == 0){
        for(int face_i=0; face_i<query_face_num; ++face_i){
            unsigned char* face_id_ptr = this->m_outputs[0].cpu<unsigned char>() + face_i*13;
            memset(face_id_ptr, '\0', 13);
        }
        return 0;
    }

    // 发现人脸ID
    std::vector<std::string> selected_face_id;
    std::vector<float> selected_face_score;
    for(int face_i=0; face_i<query_face_num; ++face_i){
        selected_face_id.push_back("");
        selected_face_score.push_back(0.0f);
    }

    Eigen::Map<EigenComMatrixXf> query_face_features_mat(const_cast<float*>(input[0].cpu<float>()), query_face_num, query_face_feature_dim);
    std::map<std::string, Tensor>::iterator iter, iend(m_face_gallery.end());
    for(iter=m_face_gallery.begin(); iter != iend; ++iter){
        std::string person_name = iter->first;
        Tensor person_face_tensor = iter->second;
        int person_face_tensor_n = person_face_tensor.dims()[0];

        const float* gallery_face_features_ptr = person_face_tensor.cpu<float>();
        Eigen::Map<EigenComMatrixXf> gallery_face_features_mat(const_cast<float*>(gallery_face_features_ptr), person_face_tensor.dims()[0], query_face_feature_dim);
        EigenComMatrixXf gallery_face_features_mat_t = gallery_face_features_mat.transpose();
        EigenComMatrixXf sim_score_mat = query_face_features_mat * gallery_face_features_mat_t;  // query_face_num x gallery_face_num

        for(int query_face_i=0; query_face_i<query_face_num; ++query_face_i){
            float max_score = 0.0f;
            for(int gallery_face_i=0; gallery_face_i<person_face_tensor_n; ++gallery_face_i){
                if(sim_score_mat(query_face_i, gallery_face_i) > max_score){
                    max_score = sim_score_mat(query_face_i, gallery_face_i);
                }
            }
            if(max_score > m_score_thres && selected_face_score[query_face_i] < max_score){
                selected_face_score[query_face_i] = max_score;
                selected_face_id[query_face_i] = person_name;
            }
        }

        for(int face_i=0; face_i<query_face_num; ++face_i){
            unsigned char* face_id_ptr = this->m_outputs[0].cpu<unsigned char>() + face_i*13;
            memset(face_id_ptr, '/0', 13);
            if(selected_face_id[face_i] != ""){
                memcpy(face_id_ptr, person_name.c_str(), 13);
            }
        }
    }

    return 0;
}

int FaceIdOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}