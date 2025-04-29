#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/common/EagleeyeYUV.h"

namespace eagleeye
{
YUVSignal::YUVSignal(){
    this->m_yuv_format = EAGLEEYE_YUV_I420;
    this->setSignalType(EAGLEEYE_SIGNAL_YUV_IMAGE);
}   

YUVSignal::~YUVSignal(){
}

void YUVSignal::copy(AnySignal* sig){
    if(sig == NULL){
        return;
    }

	if((SIGNAL_CATEGORY_IMAGE != (sig->getSignalCategory() & SIGNAL_CATEGORY_IMAGE)) || 
			(this->getValueType() != sig->getValueType())){
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

	//force time update
	modified();
}

bool YUVSignal::isempty(){
    return this->m_blob.empty();
}

void YUVSignal::setData(void* data, MetaData meta){
    // const int* data_size, const int data_dims, const int data_rotation

    // ignore is_texture
    int height = meta.rows;  // rows
    int width = meta.cols;   // cols
    Blob blob((height*width + (height/2)*(width/2) + (height/2)*(width/2)), 
                TypeTrait<unsigned char>::type,
                CPU_BUFFER,
                Aligned(64));
    
    unsigned char* blob_ptr = blob.cpu<unsigned char>();
    unsigned char* data_ptr = (unsigned char*)data;
    MetaData meta_data = meta;
    // 旋转
    if(meta.rotation == 0){
        memcpy(blob_ptr, data_ptr, sizeof(unsigned char)*(int)(height*width*1.5));

        meta_data.rows = height;
        meta_data.cols = width;
    } 
    else if(meta.rotation == 90){
        EAGLEEYE_LOGD("in rotate 90");
        I420_rotate_90(data_ptr, width, height, blob_ptr);
        meta_data.rows = width;
        meta_data.cols = height;
        EAGLEEYE_LOGD("after width %d height %d", meta_data.cols, meta_data.rows);
    }
    else if(meta.rotation == 180){
        EAGLEEYE_LOGD("in rotate 180");
        EAGLEEYE_LOGD("before width %d height %d", width, height);
        I420_rotate_180(data_ptr, width, height, blob_ptr);
        meta_data.rows = height;
        meta_data.cols = width;
        EAGLEEYE_LOGD("after width %d height %d", meta_data.cols, meta_data.rows);
    }
    else if(meta.rotation == 270){
        EAGLEEYE_LOGD("in rotate 270");
        I420_rotate_270(data_ptr, width, height, blob_ptr);
        meta_data.rows = width;
        meta_data.cols = height;
        EAGLEEYE_LOGD("after width %d height %d", meta_data.cols, meta_data.rows);
    }

    this->setData(blob, meta_data);
}

void YUVSignal::getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type){
    data = this->getData().cpu();
    data_dims = 3;
    this->m_data_size[0] = this->m_meta.rows;
    this->m_data_size[1] = this->m_meta.cols;
    this->m_data_size[2] = 3;
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

void YUVSignal::setValueType(EagleeyeType yuv_format){
    if(yuv_format != EAGLEEYE_YUV_I420 && yuv_format != EAGLEEYE_YUV_NV21 && yuv_format != EAGLEEYE_YUV_NV12){
        EAGLEEYE_LOGE("Dont support yuv format %d", (int)(yuv_format));
        return;
    }

    this->m_yuv_format = yuv_format;
}   
} // namespace eagleeye
