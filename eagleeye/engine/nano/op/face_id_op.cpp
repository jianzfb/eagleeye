#include "eagleeye/engine/nano/op/face_id_op.h"
#include "eagleeye/engine/nano/op/kvmemory_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "Eigen/Dense"

namespace eagleeye{
namespace dataflow{
typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> EMatrixF;

FaceIdOp::FaceIdOp(){
    m_cache_folder = "./cache";
    m_score_thres = 0.55f;
}

FaceIdOp::FaceIdOp(const FaceIdOp& op){
}

FaceIdOp::~FaceIdOp(){
}

int FaceIdOp::init(std::map<std::string, std::vector<std::string>> params){
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
    // input: memory name, NxD face feature
    // output: N face id 
    const char* memory_name_ptr = input[0].cpu<char>();
    char* memory_name_str_ptr = (char*)malloc(input[0].dims().production() + 1);
    memset(memory_name_str_ptr, '\0', input[0].dims().production() + 1);
    memcpy(memory_name_str_ptr, memory_name_ptr, input[0].dims().production());
    std::string memory_name = memory_name_str_ptr;
    free(memory_name_str_ptr);

    if(input[1].empty()){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{0, 16},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        return 0;
    }

    int query_face_num = input[1].dims()[0];
    this->m_outputs[0] = Tensor(
        std::vector<int64_t>{query_face_num, 16},
        EAGLEEYE_UCHAR,
        DataFormat::AUTO,
        CPU_BUFFER
    );

    // 初始化
    for(int face_i=0; face_i<query_face_num; ++face_i){
        unsigned char* face_id_ptr = this->m_outputs[0].cpu<unsigned char>() + face_i*16;
        memset(face_id_ptr, '\0', 16);
    }
    if(KVMemoryOp::m_g_info.find(memory_name) == KVMemoryOp::m_g_info.end()){
        // no face gallery
        return 0;
    }

    // 初始化重组的特征信息
    // 人脸特征库更新时间戳，用于标记是否需要重新建立重组的特征信息
    if(m_face_gallery_update_time.find(memory_name) == m_face_gallery_update_time.end()){
        m_face_gallery_update_time[memory_name] = 0;
    }
    // 人脸重组的特征信息
    if(m_face_gallery.find(memory_name) == m_face_gallery.end()){
        m_face_gallery[memory_name] = std::map<std::string, Tensor>();
    }

    EAGLEEYE_LOGD("local facegallery %s update_time %ld", memory_name.c_str(), m_face_gallery_update_time[memory_name]);
    EAGLEEYE_LOGD("KVMemoryOp %s time %ld", memory_name.c_str(), KVMemoryOp::m_g_time[memory_name]);
    if(m_face_gallery_update_time[memory_name] < KVMemoryOp::m_g_time[memory_name]){
        // 使用的人脸库旧，需要更新
        // TODO 多线程支持        
        // TODO,需要优化为仅对变动的条目进行更新
        EAGLEEYE_LOGD("update local facegallery %s", memory_name.c_str());
        m_face_gallery[memory_name].clear();
        std::map<std::string, std::vector<std::string>>::iterator iter, iend(KVMemoryOp::m_g_info[memory_name].end());
        for(iter = KVMemoryOp::m_g_info[memory_name].begin(); iter != iend; ++iter){
            std::vector<std::string> key_name_list = iter->second;
            std::vector<Tensor> tensor_list;
            for(int key_i=0; key_i<key_name_list.size(); ++key_i){
                std::string key_name = key_name_list[key_i];
                tensor_list.push_back(KVMemoryOp::m_g_memory[memory_name][key_name]);
            }
            if(tensor_list.size() == 0){
                continue;
            }

            int64_t face_num = tensor_list.size();
            int64_t feature_dim = tensor_list[0].dims().production();
            Tensor person_feature_tensor(
                std::vector<int64_t>{feature_dim, face_num},
                EAGLEEYE_FLOAT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );

            EAGLEEYE_LOGD("instance num %d, feature dim %d", face_num, feature_dim);
            // 使用feature_dim x face_num 格式存储
            float* person_feature_tensor_ptr = person_feature_tensor.cpu<float>();
            for(int key_i=0; key_i<face_num; ++key_i){
                float* src_ptr = tensor_list[key_i].cpu<float>();
                for(int i=0; i<feature_dim; ++i){
                    person_feature_tensor_ptr[i*face_num+key_i] = src_ptr[i];
                }
            }

            std::string person_name = iter->first;
            m_face_gallery[memory_name][person_name] = person_feature_tensor;
        }

        m_face_gallery_update_time[memory_name] = KVMemoryOp::m_g_time[memory_name];
    }

    if(m_face_gallery[memory_name].size() == 0){
        // face gallery empty
        return 0;
    }
    if(query_face_num == 0){
        // face query empty
        return 0;
    }
    int query_face_feature_dim = input[1].dims().production()/query_face_num;

    // 发现人脸ID
    std::vector<std::string> selected_face_id;
    std::vector<float> selected_face_score;
    for(int face_i=0; face_i<query_face_num; ++face_i){
        selected_face_id.push_back("");
        selected_face_score.push_back(0.0f);
    }

    Eigen::Map<EMatrixF> query_face_features_mat(const_cast<float*>(input[1].cpu<float>()), query_face_num, query_face_feature_dim);
    std::map<std::string, Tensor>::iterator iter, iend(m_face_gallery[memory_name].end());
    EAGLEEYE_LOGD("person num %d in memory %s", m_face_gallery[memory_name].size(), memory_name.c_str());
    for(iter=m_face_gallery[memory_name].begin(); iter != iend; ++iter){
        std::string person_name = iter->first;
        Tensor person_face_tensor = iter->second;
        int person_face_tensor_n = person_face_tensor.dims()[1];
        EAGLEEYE_LOGD("person name %s, sample num %d", person_name.c_str(), person_face_tensor_n);

        float* gallery_face_features_ptr = person_face_tensor.cpu<float>();
        Eigen::Map<EMatrixF> gallery_face_features_mat(gallery_face_features_ptr, query_face_feature_dim, person_face_tensor_n);
        EMatrixF sim_score_mat = query_face_features_mat * gallery_face_features_mat;  // query_face_num x gallery_face_num

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
                EAGLEEYE_LOGD("select face [%s] score [%f], thres [%f], select face score [%f]", person_name.c_str(), max_score, m_score_thres, selected_face_score[query_face_i]);
            }
        }
    }

    for(int face_i=0; face_i<query_face_num; ++face_i){
        unsigned char* face_id_ptr = this->m_outputs[0].cpu<unsigned char>() + face_i*16;
        if(selected_face_id[face_i] != ""){
            memcpy(face_id_ptr, selected_face_id[face_i].c_str(), 16);
        }
    }
    return 0;
}

int FaceIdOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}