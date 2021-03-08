#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/common/EagleeyeYUV.h"

namespace eagleeye
{
YUVSignal::YUVSignal(){
    m_release_count = 1;
    this->m_yuv_format = EAGLEEYE_YUV_I420;
}   

YUVSignal::~YUVSignal(){

}

void YUVSignal::copyInfo(AnySignal* sig){
    Superclass::copyInfo(sig);
    if(sig == NULL){
        return;
    }

    if((SIGNAL_CATEGORY_IMAGE == (sig->getSignalCategory() & SIGNAL_CATEGORY_IMAGE)) && 
            (this->getSignalValueType() == sig->getSignalValueType())){
        YUVSignal* from_sig = (YUVSignal*)(sig);
        this->m_meta = from_sig->meta();
    }
}

void YUVSignal::copy(AnySignal* sig){
    if(sig == NULL){
        return;
    }

	if((SIGNAL_CATEGORY_IMAGE != (sig->getSignalCategory() & SIGNAL_CATEGORY_IMAGE)) || 
			(this->getSignalValueType() != sig->getSignalValueType())){
		return;
	}

    YUVSignal* from_sig = (YUVSignal*)(sig);
    MetaData from_meta;
    Blob from_data = from_sig->getData(from_meta);
    this->setData(from_data, from_meta);
}

Blob YUVSignal::getData(){
    return this->m_blob;
}

void YUVSignal::setData(Blob data){
    this->m_blob = data;

    modified();
}

Blob YUVSignal::getData(MetaData& m){
    m = this->m_meta;
    return this->m_blob;
}

void YUVSignal::setData(Blob data, MetaData m){
    this->m_blob = data;
    this->m_meta = m;

    modified();
}

void YUVSignal::makeempty(bool auto_empty){
    if(auto_empty){
		if(this->m_release_count % this->getOutDegree() != 0){
			this->m_release_count += 1;
			return;
		}
	}

    this->m_blob = Blob();
	this->m_meta.name = "";
	this->m_meta.info = "";
	this->m_meta.fps = 0.0;
	this->m_meta.nb_frames = 0;
	this->m_meta.frame = 0;
	this->m_meta.is_end_frame = false;
	this->m_meta.is_start_frame = false;
	this->m_meta.rotation = 0;
	this->m_meta.rows = 0;
	this->m_meta.cols = 0;
	this->m_meta.timestamp = 0;

	if(auto_empty){
		// reset count
		this->m_release_count = 1;
	}

	//force time update
	modified();
}

bool YUVSignal::isempty(){
    return this->m_blob.empty();
}

void YUVSignal::setSignalContent(void* data, const int* data_size, const int data_dims, const int data_rotation, bool is_texture){
    // ignore is_texture
    int height = data_size[0];  // rows
    int width = data_size[1];   // cols
    Blob blob(sizeof(unsigned char)*(height*width + (height/2)*(width/2) + (height/2)*(width/2)), 
                    Aligned(64), 
                    EagleeyeRuntime(EAGLEEYE_CPU));
    
    unsigned char* blob_ptr = (unsigned char*)blob.cpu();
    unsigned char* data_ptr = (unsigned char*)data;
    MetaData meta_data = this->meta();
    // 旋转
    if(data_rotation == 0){
        memcpy(blob_ptr, data_ptr, sizeof(unsigned char)*(int)(height*width*1.5));

        meta_data.rows = height;
        meta_data.cols = width;
    } 
    else if(data_rotation == 90){
        eagleeye_I420_rotate_90(data_ptr, width, height, blob_ptr);
        meta_data.rows = width;
        meta_data.cols = height;
    }
    else if(data_rotation == 180){
        eagleeye_I420_rotate_180(data_ptr, width, height, blob_ptr);
        meta_data.rows = height;
        meta_data.cols = width;
    }
    else if(data_rotation == 270){
        eagleeye_I420_rotate_270(data_ptr, width, height, blob_ptr);

        meta_data.rows = width;
        meta_data.cols = height;
    }

    this->setData(blob, meta_data);
}

void YUVSignal::getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type){
    data = this->getData().cpu();
    data_dims = 3;
    data_size[0] = this->m_meta.rows;
    data_size[1] = this->m_meta.cols;
    data_size[2] = 3;
    data_type = this->m_yuv_format;
}

void YUVSignal::setMeta(MetaData meta){
    this->m_meta = meta;
}

void YUVSignal::printUnit(){
    Superclass::printUnit();
}

int YUVSignal::getWidth(){
    return this->m_meta.cols;
}
int YUVSignal::getHeight(){
    return this->m_meta.rows;
}

void YUVSignal::setSignalValueType(EagleeyeType yuv_format){
    if(yuv_format != EAGLEEYE_YUV_I420 && yuv_format != EAGLEEYE_YUV_NV21 && yuv_format != EAGLEEYE_YUV_NV12){
        EAGLEEYE_LOGE("Dont support yuv format %d", (int)(yuv_format));
        return;
    }

    this->m_yuv_format = yuv_format;
}   
} // namespace eagleeye
