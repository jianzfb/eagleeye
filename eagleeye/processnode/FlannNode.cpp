#include "eagleeye/processnode/FlannNode.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/CJsonObject.hpp"
#include "eagleeye/framework/pipeline/BooleanSignal.h"
#include "eagleeye/framework/pipeline/TensorSignal.h"
#include <flann/flann.hpp>
#include <fstream>

namespace eagleeye{
std::map<std::string, std::vector<std::string>> g_collection_ids;
std::map<std::string, std::shared_ptr<flann::Index<flann::L2<float>>>> g_collection_index;
std::map<std::string, long> g_collection_update_time;

FlannNode::FlannNode(){
    m_collection = "default";
    m_root_folder = "./";
    m_is_init = false;
    m_top_k = 1;
    m_use_norm = true;

    // 设置输出端口（用于标记执行成功/失败）
    this->setNumberOfOutputSignals(3);
    this->setOutputPort(new BooleanSignal(), 0);        // 执行是否成功
    this->setOutputPort(new TensorSignal(), 1);         // SCORE: float N x top_k
    this->setOutputPort(new TensorSignal(), 2);         // ID: char N x top_k x 16

    EAGLEEYE_MONITOR_VAR(std::string, setWritableFolder, getWritableFolder, "writable_path", "", "");
}
FlannNode::FlannNode(std::string collection, std::string root_folder, int top_k, bool use_norm){
    m_collection = collection;
    m_top_k = top_k;
    m_is_init = false;
    m_use_norm = use_norm;
    if(!isdirexist(root_folder.c_str())){
        createdirectory(root_folder.c_str());
    }
    m_root_folder = root_folder;

    // 设置输出端口（用于标记执行成功/失败）
    this->setNumberOfOutputSignals(3);
    this->setOutputPort(new BooleanSignal(), 0);        // 执行是否成功
    this->setOutputPort(new TensorSignal(), 1);         // SCORE: float N x top_k
    this->setOutputPort(new TensorSignal(), 2);         // ID: char N x top_k x 16

    EAGLEEYE_MONITOR_VAR(std::string, setWritableFolder, getWritableFolder, "writable_path", "", "");
}

FlannNode::~FlannNode(){
}

void FlannNode::executeNodeInfo(){
    // 加载数据库
    // 首次或数据库发生更新，均需要进行重新初始化flann
    if(!m_is_init || g_collection_update_time[m_collection] < m_memory->time()){
        if(m_memory.get() == NULL){
            m_memory = std::shared_ptr<KVMemoryManage>(new KVMemoryManage(m_root_folder, m_collection));
        }

        // 创建搜索树
        g_collection_index[m_collection] = std::shared_ptr<flann::Index<flann::L2<float>>>(new flann::Index<flann::L2<float>>(flann::KDTreeIndexParams(4)));
        g_collection_ids[m_collection] = std::vector<std::string>();

        // 加载数据
        long time = m_memory->load(
            [&](neb::CJsonObject& db){
                if(db.GetArraySize() > 0){
                    float* data_ptr = NULL;
                    int point_num = db.GetArraySize();
                    int point_dim = 0;
                    for(int i=0; i<point_num; ++i){
                        neb::CJsonObject info;
                        db.Get(i, info);
                        std::string name;
                        info.Get("key", name);
                        g_collection_ids[m_collection].push_back(name);

                        neb::CJsonObject feature;
                        info.Get("value", feature);
                        point_dim = feature.GetArraySize();
                        if(data_ptr == NULL){
                            data_ptr = new float[point_num*point_dim];
                        }
                        float* point_i_ptr = data_ptr + i * point_dim;
                        for(int j=0; j<point_dim; ++j){
                            feature.Get(j, point_i_ptr[j]);
                        }
                    }

                    flann::Matrix points(data_ptr, point_num, point_dim);
                    g_collection_index[m_collection]->buildIndex(points);
                    delete[] data_ptr;
                }
            }
        );
        g_collection_update_time[m_collection] = time;
        m_is_init = true;
    }

    // input: feature, group, ids
    // 检索（NxD）
    TensorSignal* feature_sig = (TensorSignal*)this->getInputPort(0);
    Tensor face_features = feature_sig->getData().clone();
    if(face_features.empty()){
        BooleanSignal* status = (BooleanSignal*)this->getOutputPort(0);
        status->setData(false);
        return;
    }

    // 组标识(忽略)
    TensorSignal* group_sig = (TensorSignal*)this->getInputPort(1);
    std::vector<std::string> str_list =  group_sig->getString();
    std::string group_name = str_list.size() == 0 ? "" : str_list[0];

    int face_num = face_features.dims()[0];
    int face_dim = face_features.dims()[1];
    float* face_ptr = face_features.cpu<float>();
    if(m_use_norm){
        // normlize
        for(int face_i=0; face_i<face_num; ++face_i){
            float* face_i_ptr = face_ptr + face_i * face_dim;
            float vec_len = 0.0f;
            for(int i=0; i<face_dim; ++i){
                vec_len += face_i_ptr[i] * face_i_ptr[i];
            }
            vec_len = sqrt(vec_len);
            for(int i=0; i<face_dim; ++i){
                face_i_ptr[i] /= vec_len;
            }
        }
    }

    // 操作 插入/检索/删除
    if(m_mode == FLANN_INSERT){
        TensorSignal* ids_sig = (TensorSignal*)this->getInputPort(2);
        std::vector<std::string> insert_ids = ids_sig->getString();
        if(insert_ids.size() == 0){
            // 基于时间戳分配ID
            for(int face_i=0; face_i<face_num; ++face_i){
                struct timeval now_tv;
                gettimeofday(&now_tv,NULL);
                // 16位编码
                std::string group_str = std::to_string(now_tv.tv_sec * 1000000 + now_tv.tv_usec);
                insert_ids.push_back(group_str);
                // 10 微秒
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
        for(int i=0; i<insert_ids.size(); ++i){
            g_collection_ids[m_collection].push_back(insert_ids[i]);
        }

        std::unique_lock<std::mutex> locker(m_mu);
        flann::Matrix<float> points(face_ptr, face_num, face_dim);
        g_collection_index[m_collection]->addPoints(points);
        locker.unlock();

        // 更新输出
        TensorSignal* face_id_sig = (TensorSignal*)this->getOutputPort(2);
        int face_id_dim = 16;
        Tensor face_id_content(
            std::vector<int64_t>{face_num, 1, face_id_dim},
            EAGLEEYE_CHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        face_id_sig->setData(face_id_content);
        for(int face_i=0; face_i<face_num; ++face_i){
            std::string face_id_str = insert_ids[face_i];
            char* face_id_content_ptr = face_id_content.cpu<char>() + face_i * 1 * face_id_dim;
            memcpy(face_id_content_ptr, face_id_str.c_str(), face_id_str.size());
        }

        // 更新存储
        for(int i=0; i<insert_ids.size(); ++i){
            int insert_index = -1;
            this->m_memory->insert(g_collection_ids[m_collection][i], std::vector<float>(face_ptr+i*face_dim, face_ptr+(i+1)*face_dim), insert_index);
        }
    }
    else if(m_mode == FLANN_QUERY){
        if(g_collection_index[m_collection]->size() == 0){
            BooleanSignal* status = (BooleanSignal*)this->getOutputPort(0);
            status->setData(false);
            EAGLEEYE_LOGD("No data in search db.");
            return;
        }

        std::unique_lock<std::mutex> locker(m_mu);
        flann::Matrix<float> points(face_ptr, face_num, face_dim);
        flann::Matrix<int> indices(new int[face_num*m_top_k], face_num, m_top_k);
        flann::Matrix<float> dists(new float[face_num*m_top_k], face_num, m_top_k);
        g_collection_index[m_collection]->knnSearch(points, indices, dists, m_top_k, flann::SearchParams(128));
        locker.unlock();

        TensorSignal* face_score_sig = (TensorSignal*)this->getOutputPort(1);
        Tensor person_score_content(
            std::vector<int64_t>{face_num, m_top_k},
            EAGLEEYE_FLOAT,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        face_score_sig->setData(person_score_content);
        memcpy(person_score_content.cpu<float>(), dists.ptr(), sizeof(float)*face_num*m_top_k);

        TensorSignal* face_id_sig = (TensorSignal*)this->getOutputPort(2);
        int face_id_dim = 16;
        Tensor face_id_content(
            std::vector<int64_t>{face_num, m_top_k, face_id_dim},
            EAGLEEYE_CHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        face_id_sig->setData(face_id_content);
        for(int face_i=0; face_i<face_num; ++face_i){
            char* face_id_content_ptr = face_id_content.cpu<char>() + face_i * this->m_top_k * face_id_dim;
            for(int k=0; k<m_top_k; ++k){
                int face_index = indices.ptr()[face_i*m_top_k+k];
                std::string face_id_str = g_collection_ids[m_collection][face_index];
                memcpy(face_id_content_ptr+k*face_id_dim, face_id_str.c_str(), face_id_dim);
            }
        }

        delete[] indices.ptr();
        delete[] dists.ptr();
    }
    else{
        // TODO，等待实现
    }

    BooleanSignal* status = (BooleanSignal*)this->getOutputPort(0);
    status->setData(true);
}

void FlannNode::setWritableFolder(const std::string writable_folder){
    this->m_root_folder = writable_folder;
}
void FlannNode::getWritableFolder(std::string& writable_folder){
    writable_folder = this->m_root_folder;
}

}