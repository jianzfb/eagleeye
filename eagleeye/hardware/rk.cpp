#include "eagleeye/hardware/rk.h"
#include <unistd.h> 
#include <fstream>
#ifdef EAGLEEYE_RKCHIP
#include "im2d_version.h"
#include "rk_type.h"
#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_frame.h"
#include "mpp_err.h"
#include "RgaUtils.h"
#include "im2d_buffer.h"
#include "im2d_type.h"
#include "im2d_single.h"

const int PACKAGE_SIZE = 128*1024;    // 1M

namespace eagleeye{
RKH264Decoder::RKH264Decoder(){
    // 初始化
    initial();

    m_cache_buf = (uint8_t*)malloc(PACKAGE_SIZE*sizeof(uint8_t));
    m_cache_offset = 0;
}

RKH264Decoder::~RKH264Decoder(){
    // 销毁
    destroy();

    free(m_cache_buf);
}

int RKH264Decoder::initial(){
    MppCtx mpp_ctx;
    MppApi* mpp_api;
    MPP_RET mpp_ret = mpp_create(&mpp_ctx, &mpp_api);
    if (MPP_OK != mpp_ret) {
        EAGLEEYE_LOGE("MPP mpp_create failure.");
    }

    // 不分帧模式
    RK_U32 need_split = 1;
    mpp_ret = mpp_init(mpp_ctx, MPP_CTX_DEC, MPP_VIDEO_CodingAVC);
    if (mpp_ret != MPP_OK) {
        EAGLEEYE_LOGE("MPP mpp_init failure.");
    }

    MppDecCfg cfg       = NULL;
    mpp_dec_cfg_init(&cfg);
    /* get default config from decoder context */
    mpp_ret = mpp_api->control(mpp_ctx, MPP_DEC_GET_CFG, cfg);
    if (mpp_ret) {
        EAGLEEYE_LOGE("failed to get decoder cfg ret %d\n", mpp_ret);
    }

    /*
     * split_parse is to enable mpp internal frame spliter when the input
     * packet is not aplited into frames.
     */
    mpp_ret = mpp_dec_cfg_set_u32(cfg, "base:split_parse", need_split);
    if (mpp_ret) {
        EAGLEEYE_LOGE("failed to set split_parse ret %d\n", mpp_ret);
    }

    mpp_ret = mpp_api->control(mpp_ctx, MPP_DEC_SET_CFG, cfg);
    if (mpp_ret) {
        EAGLEEYE_LOGE("failed to set cfg %p ret %d\n", cfg, mpp_ret);
    }

    m_mpp_ctx = mpp_ctx;
    m_mpp_api = mpp_api;
    m_frm_grp = NULL;
    return 0;
}

void RKH264Decoder::destroy(){
    // 销毁RK资源
    if (m_frm_grp != NULL) {
        mpp_buffer_group_put(m_frm_grp);
        m_frm_grp = NULL;
    }

    MppCtx mpp_ctx = (MppCtx)m_mpp_ctx;
    MppApi* mpp_api = (MppApi*)m_mpp_api;
    mpp_api->reset(mpp_ctx);
    mpp_destroy(mpp_ctx);
}

int RKH264Decoder::decode(uint8_t* package_data, int package_size, std::vector<Matrix<Array<unsigned char, 3>>>& image_list){
    // 填充缓冲buf，直到满足PACKAGE_SIZE
    if(m_cache_offset + package_size < PACKAGE_SIZE){
        memcpy(m_cache_buf+m_cache_offset, package_data, package_size*sizeof(uint8_t));
        m_cache_offset = m_cache_offset + package_size;
        return 0;
    }

    EAGLEEYE_LOGD("Acculate enough package");

    int remain_size = (m_cache_offset + package_size) - PACKAGE_SIZE;
    int copy_size = package_size - remain_size;
    memcpy(m_cache_buf+m_cache_offset, package_data, copy_size*sizeof(uint8_t));        
    m_cache_offset = m_cache_offset + copy_size;

    image_list.clear();
    MppCtx mpp_ctx = (MppCtx)m_mpp_ctx;
    MppApi* mpp_api = (MppApi*)m_mpp_api;
    MppPacket mpp_packet = NULL;
    mpp_packet_init(&mpp_packet, m_cache_buf, PACKAGE_SIZE);

    //  write data to packet
    // mpp_packet_write(mpp_packet, 0, data, size);
    //  reset pos and set valid length
    // mpp_packet_set_pos(mpp_packet, data);
    // mpp_packet_set_length(mpp_packet, size);
    // mpp_packet_set_eos(mpp_packet);

    RK_U32 pkt_done = 0;
    RK_U32 ret = 0;
    do {
        // send the packet first if packet is not done
        RK_S32 times = 5;
        RK_U32 frm_eos = 0;
    
        if (!pkt_done) {
            ret = mpp_api->decode_put_packet(mpp_ctx, mpp_packet);
            if (MPP_OK == ret){
                pkt_done = 1;
            }
        }

        do{
            MppFrame mpp_frame = NULL;
            RK_S32 get_frm = 0;            
            ret = mpp_api->decode_get_frame(mpp_ctx, &mpp_frame);
            if (ret == MPP_ERR_TIMEOUT) {
                if(times > 0){
                    times --;
                    usleep(1000);
                    continue;
                }
            }
            if (ret) {
                EAGLEEYE_LOGE("%p decode_get_frame failed ret %d\n", ret, mpp_ctx);
                break;
            }

            if(mpp_frame){
                if (mpp_frame_get_info_change(mpp_frame)) {
                    // 首次解码会进入这里
                    RK_U32 width = mpp_frame_get_width(mpp_frame);
                    RK_U32 height = mpp_frame_get_height(mpp_frame);
                    RK_U32 hor_stride = mpp_frame_get_hor_stride(mpp_frame);
                    RK_U32 ver_stride = mpp_frame_get_ver_stride(mpp_frame);
                    RK_U32 buf_size = mpp_frame_get_buf_size(mpp_frame);

                    EAGLEEYE_LOGD("decode_get_frame get info changed found");
                    EAGLEEYE_LOGD("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
                            width, height, hor_stride, ver_stride, buf_size);

                    if (NULL == this->m_frm_grp) {
                        /* If buffer group is not set create one and limit it */
                        ret = mpp_buffer_group_get_internal(&this->m_frm_grp, MPP_BUFFER_TYPE_ION);
                        if (ret) {
                            EAGLEEYE_LOGE("get mpp buffer group failed ret %d", ret);
                            break;
                        }

                        /* Set buffer to mpp decoder */
                        ret = mpp_api->control(mpp_ctx, MPP_DEC_SET_EXT_BUF_GROUP, this->m_frm_grp);
                        if (ret) {
                            EAGLEEYE_LOGE("set buffer group failed ret %d", ret);
                            break;
                        }
                    } else {
                        /* If old buffer group exist clear it */
                        ret = mpp_buffer_group_clear(this->m_frm_grp);
                        if (ret) {
                            EAGLEEYE_LOGE("clear buffer group failed ret %d", ret);
                            break;
                        }
                    }

                    /* Use limit config to limit buffer count to 24 with buf_size */
                    ret = mpp_buffer_group_limit_config(this->m_frm_grp, buf_size, 24);
                    if (ret) {
                        EAGLEEYE_LOGE("limit buffer group failed ret %d", ret);
                        break;
                    }

                    mpp_api->control(mpp_ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                    mpp_frame_deinit(&mpp_frame);
                    continue;
                }

                RK_U32 err_info = mpp_frame_get_errinfo(mpp_frame) | mpp_frame_get_discard(mpp_frame);
                if (err_info) {
                    // 解码错误
                    EAGLEEYE_LOGE("decoder_get_frame get err info:%d discard:%d.\n",
                            mpp_frame_get_errinfo(mpp_frame), mpp_frame_get_discard(mpp_frame));
                    mpp_frame_deinit(&mpp_frame);
                    continue;
                }

                frm_eos = mpp_frame_get_eos(mpp_frame);
                get_frm = 1;
                // color space transform
                {
                    RK_U32 src_width    = mpp_frame_get_width(mpp_frame);
                    RK_U32 src_height   = mpp_frame_get_height(mpp_frame);
                    RK_U32 src_h_stride = mpp_frame_get_hor_stride(mpp_frame);
                    RK_U32 src_v_stride = mpp_frame_get_ver_stride(mpp_frame);
                    MppBuffer src_buf   = mpp_frame_get_buffer(mpp_frame);
                    RK_S64 pts = mpp_frame_get_pts(mpp_frame);

                    int src_format = RK_FORMAT_YCbCr_420_SP;
                    int src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
                    rga_buffer_t src_img, dst_img;
                    rga_buffer_handle_t src_handle, dst_handle;
                    memset(&src_img, 0, sizeof(src_img));
                    memset(&dst_img, 0, sizeof(dst_img));

                    int dst_width, dst_height, dst_format;
                    dst_width = src_width;
                    dst_height = src_height;
                    // BGR
                    dst_format = RK_FORMAT_BGR_888;

                    int dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);
                    unsigned char* dst_buf = new unsigned char[dst_buf_size];
                    src_handle = importbuffer_virtualaddr(mpp_buffer_get_ptr(src_buf), src_buf_size);
                    dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);

                    src_img = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
                    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
                    imcvtcolor(src_img, dst_img, src_format, dst_format);
                    if (src_handle)
                        releasebuffer_handle(src_handle);
                    if (dst_handle)
                        releasebuffer_handle(dst_handle);

                    // 记录到image_list
                    image_list.push_back(
                        Matrix<Array<unsigned char, 3>>(dst_height, dst_width, dst_buf, false, true)
                    );
                    mpp_frame_deinit(&mpp_frame);
                }
            }

            if (frm_eos) {
                break;
            }

            if (get_frm)
                continue;

            break;            
        } while(1);

        if (pkt_done)
            break;

        /*
        * why sleep here:
        * mpi->decode_put_packet will failed when packet in internal queue is
        * full,waiting the package is consumed .Usually hardware decode one
        * frame which resolution is 1080p needs 2 ms,so here we sleep 3ms
        * * is enough.
        */
        usleep(3 * 1000);
    } while (1);

    EAGLEEYE_LOGD("Found frame size %d.", image_list.size());

    // for(int i=0; i<image_list.size(); ++i){
    //     Matrix<Array<unsigned char, 3>> image = image_list[i];
    //     std::string file_name = std::to_string(i)+"_image.bin";
	// 	std::ofstream binary_file(std::string("/storage/")+file_name,std::ios::binary);
    //     char* p = image.cpu<char>();
    //     int rows = image.rows();
    //     int cols = image.cols();
    //     binary_file.write(p,sizeof(char)*rows*cols*3);
    //     binary_file.close();
    // }

    // 重置CACHE
    m_cache_offset = 0;
    memcpy(m_cache_buf+m_cache_offset, package_data+copy_size, remain_size*sizeof(uint8_t));
    m_cache_offset = remain_size;
    return 0;
}
}

#else
namespace eagleeye{
RKH264Decoder::RKH264Decoder(){}
RKH264Decoder::~RKH264Decoder(){}
int RKH264Decoder::initial(){return 0;}
void RKH264Decoder::destroy(){}
int RKH264Decoder::decode(uint8_t* package_data, int package_size, std::vector<Matrix<Array<unsigned char, 3>>>& image_list){return 0;}
}

#endif