#ifndef _EAGLEEYE_MilvusNode_H_
#define _EAGLEEYE_MilvusNode_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>
#include <map>
#include <memory>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include "milvus/MilvusClient.h"
#include "milvus/types/CollectionSchema.h"
#include "eagleeye/common/EagleeyeStr.h"

namespace eagleeye{
enum MilvusMode{
    MILVUS_INSERT   = 0,
    MILVUS_QUERY    = 1,
    MILVUS_REMOVE   = 2
};

class MilvusNode:public AnyNode, DynamicNodeCreator<MilvusNode>{
public:
    typedef AutoNode                Self;
    typedef AnyNode                 Superclass;
    EAGLEEYE_CLASSIDENTITY(MilvusNode);

    MilvusNode(){};
    MilvusNode(std::string collection, int top_k, bool use_norm, std::string ip, int port){
        m_is_ready = false;
        m_client = milvus::MilvusClient::Create();

        // step 1: 链接
        milvus::ConnectParam connect_param{ip, port};
        auto status = m_client->Connect(connect_param);
        EAGLEEYE_LOGD("milvus connect ip %s, port %d", ip.c_str(), port);
        if(!status.IsOk()){
            // 失败链接
            EAGLEEYE_LOGE("Failed to connect %s:%d", ip.c_str(), port);
            return;
        }
        EAGLEEYE_LOGD("milvus load collection %s", collection.c_str());

        // step 2: 加载collection
        status = m_client->LoadCollection(collection);
        if(!status.IsOk()){
            // 失败加载
            EAGLEEYE_LOGE("Failed to load collection %s (%s:%d)", collection.c_str(), ip.c_str(), port);
            return;
        }

        this->m_collection = collection;
        this->m_ip = ip;
        this->m_port = port;
        this->m_top_k = top_k;
        this->m_mode = MILVUS_QUERY;
        m_is_ready = true;
        m_use_norm = use_norm;

        // 输入端口说明
        // 1: 特征向量:     tensor
        // 2: 组标识:       string
        // 3: id:          string

        // 设置输出端口（用于标记执行成功/失败）
        this->setNumberOfOutputSignals(3);
        this->setOutputPort(new BooleanSignal(), 0);        // 执行是否成功
        this->setOutputPort(new TensorSignal(), 1);         // SCORE: float N x top_k
        this->setOutputPort(new TensorSignal(), 2);         // ID: char N x top_k x 16
    }
    virtual ~MilvusNode(){}

    void setMode(MilvusMode mode){m_mode = mode;};

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo(){
        if(!m_is_ready){
            BooleanSignal* status = (BooleanSignal*)this->getOutputPort(0);
            status->setData(false);
            return;
        }

        // input: group, feature
        // 检索（NxD）
        TensorSignal* feature_sig = (TensorSignal*)this->getInputPort(0);
        Tensor face_features = feature_sig->getData().clone();
        if(face_features.empty()){
            BooleanSignal* status = (BooleanSignal*)this->getOutputPort(0);
            status->setData(false);
            return;
        }

        // 组标识
        std::string group_name = "";
        if(this->getNumberOfInputSignals() >= 2){
            TensorSignal* group_sig = (TensorSignal*)this->getInputPort(1);
            std::vector<std::string> str_list =  group_sig->getString();
            group_name = str_list.size() == 0 ? "" : str_list[0];
        }

        int face_num = face_features.dims()[0];
        int face_dim = face_features.dims()[1];
        float* face_ptr = face_features.cpu<float>();
        if(m_mode == MILVUS_INSERT){
            // 插入
            TensorSignal* ids_sig = (TensorSignal*)this->getInputPort(2);
            std::vector<std::string> insert_ids = ids_sig->getString();

            std::vector<std::vector<float>> insert_vectors;
            for(int face_i=0; face_i<face_num; ++face_i){
                float* face_i_ptr = face_ptr + face_i * face_dim;
                if(m_use_norm){
                    // normlize
                    float vec_len = 0.0f;
                    for(int i=0; i<face_dim; ++i){
                        vec_len += face_i_ptr[i] * face_i_ptr[i];
                    }
                    vec_len = sqrt(vec_len);
                    for(int i=0; i<face_dim; ++i){
                        face_i_ptr[i] /= vec_len;
                    }
                }
                std::vector<float> q_vector(face_i_ptr, face_i_ptr + face_dim);
                insert_vectors.push_back(q_vector);
            }
            if(insert_ids.size() == 0){
                // 基于时间戳分配ID
                for(int face_i=0; face_i<face_num; ++face_i){
                    struct timeval now_tv;
                    gettimeofday(&now_tv,NULL);
                    std::string group_str = std::to_string(now_tv.tv_sec * 1000000 + now_tv.tv_usec);
                    insert_ids.push_back(group_str);
                    // 10 微秒
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }

            std::vector<milvus::FieldDataPtr> fields_data{
                std::make_shared<milvus::VarCharFieldData>("person_id", insert_ids),
                std::make_shared<milvus::FloatVecFieldData>("person_feature", insert_vectors)
            };

            milvus::DmlResults dml_results;
            auto status = m_client->Insert(m_collection, group_name, fields_data, dml_results);
            if(!status.IsOk()){
                BooleanSignal* response = (BooleanSignal*)this->getOutputPort(0);
                response->setData(false);
                return;
            }

            TensorSignal* person_id_sig = (TensorSignal*)this->getOutputPort(2);
            int person_id_dim = 16;
            Tensor person_id_content(
                std::vector<int64_t>{face_num, 1, person_id_dim},
                EAGLEEYE_CHAR,
                DataFormat::AUTO,
                CPU_BUFFER
            );
            person_id_sig->setData(person_id_content);
            for(int face_i=0; face_i<face_num; ++face_i){
                std::string face_id_str = insert_ids[face_i];
                char* face_id_content_ptr = person_id_content.cpu<char>() + face_i * 1 * person_id_dim;
                memcpy(face_id_content_ptr, face_id_str.c_str(), face_id_str.size());
            }
        }
        else if(m_mode == MILVUS_QUERY){
            // 搜索
            milvus::SearchArguments arguments{};
            arguments.SetCollectionName(this->m_collection);
            arguments.AddPartitionName(group_name);
            arguments.AddOutputField("person_id");
            arguments.SetTopK(m_top_k);

            for(int face_i=0; face_i<face_num; ++face_i){
                float* face_i_ptr = face_ptr + face_i * face_dim;
                if(m_use_norm){
                    // normlize
                    float vec_len = 0.0f;
                    for(int i=0; i<face_dim; ++i){
                        vec_len += face_i_ptr[i] * face_i_ptr[i];
                    }
                    vec_len = sqrt(vec_len);
                    for(int i=0; i<face_dim; ++i){
                        face_i_ptr[i] /= vec_len;
                    }
                }
                std::vector<float> q_vector(face_i_ptr, face_i_ptr + face_dim);
                arguments.AddTargetVector("person_feature", std::move(q_vector));
            }

            milvus::SearchResults search_results{};
            m_client->Search(arguments, search_results);

            TensorSignal* person_score_sig = (TensorSignal*)this->getOutputPort(1);
            Tensor person_score_content(
                std::vector<int64_t>{face_num, m_top_k},
                EAGLEEYE_FLOAT,
                DataFormat::AUTO,
                CPU_BUFFER
            );
            person_score_sig->setData(person_score_content);

            TensorSignal* person_id_sig = (TensorSignal*)this->getOutputPort(2);
            int person_id_dim = 16;
            Tensor person_id_content(
                std::vector<int64_t>{face_num, m_top_k, person_id_dim},
                EAGLEEYE_CHAR,
                DataFormat::AUTO,
                CPU_BUFFER
            );
            person_id_sig->setData(person_id_content);
        
            int person_i = 0;
            for (auto& result : search_results.Results()) {
                auto& ids = result.Ids().IntIDArray();  // = top_k
                auto& distances = result.Scores();
                if (ids.size() != distances.size()) {
                    EAGLEEYE_LOGD("Illegal result!");
                    continue;
                }

                auto person_id_field = result.OutputField("person_id");
                milvus::VarCharFieldDataPtr person_ids_ptr = std::static_pointer_cast<milvus::VarCharFieldData>(person_id_field);
                auto& person_ids = person_ids_ptr->Data();

                char* person_id_content_ptr = person_id_content.cpu<char>() + person_i * this->m_top_k * person_id_dim;
                float* person_score_content_ptr = person_score_content.cpu<float>() + person_i * this->m_top_k;
                for(int i=0; i<std::min(int(this->m_top_k), int(person_ids.size())); ++i){
                    person_score_content_ptr[i] = distances[i];
                    memcpy(person_id_content_ptr + i * person_id_dim, person_ids[i].c_str(), person_id_dim);
                }
                person_i += 1;
            }
        }
        else{
            // TODO, 等待实现
        }

        BooleanSignal* status = (BooleanSignal*)this->getOutputPort(0);
        status->setData(true);
    }

private:
    MilvusNode(const MilvusNode&);
    void operator=(const MilvusNode&);

    std::shared_ptr<milvus::MilvusClient> m_client;
    std::string m_collection;
    std::string m_ip;
    int m_port;
    bool m_is_ready;

    MilvusMode m_mode;
    int m_top_k;
    bool m_use_norm;
};
}
#endif