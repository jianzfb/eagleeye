#include "eagleeye/engine/nano/dataflow/heft.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/common/EagleeyeProcessor.h"
#include "eagleeye/engine/nano/dataflow/graph.hpp"

namespace eagleeye{
namespace dataflow{
HEFT::HEFT(Graph* g, std::vector<EagleeyeRuntime> runtime):
        Schedule(g, runtime){
}  

HEFT::~HEFT(){
    delete[] this->adj_;
    delete[] this->rank_;
    delete[] this->avg_;
    delete[] this->task_time_;
    delete[] this->eft_;
    delete[] this->est_;
    delete[] this->aft_;
    delete[] this->L_;
    delete[] this->B_;    
    delete[] this->mini_processor_;
    delete[] this->fixed_;
}

void HEFT::collect_statistic(){
    // 任务数量
    tasks_num_ = g_->getNodesNum();
    // 处理器数量
    processor_num_ = runtime_.size();

    // 任务之间的传输数据量
    this->adj_ = new int[tasks_num_*tasks_num_];  
    memset(this->adj_, -1, sizeof(int)*tasks_num_*tasks_num_);
    // 任务排序值
    this->rank_ = new float[tasks_num_];
    // 任务平均计算时间
    this->avg_ = new float[tasks_num_];
    // 任务计算时间
    this->task_time_ = new int[tasks_num_*processor_num_];
    // 最早完成时间
    this->eft_ = new int[tasks_num_*processor_num_];
    // 最早开始时间
    this->est_ = new int[tasks_num_*processor_num_];
    // 实际完成时间
    this->aft_ = new int[tasks_num_];  
    memset(this->aft_, 0, sizeof(int)*tasks_num_);
    // 处理器启动数据传输时间
    this->L_ = new int[processor_num_];
    // 处理器传输数据带宽
    this->B_ = new int[processor_num_*processor_num_];
    // 任务分配处理器
    this->mini_processor_ = new int[tasks_num_];
    // 记录每一处理器上的任务队列
    processor_scheduler_.reserve(processor_num_);   
    // 每一任务的predecessor
    parent_.reserve(tasks_num_);
    // 是否在固定处理器上
    fixed_ = new bool[tasks_num_];
    memset(fixed_, 0, sizeof(bool)*tasks_num_);
    fixed_runtime_index_.reserve(tasks_num_);

    // 1.step 分析任务之间的数据传输量
    std::unordered_set<Edge*> edges = g_->getEdges();
    std::unordered_set<Edge*>::iterator eiter, eiend(edges.end());
    for(eiter = edges.begin(); eiter != eiend; ++eiter){
        Node& from_node = (*eiter)->prev();
        Node& to_node = (*eiter)->next();

        adj_[from_node.getId() * tasks_num_ + to_node.getId()] = from_node.size(to_node.linkIndexOfNode(from_node));
        parent_[to_node.getId()].push_back(from_node.getId());
    }

    EAGLEEYE_LOGD("data transfer matrix");
    for(int i=0; i<tasks_num_; ++i){
        for(int j=0; j<tasks_num_; ++j){
            std::cout<<adj_[i*tasks_num_+j]<<"\t";
        }
        std::cout<<"\n";
    }

    // 2.step 分析任务在不同处理器上的运行时间(/us)
    std::unordered_set<Node*> nodes = g_->getNodes();
    std::unordered_set<Node*>::iterator niter, niend(nodes.end());
    memset(avg_, 0, sizeof(float)*tasks_num_);
    for(int j=0; j<processor_num_; ++j){
        for(niter = nodes.begin(); niter != niend; ++niter){
            float time = (*niter)->fire(runtime_[j]);
            task_time_[(*niter)->getId()*processor_num_+j] = time;
            avg_[(*niter)->getId()] += time;
        }
    }
    for(niter = nodes.begin(); niter != niend; ++niter){
        avg_[(*niter)->getId()] /= processor_num_;

        EAGLEEYE_LOGD("task %d avg time %f us", (*niter)->getId(), avg_[(*niter)->getId()]);
    }

    // 3.step 分析处理器性能
    // L_ 启动数据传输的时间，B_ 传输数据的带宽
    for(int m=0; m<processor_num_; ++m){
        L_[m] = Processor::getL(this->runtime_[m]);
        for(int n=0; n<processor_num_; ++n){
            if(m == n){
                // 带宽无限
                B_[m*processor_num_ + n] = 1e6;
                continue;
            }
            B_[m*processor_num_ + n] = Processor::getB(this->runtime_[m], this->runtime_[n]);
            // transfer from MB/s to B/us
            B_[m*processor_num_ + n] = int(B_[m*processor_num_ + n] * ((1024.0f * 1024.0f) / (1000.0f*1000.0f)));
            EAGLEEYE_LOGD("bandwidth %d B/us from %d to %d", B_[m*processor_num_ + n], m, n);
        }
    }

    // 4.step 任务排序
    for(niter = nodes.begin(); niter != niend; ++niter){
        this->rec_rank((*niter)->getId());
    }
        
    for(niter = nodes.begin(); niter != niend; ++niter){
        rank_task_.push_back(std::make_pair(rank_[(*niter)->getId()], (*niter)->getId()));
    }

    sort(rank_task_.rbegin(), rank_task_.rend());
    for(int i=0; i<rank_task_.size(); ++i){
        std::cout<<"task "<<rank_task_[i].second<<" rank "<<rank_task_[i].first<<std::endl;
    }

    // 5.step 获得任务是否有固定处理器选择
    for(niter = nodes.begin(); niter != niend; ++niter){
        fixed_[(*niter)->getId()] = (*niter)->isFixedOnRuntime();
        if(fixed_[(*niter)->getId()]){
            for(int i=0; i<processor_num_; ++i){
                if(runtime_[i].type() == (*niter)->getRuntime().type()){
                    fixed_runtime_index_[(*niter)->getId()] = i;
                    break;
                }
            }
        }        
    }
}

void HEFT::analyze(){
    int mini;
    mini = INT_MAX;
    int t;
    int mini_eft;
    int length = -1;

    ProcessorSlot temp;
    ProcessorSlot temp2;
    t = rank_task_[0].second;
    for (int i = 0; i < processor_num_; i++) {
        est_[rank_task_[0].second*processor_num_+i] = 0;
        if(mini > task_time_[t*processor_num_+i]) {
            mini = task_time_[t*processor_num_+i];
            mini_processor_[t] = i;
        }
    }
    if(fixed_[t]){
        mini_processor_[t] = fixed_runtime_index_[t];
        mini = task_time_[t*processor_num_+fixed_runtime_index_[t]];
    }
    EAGLEEYE_LOGD("task %d schedule on %d processor", t, mini_processor_[t]);

    temp.start_time = 0;
    temp.end_time = mini;
    temp.task_no = t;
    processor_scheduler_[mini_processor_[t]].push_back(temp);

    aft_[rank_task_[0].second] = mini;
    int i;
    for (int h = 1; h < tasks_num_; h++) {
        i = rank_task_[h].second;

        for (int j = 0; j < processor_num_; j++) {
            est_[i*processor_num_+j] = est_cal(i, j);
            EAGLEEYE_LOGD("task %d est %d on processor %d", i, est_[i*processor_num_+j], j);
        }
    
        mini_eft = INT_MAX;
        int processor_no = 0;

        for (int j = 0; j < processor_num_; j++) {
            eft_[i*processor_num_+j] = eft_cal(i, j);
            EAGLEEYE_LOGD("task %d eft %d on processor %d", i, eft_[i*processor_num_+j], j);
            if(eft_[i*processor_num_+j] < mini_eft) {
                mini_eft = eft_[i*processor_num_+j];
                processor_no = j;
            }
        }
        
        mini_processor_[i] = processor_no;
        if(fixed_[i]){
            processor_no = fixed_runtime_index_[t];
            mini_processor_[t] = processor_no;
            mini_eft = eft_[i*processor_num_+processor_no];
        }
        EAGLEEYE_LOGD("task %d schedule on %d processor", i, processor_no);

        temp2.start_time = mini_eft - task_time_[i*processor_num_+processor_no];
        temp2.end_time = mini_eft;
        aft_[i] = mini_eft;
        temp2.task_no = i;
        length = eagleeye_max(mini_eft, length);

        processor_scheduler_[processor_no].push_back(temp2);
    }    
}

EagleeyeRuntime HEFT::getRuntime(Node* node){
    return runtime_[mini_processor_[node->getId()]];
}
} // namespace dataflow    
}