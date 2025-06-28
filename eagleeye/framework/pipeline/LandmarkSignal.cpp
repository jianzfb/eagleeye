#include "eagleeye/framework/pipeline/LandmarkSignal.h"

namespace eagleeye
{
LandmarkSignal::LandmarkSignal(Matrix<float> landmark){
    m_landmark = landmark;
    this->setSignalType(EAGLEEYE_SIGNAL_LANDMARK);
}
    
LandmarkSignal::~LandmarkSignal(){

}

void LandmarkSignal::setJoints(Matrix<int> joints){
    m_joints = joints;
}

Matrix<int> LandmarkSignal::getJoints(){
    return m_joints;
}

void LandmarkSignal::copyInfo(AnySignal* sig){
    if(sig == NULL){
        return;
    }

    Superclass::copyInfo(sig);
    if(SIGNAL_CATEGORY_DEFAULT == (sig->getSignalCategory() & SIGNAL_CATEGORY_DEFAULT)){
        if(this->getSignalType() == sig->getSignalType()){
            LandmarkSignal* from_sig = (LandmarkSignal*)(sig);	
            this->setJoints(from_sig->getJoints());
        }
	}
}

void LandmarkSignal::copy(AnySignal* sig, bool is_deep){
    if((SIGNAL_CATEGORY_DEFAULT != (sig->getSignalCategory() & SIGNAL_CATEGORY_DEFAULT)) || (this->getSignalType() != sig->getSignalType())){
		return;
	}
	LandmarkSignal* from_sig = (LandmarkSignal*)(sig);	

	MetaData from_data_meta;
	Matrix<float> from_data = from_sig->getData(from_data_meta, is_deep);
	this->setData(from_data, from_data_meta);
    this->setJoints(from_sig->getJoints());
}

void LandmarkSignal::printUnit(){
    Superclass::printUnit();
}

void LandmarkSignal::makeempty(bool auto_empty){
	m_landmark = Matrix<float>();
    m_joints = Matrix<int>();
	this->m_meta.name = "";
	this->m_meta.fps = 0.0;
	this->m_meta.nb_frames = 0;
	this->m_meta.frame = 0;
	this->m_meta.is_end_frame = false;
	this->m_meta.is_start_frame = false;
	this->m_meta.rotation = 0;
	this->m_meta.rows = 0;
	this->m_meta.cols = 0;
	this->m_meta.timestamp = 0;

	//force time update
	modified();
}

bool LandmarkSignal::isempty(){
    if(this->getSignalCategory() == SIGNAL_CATEGORY_DEFAULT){
        if(m_landmark.rows() == 0 || m_landmark.cols() == 0){
            return true;
        }
    }

    return false;
}

typename LandmarkSignal::DataType LandmarkSignal::getData(bool deep_copy){
    return m_landmark;
}

typename LandmarkSignal::DataType LandmarkSignal::getData(MetaData& mm, bool deep_copy){
    mm = this->m_meta;
    return m_landmark;
}

void LandmarkSignal::setData(typename LandmarkSignal::DataType data){
    this->m_landmark = data;
}

void LandmarkSignal::setMeta(MetaData meta){
    this->m_meta = meta;
}

void LandmarkSignal::setData(typename LandmarkSignal::DataType data, MetaData mm){
    this->m_landmark = data;
    this->m_meta = mm;
}

void LandmarkSignal::setData(void* data, MetaData meta){
    Matrix<float> signal_content(meta.rows, meta.cols, data, true);
    this->setData(signal_content, meta);
}


} // namespace eagleeye
