#include "eagleeye/processnode/SnapshotNode.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "libyuv.h"

#ifdef EAGLEEYE_RKCHIP
#include "im2d_version.h"
#include "rk_type.h"
#include "rk_mpi.h"
#include "RgaUtils.h"
#include "im2d_buffer.h"
#include "im2d_type.h"
#include "im2d_single.h"
#include "utils/utils.h"
#include "osal/inc/mpp_env.h"
#include "osal/inc/mpp_mem.h"
#include "osal/inc/mpp_common.h"
#include "utils/mpi_enc_utils.h"
#include "mpp_rc_api.h"
#endif

namespace eagleeye{
SnapeshotNode::SnapeshotNode(){
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>(), 0);
    m_is_init = false;
    this->m_image_format = 1;   // 默认BGR

    EAGLEEYE_MONITOR_VAR(std::string, setFilePath, getFilePath, "file","","");
    EAGLEEYE_MONITOR_VAR(std::string, setPrefix, getPrefix, "prefix","","");
    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder","","");

    m_pkt_buf = NULL;
    m_frame_buf = NULL;
    m_md_info = NULL;    
    m_buf_grp = NULL;
    m_mpp_ctx = NULL;
    m_mpp_api = NULL;

    m_frame_size = 0;
    m_header_size = 0;
}

SnapeshotNode::~SnapeshotNode(){
#ifdef EAGLEEYE_RKCHIP
    if(m_pkt_buf != NULL){
        mpp_buffer_put((MppBuffer)(m_pkt_buf));
    }
    if(m_frame_buf != NULL){
        mpp_buffer_put((MppBuffer)(m_frame_buf));
    }
    if(m_md_info != NULL){
        mpp_buffer_put((MppBuffer)(m_md_info));
    }
    if(m_buf_grp != NULL){
        mpp_buffer_group_put((MppBufferGroup)(m_buf_grp));
    }
    if(m_mpp_ctx != NULL){
        MppCtx mpp_ctx = (MppCtx)m_mpp_ctx;
        MppApi* mpp_api = (MppApi*)m_mpp_api;

        mpp_api->reset(mpp_ctx);
        mpp_destroy(mpp_ctx);
    }
#endif
}

#ifdef EAGLEEYE_RKCHIP
extern MPP_RET mpp_enc_cfg_setup(MppEncCfg  cfg, MppApi* mpi, MppCtx ctx, int frame_width, int frame_height, int frame_elem_size, int frame_pix_fmt, MppCodingType type);
#endif

void SnapeshotNode::executeNodeInfo(){
    // 支持RGB/BGR, RGBA/BGRA
    int image_h = 0;
    int image_w = 0;
    int elem_size = 3;
    unsigned char* image_ptr = NULL;
    MetaData image_meta_data;
    if(m_image_format == 0 || m_image_format == 1){
        ImageSignal<Array<unsigned char,3>>* input_img_signal = 
                        (ImageSignal<Array<unsigned char,3>>*)(this->getInputPort(0));
        Matrix<Array<unsigned char,3>> c3_image = input_img_signal->getData(image_meta_data);
        if(!c3_image.isContinuous()){
            c3_image = c3_image.clone();
        }

        image_h = c3_image.rows();
        image_w = c3_image.cols();
        m_c4_image = Matrix<Array<unsigned char,4>>(image_h, image_w);
        unsigned char* c3_ptr = c3_image.cpu<unsigned char>();
        unsigned char* c4_ptr = m_c4_image.cpu<unsigned char>();
        for(int i=0; i<image_h; ++i){
            unsigned char* c3_row_ptr = c3_ptr + i*image_w*3;
            unsigned char* c4_row_ptr = c4_ptr + i*image_w*4;
            for(int j=0; j<image_w; ++j){
                c4_row_ptr[j*4] = c3_row_ptr[j*3];
                c4_row_ptr[j*4+1] = c3_row_ptr[j*3+1];
                c4_row_ptr[j*4+2] = c3_row_ptr[j*3+2];
                c4_row_ptr[j*4+3] = 0;
            }
        }
        image_ptr = c4_ptr;
        elem_size = 4;
    }
    else{
        ImageSignal<Array<unsigned char,4>>* input_img_signal = 
                        (ImageSignal<Array<unsigned char,4>>*)(this->getInputPort(0));
        m_c4_image = input_img_signal->getData(image_meta_data);
        if(!m_c4_image.isContinuous()){
            m_c4_image = m_c4_image.clone();
        }

        image_h = m_c4_image.rows();
        image_w = m_c4_image.cols();
        elem_size = 4;
        image_ptr = m_c4_image.cpu<unsigned char>();
    }

    if(!image_meta_data.is_snapshot_frame){
        // no snapshot, directly return
        this->getOutputPort(0)->meta().is_snapshot_frame = false;
        return;
    }

    // snapshot
    this->getOutputPort(0)->meta().is_snapshot_frame = true;

    // 设置文件名
    if(image_meta_data.name != ""){
        // 检查目录
        if(this->m_folder != "./"){
            if(!isdirexist(this->m_folder.c_str())){
                createdirectory(this->m_folder.c_str());
            }
        }

        this->m_file_path = this->m_folder+this->m_prefix + std::string(image_meta_data.name) +".jpg";
    }


    if(this->m_file_path.empty()){
        // 检查目录
        if(this->m_folder != "./"){
            if(!isdirexist(this->m_folder.c_str())){
                createdirectory(this->m_folder.c_str());
            }
        }

        // 动态生成唯一文件名
        this->m_file_path = this->m_folder+this->m_prefix + EagleeyeTime::getTimeStamp()+".jpg";
    }

    // open file
    m_output_file.open(m_file_path.c_str(), std::ios::binary);

    if(!this->m_is_init){
#ifdef EAGLEEYE_RKCHIP
        /*
        * Can use packet with normal malloc buffer as input not pkt_buf.
        * Please refer to vpu_api_legacy.cpp for normal buffer case.
        * Using pkt_buf buffer here is just for simplifing demo.
        */
        MppBufferGroup buf_grp = nullptr;
        mpp_buffer_group_get_internal(&buf_grp, MPP_BUFFER_TYPE_DRM);
        MppBuffer pkt_buf;
        m_frame_size = MPP_ALIGN(image_w*elem_size, 64) * MPP_ALIGN(image_h, 64);
        mpp_buffer_get(buf_grp, &pkt_buf, m_frame_size);
        m_pkt_buf = pkt_buf;

        MppBuffer frame_buf;
        MppFrameFormat fmt;
        if(m_image_format == 2 || m_image_format == 0){
            //RGBA
            fmt = MPP_FMT_RGBA8888;
        }
        else{
            //BGRA
            fmt = MPP_FMT_BGRA8888;
        }

        m_header_size = 0;
        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            if ((fmt & MPP_FRAME_FBC_MASK) == MPP_FRAME_FBC_AFBC_V1)
                m_header_size = MPP_ALIGN(MPP_ALIGN(image_w, 16) * MPP_ALIGN(image_h, 16) / 16, SZ_4K);
            else
                m_header_size = MPP_ALIGN(image_w, 16) * MPP_ALIGN(image_h, 16) / 16;
        }
        mpp_buffer_get(buf_grp, &frame_buf, m_frame_size + m_header_size);
        m_frame_buf = frame_buf;

        MppBuffer md_info;
        int mdinfo_size = (MPP_VIDEO_CodingHEVC == MPP_VIDEO_CodingMJPEG) ?
                      (MPP_ALIGN(image_w*elem_size, 32) >> 5) *
                      (MPP_ALIGN(image_h, 32) >> 5) * 16 :
                      (MPP_ALIGN(image_w*elem_size, 64) >> 6) *
                      (MPP_ALIGN(image_h, 16) >> 4) * 16;
        mpp_buffer_get(buf_grp, &md_info, mdinfo_size);
        m_md_info = md_info;

        MppCtx mpp_ctx;
        MppApi* mpp_api;
        MPP_RET ret = MPP_OK;
        ret = mpp_create(&mpp_ctx, &mpp_api);
        if (MPP_OK != ret) {
            EAGLEEYE_LOGE("MPP mpp_create failure.");
        }

        MppPollType timeout = MPP_POLL_BLOCK;
        ret = mpp_api->control(mpp_ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
        if (MPP_OK != ret) {
            EAGLEEYE_LOGE("mpi control set output timeout %d ret %d", timeout, ret);
            return;
        }

        ret = mpp_init(mpp_ctx, MPP_CTX_ENC, MPP_VIDEO_CodingMJPEG);
        if (ret) {
            EAGLEEYE_LOGE("mpp_init failed ret %d", ret);
            return;
        }

        MppEncCfg cfg;
        ret = mpp_enc_cfg_init(&cfg);
        if (ret) {
            EAGLEEYE_LOGE("mpp_enc_cfg_init failed ret %d", ret);
            return;
        }

        ret = mpp_api->control(mpp_ctx, MPP_ENC_GET_CFG, cfg);
        if (ret) {
            EAGLEEYE_LOGE("get enc cfg failed ret %d", ret);
            return;
        }

        if(this->m_image_format == 2 || this->m_image_format == 0){
            ret = mpp_enc_cfg_setup(cfg, mpp_api, mpp_ctx, image_w, image_h, elem_size, 2, MPP_VIDEO_CodingMJPEG);
        }
        else{
            ret = mpp_enc_cfg_setup(cfg, mpp_api, mpp_ctx, image_w, image_h, elem_size, 3, MPP_VIDEO_CodingMJPEG);
        }
        if (ret) {
            EAGLEEYE_LOGE("mpp setup failed ret %d", ret);
            return;
        }

        m_mpp_ctx = mpp_ctx;
        m_mpp_api = mpp_api;
#endif

        m_is_init = true;
    }

#ifdef EAGLEEYE_RKCHIP
    // RGB/BGR， RGBA/BGRA
    MppCtx mpp_ctx = (MppCtx)(m_mpp_ctx);
    MppApi* mpp_api = (MppApi*)m_mpp_api;
    MppPacket packet = NULL;
    MppBuffer pkt_buf = (MppBuffer)m_pkt_buf;
    MPP_RET ret = MPP_OK;

    MppMeta meta = NULL;
    MppFrame frame = NULL;
    MppBuffer frame_buf = (MppBuffer)(m_frame_buf);
    void *buf = mpp_buffer_get_ptr(frame_buf);
    RK_U32 eoi = 1;
    memcpy(buf, image_ptr, image_h*image_w*elem_size);

    // 初始化frame
    ret = mpp_frame_init(&frame);
    if (ret) {
        mpp_err_f("mpp_frame_init failed\n");
        return;
    }

    mpp_frame_set_width(frame, image_w);
    mpp_frame_set_height(frame, image_h);
    mpp_frame_set_hor_stride(frame, image_w*elem_size);
    mpp_frame_set_ver_stride(frame, image_h);
    if(m_image_format == 2 || m_image_format == 0){
        // RGBA
        mpp_frame_set_fmt(frame, MPP_FMT_RGBA8888);
    }
    else{
        // BGRA
        mpp_frame_set_fmt(frame, MPP_FMT_BGRA8888);
    }
    mpp_frame_set_eos(frame, 0);
    mpp_frame_set_buffer(frame, frame_buf);

    meta = mpp_frame_get_meta(frame);
    mpp_packet_init_with_buffer(&packet, pkt_buf);
    /* NOTE: It is important to clear output packet length!! */
    mpp_packet_set_length(packet, 0);
    mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
    mpp_meta_set_buffer(meta, KEY_MOTION_INFO, m_md_info);

    /*
        * NOTE: in non-block mode the frame can be resent.
        * The default input timeout mode is block.
        *
        * User should release the input frame to meet the requirements of
        * resource creator must be the resource destroyer.
        */
    ret = mpp_api->encode_put_frame(mpp_ctx, frame);
    if (ret) {
        mpp_frame_deinit(&frame);
        return;
    }
    mpp_frame_deinit(&frame);

    do {
        ret = mpp_api->encode_get_packet(mpp_ctx, &packet);
        if (ret) {
            return;
        }

        if (packet) {
            // write packet to file here
            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);
            m_output_file.write((char*)ptr, len);

            /* for low delay partition encoding */
            if (mpp_packet_is_partition(packet)) {
                eoi = mpp_packet_is_eoi(packet);
            }
            mpp_packet_deinit(&packet);
        }
    } while (!eoi);

    m_output_file.close();
#endif
}

void SnapeshotNode::setImageFormat(int image_format){
    // image_format: 0: RGB; 1: BGR, 2: RGBA, 3: BGRA
    if(image_format > 3){
        EAGLEEYE_LOGE("image format only support 0(RGB),1(BGR),2(RGBA),3(BGRA)");
        return;
    }

    this->m_image_format = image_format;
}

void SnapeshotNode::setFilePath(std::string file_path){
    this->m_file_path = file_path;
}

void SnapeshotNode::getFilePath(std::string& file_path){
    file_path = this->m_file_path;
}

void SnapeshotNode::setPrefix(std::string prefix){
    this->m_prefix = prefix;
}

void SnapeshotNode::getPrefix(std::string& prefix){
    prefix = this->m_prefix;
}

void SnapeshotNode::setFolder(std::string folder){
    this->m_folder = folder;
    if(!endswith(this->m_folder, "/")){
        this->m_folder = folder +"/";
    }
}
void SnapeshotNode::getFolder(std::string& folder){
    folder = this->m_folder;
}
}