#ifndef _EAGLEEYE_HEFT_H_
#define _EAGLEEYE_HEFT_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/engine/nano/dataflow/schedule.h"
#include <vector>
#include <iostream>

namespace eagleeye{
namespace dataflow{
class ProcessorSlot{
public:
    int start_time;
    int end_time;
    int task_no;
};

class Graph;
class HEFT:public Schedule{
public:
    HEFT(Graph* g, std::vector<EagleeyeRuntime> runtime);
    virtual ~HEFT();

    virtual void collect_statistic();
    virtual void analyze();
    virtual EagleeyeRuntime getRuntime(Node* node);

    float rec_rank(int i){
        float maxi = 0;
        float temp;

        for (int j = 0; j < tasks_num_; j++) {
            if(adj_[i*tasks_num_+j] != -1) {
                temp = rec_rank(j);
                if (maxi < adj_[i*tasks_num_+j] + temp)
                    maxi = adj_[i*tasks_num_+j] + temp;
            }
        }
        rank_[i] = avg_[i] + maxi;
        return rank_[i];
    }

    int est_cal(int i, int j){
        int comm;
        int par;
        int max_time;
        int temp = 0;;

        max_time = 0;
        for (int k = 0; k < parent_[i].size(); k++) {
            par = parent_[i][k];
            if(mini_processor_[par] == j) {
                temp = aft_[par];
            }
            else {
                temp = adj_[par*tasks_num_+i]/B_[mini_processor_[par]*processor_num_+j] + L_[mini_processor_[par]] + aft_[par];
            }

            max_time = eagleeye_max(max_time, temp);
        }

        return max_time;
    }

    int eft_cal(int i, int j){
        int start_time;
        if(processor_scheduler_[j].empty()) {
            return est_[i*processor_num_+j] + task_time_[i*processor_num_+j];
        }
        /*
        for (int k = 0; k < processor_scheduler[j].length() -1 ; k++) {
            if( processor_scheduler[j][k].end_time > est[i][j] and processor_scheduler[j][k+1].start_time > est[i][j] + task_time[i][j]) {
                return est[i][j] + task_time[i][j];
            }

        }

        return processor_scheduler[

        */
        int ok;
        int endtime;
        ok = 1;
        for (int timer = est_[i*processor_num_+j]; 1; timer++) {
            ok = 1;
            endtime = timer + task_time_[i*processor_num_+j];
            for (int k = 0; k < processor_scheduler_[j].size(); k++) {
                if((timer < processor_scheduler_[j][k].start_time && endtime > processor_scheduler_[j][k].start_time) || 
                    (timer < processor_scheduler_[j][k].end_time && endtime > processor_scheduler_[j][k].end_time) ) {
                    ok = 0;
                    break;
                }
            }

            if(ok == 1) {
                return endtime;
            }
        }
        return endtime;
    }

private:
    int* adj_;          // 任务之间数据传输量
    float* rank_;       // 任务排序值
    float* avg_;        // 任务平均处理时间
    int* task_time_;    // 任务执行时间（/处理器）
    int* eft_;          // 最早完成时间 (/处理器)
    int* est_;          // 最早开始时间 (/处理器)
    int* aft_;          // 实际完成时间
    std::vector<std::vector <ProcessorSlot>> processor_scheduler_;  // 处理器任务调度
    std::vector <std::pair<float, int> > rank_task_;
    std::vector<std::vector <int>> parent_;
    int* mini_processor_;   //  每一任务分配的处理器
    int* L_;            // 启动数据传输时间
    int* B_;            // 处理器间传输带宽
    bool* fixed_;
    std::vector<int> fixed_runtime_index_;
};
}
}
#endif