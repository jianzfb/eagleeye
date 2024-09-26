#include "eagleeye/processnode/PlaceholderQueue.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/BooleanSignal.h"
#include "eagleeye/common/EagleeyeMacro.h"
namespace eagleeye{
PlaceholderQueue::PlaceholderQueue(int queue_size){
    this->m_queue_size = queue_size;
}   
PlaceholderQueue::~PlaceholderQueue(){

} 

void PlaceholderQueue::config(int placeholder_i, std::string data_type, std::string data_category){
    AnySignal* out_signal = NULL;
    int data_category_n = std::stoi(data_category);
    if(data_category_n == EAGLEEYE_SIGNAL_IMAGE ||
        data_category_n == EAGLEEYE_SIGNAL_RGB_IMAGE ||
        data_category_n == EAGLEEYE_SIGNAL_BGR_IMAGE){
        out_signal = new ImageSignal<Array<unsigned char, 3>>();
    }
    else if(data_category_n == EAGLEEYE_SIGNAL_RGBA_IMAGE ||
            data_category_n == EAGLEEYE_SIGNAL_BGRA_IMAGE){
        out_signal = new ImageSignal<Array<unsigned char, 4>>();
    }
    else if(data_category_n == EAGLEEYE_SIGNAL_GRAY_IMAGE ||
            data_category_n == EAGLEEYE_SIGNAL_MASK || 
            data_category_n == EAGLEEYE_SIGNAL_UCMATRIX){
        out_signal = new ImageSignal<unsigned char>();
    }
    else if(data_category_n == EAGLEEYE_SIGNAL_DET ||
            data_category_n == EAGLEEYE_SIGNAL_DET_EXT ||
            data_category_n == EAGLEEYE_SIGNAL_TRACKING ||
            data_category_n == EAGLEEYE_SIGNAL_POS_2D ||
            data_category_n == EAGLEEYE_SIGNAL_POS_3D ||
            data_category_n == EAGLEEYE_SIGNAL_LANDMARK || 
            data_category_n == EAGLEEYE_SIGNAL_FMATRIX){
        out_signal = new ImageSignal<float>();
    }
    else if(data_category_n == EAGLEEYE_SIGNAL_CLS ||
            data_category_n == EAGLEEYE_SIGNAL_STATE ||
            data_category_n == EAGLEEYE_SIGNAL_IMATRIX){
        out_signal = new ImageSignal<int>();
    }
    else if(data_category_n == EAGLEEYE_SIGNAL_SWITCH){
        out_signal = new BooleanSignal();
    }
    else if(data_category_n == EAGLEEYE_SIGNAL_RECT ||
            data_category_n == EAGLEEYE_SIGNAL_LINE ||
            data_category_n == EAGLEEYE_SIGNAL_POINT){
        out_signal = new ImageSignal<float>();
    }
    else if(data_category_n == EAGLEEYE_SIGNAL_TIMESTAMP){
        out_signal = new ImageSignal<double>();
    }
    else{
        EAGLEEYE_LOGE("Dont support signal type %s (placeholder %d).", data_category.c_str(), placeholder_i);
    }

    // 切换到队列
    out_signal->transformCategoryToQ(m_queue_size);

    if(this->getNumberOfOutputSignals() < placeholder_i+1){
        this->setNumberOfOutputSignals(placeholder_i+1);
    }
    this->setOutputPort(out_signal, placeholder_i);
}

void PlaceholderQueue::push(int placeholder_i, void* data, const size_t* data_size, const int data_dims, const int data_rotation, const int data_type){
    AnySignal* signal = this->getOutputPort(placeholder_i);

    MetaData meta;
    meta.rows = data_size[0];
    meta.cols = data_size[1];
    meta.rotation = data_rotation;
    // memory copy mode
    signal->setData(data, meta);
}

void PlaceholderQueue::postexit(){
    ImageSignal<Array<unsigned char, 3>>* out_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
    MetaData meta;
    out_sig->setData(Matrix<Array<unsigned char, 3>>(0,0), meta);
}
}