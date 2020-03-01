#include "eagleeye/processnode/OpticalFlowNode.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/basic/Variable.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/basic/MetaOperation.h"
#include <math.h>

namespace eagleeye{
OpticalFlowNode::OpticalFlowNode(bool isback){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<Array<float,2>>, OUTPUT_PORT_OPTICALFLOW);

    // whether compute back optical flow
    this->m_isback = isback;
    // 初始化标记，创建显存等操作
    this->m_has_been_ini = false;

    // 设置输入端口（拥有1个输入端口）
    // port 0: frame
	this->setNumberOfInputSignals(1);

    // inner parameters
    this->m_N = 192;
    int bs = sizeof(unsigned int)*8;
    this->m_N_space = (m_N+bs-1)/bs;
    this->m_patchRad = 21.0f;
    this->m_random_search_r = 31.0f;
    this->m_isfirst = 1;
    this->m_doubleProp = 1;
    this->m_jfMax = 4;
    this->m_refine_flag = false;
    this->m_iters = 3;
    
    this->m_median_filter_size = 1;
    this->m_gaussian_filter_size = 5;
    this->m_sEPE = 0.4f;
    this->m_scolor = 0.1f;
    this->m_sp = 6.0f;

    this->m_process_at_scale = 1.0f;

    EAGLEEYE_MONITOR_VAR(bool, setRefineProcess, getRefineProcess, "isrefine", "", "");
    EAGLEEYE_MONITOR_VAR(int, setIterCount, getIterCount, "iter", "2", "10");
    EAGLEEYE_MONITOR_VAR(int, setMedianFSize, getMedianFSize, "medianfs", "1", "11");
    EAGLEEYE_MONITOR_VAR(int, setGaussianSize, getGaussianSize, "gaussianfs", "1", "11");
    EAGLEEYE_MONITOR_VAR(float, setEPE, getEPE, "EPE", "0.1", "1.0");
    EAGLEEYE_MONITOR_VAR(float, setColorS, getColorS, "color_sigma", "0.1", "1.0");
    EAGLEEYE_MONITOR_VAR(float, setSpatialS, getSpatialS, "spatial_sigma", "2.0", "10.0");
    EAGLEEYE_MONITOR_VAR(float, setBRIEFSamplingRad, getBRIEFSamplingRad, "BRIEF_R", "5", "21");
    EAGLEEYE_MONITOR_VAR(float, setRandomSearchRad, getRandomSearchRad, "RANDOMSEARCH_R", "11", "51");
    EAGLEEYE_MONITOR_VAR(float, setProcessAtScale, getProcessAtScale, "process_at_scale", "0.5", "2.0");

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    // OpenCLEnv::setWritablePath("/sdcard/");
    EAGLEEYE_OPENCL_KERNEL_GROUP(opticalflow, algorithm, binaryFeatures,init,propagate,randomSearch,medianFilterS1D, crossTrilateralFilter,convert, resizeAndCombineToFlowXY, filter1D);
#endif
}   

OpticalFlowNode::~OpticalFlowNode(){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION    
    EAGLEEYE_OPENCL_RELEASE_KERNEL_GROUP(opticalflow);
#endif
} 

void OpticalFlowNode::executeNodeInfo(){
    // 0.step input/output signal
    // get input signal
	ImageSignal<Array<unsigned char, 3>>* frame_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->m_input_signals[INPUT_PORT_FRAME]);
    Matrix<Array<unsigned char, 3>> frame = frame_sig->getData();
    int frame_rows = frame.rows();
    int frame_cols = frame.cols();

    Matrix<Array<unsigned char, 3>> processed_frame = frame;
    if(this->m_process_at_scale != 1.0f){
        int resized_rows = frame.rows() * this->m_process_at_scale;
        int resized_cols = frame.cols() * this->m_process_at_scale;
        processed_frame = resize(frame, resized_rows, resized_cols, BILINEAR_INTERPOLATION);
    }
 
    this->m_sy_in = processed_frame.rows();
    this->m_sx_in = processed_frame.cols();
    this->m_sy = this->m_sy_in;
    this->m_sx = this->m_sx_in;

    if(!this->m_has_been_ini){
        EAGLEEYE_LOGD("init brief feature sampling position");
        Variable<float> random_var = Variable<float>::uniform(-1.0f, 1.0f);

        // BRIEF patch sampling
        float *samp_patch = new float[4*this->m_N];
        for (unsigned int s=0; s<4*this->m_N; s++){
            float val = random_var.var();
            samp_patch[s] = this->m_patchRad * std::max(-1.0f, std::min(1.0f, val));
        }

        EAGLEEYE_LOGD("init random seed");
        int* seed_state = new int[this->m_sx*this->m_sy];
        int sxy = this->m_sx*this->m_sy;
        for(unsigned int i=0; i<sxy; ++i){
            seed_state[i] = rand();
        }

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
        EAGLEEYE_OPENCL_CREATE_READ_BUFFER(opticalflow, samp_patch, sizeof(float)*4*this->m_N); 
        EAGLEEYE_OPENCL_COPY_TO_DEVICE(opticalflow, samp_patch, samp_patch);
        
        EAGLEEYE_OPENCL_CREATE_READ_BUFFER(opticalflow, preI, sizeof(float)*this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_BUFFER(opticalflow, curI, sizeof(float)*this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, dev_feat1, this->m_sx*this->m_sy*this->m_N_space*sizeof(unsigned int));
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, dev_feat2, this->m_sx*this->m_sy*this->m_N_space*sizeof(unsigned int));
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NX, sizeof(short)* this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NY, sizeof(short)* this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NX_MEDIAN, sizeof(short)*this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NY_MEDIAN, sizeof(short)*this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, N_TEMP, sizeof(short)*this->m_sx*this->m_sy);

        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NX_F, sizeof(float)* this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NY_F, sizeof(float)* this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NX_MEDIAN_F, sizeof(float)*this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NY_MEDIAN_F, sizeof(float)*this->m_sx*this->m_sy);

        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NX_REFINE, sizeof(float)*this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NY_REFINE, sizeof(float)*this->m_sx*this->m_sy);
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, NXY, sizeof(float)*2*frame_rows*frame_cols);

        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, dev_dist, this->m_sx*this->m_sy*sizeof(float));
        EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(opticalflow, seed_state, this->m_sx*this->m_sy*sizeof(int));
        EAGLEEYE_OPENCL_COPY_TO_DEVICE(opticalflow, seed_state, seed_state);
 #endif

        delete[] samp_patch;
        delete[] seed_state;
        this->m_has_been_ini = true;
    }

    EAGLEEYE_LOGD("convert rgb to gray image");
    Matrix<float> image_gray = processed_frame.transform(RGB2GRAY<float>());
    
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    EAGLEEYE_OPENCL_COPY_TO_DEVICE(opticalflow, curI, image_gray.dataptr());
    size_t work_dims = 2;
    size_t global_size[2];
    global_size[0] = this->m_sx;
    global_size[1] = this->m_sy;
    size_t local_size[] = {32,32};
    // 完成验证 - brief feature
    EAGLEEYE_TIME_START(brief);
    // set args for kernel binary features 
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, binaryFeatures, 0, curI);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, binaryFeatures, 1, dev_feat2);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, binaryFeatures, 2, this->m_sx);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, binaryFeatures, 3, this->m_sy);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, binaryFeatures, 4, this->m_N);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, binaryFeatures, 5, this->m_N_space);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, binaryFeatures, 6, samp_patch);
    EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, binaryFeatures, work_dims, global_size, local_size);
    EAGLEEYE_TIME_END(brief);

    if(this->m_isfirst == 0 && this->m_isback){
        // preI hold current frame
        // dev_feat1 hold current BRIEF

        // curI hold pre frame
        // dev_feat2 hold pre BRIEF
        EAGLEEYE_OPENCL_SWAP(opticalflow, dev_feat1, dev_feat2);
        EAGLEEYE_OPENCL_SWAP(opticalflow, preI, curI);
    }

    // 完成验证 - init
    // set args for kernel init
    EAGLEEYE_TIME_START(init);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, init, 0, preI);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, init, 1, curI);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, init, 2, dev_feat1);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, init, 3, dev_feat2);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, init, 4, dev_dist);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, init, 5, NX);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, init, 6, NY);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, init, 7, this->m_sx);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, init, 8, this->m_sy);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, init, 9, this->m_isfirst);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, init, 10, this->m_N);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, init, 11, this->m_N_space);
    EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, init, work_dims, global_size, local_size);
    EAGLEEYE_TIME_END(init);
    if(this->m_isfirst == 1){
        this->m_isfirst = 0;

        // return early
        EAGLEEYE_OPENCL_SWAP(opticalflow, dev_feat1, dev_feat2);
        EAGLEEYE_OPENCL_SWAP(opticalflow, preI, curI);
        return;
    }
    
    int pd = -1;
    // float dist = sqrt(ceil(this->m_sx*0.5f/3.0f));
    // float dist = 20.0f;
    // Iterate propagation and random search
    for (unsigned int i=0; i<this->m_iters; i++){
        // 完成验证 propagate
        EAGLEEYE_TIME_START(propagate);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, propagate, 0, preI);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, propagate, 1, curI);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, propagate, 2, dev_feat1);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, propagate, 3, dev_feat2);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, propagate, 4, dev_dist);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, propagate, 5, NX);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, propagate, 6, NY);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, propagate, 7, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, propagate, 8, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, propagate, 9, pd);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, propagate, 10, this->m_jfMax);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, propagate, 11, this->m_doubleProp);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, propagate, 12, this->m_N);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, propagate, 13, this->m_N_space);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, propagate, work_dims, global_size, local_size);
        EAGLEEYE_TIME_END(propagate);

        // 完成验证 randomsearch
        EAGLEEYE_TIME_START(randomsearch);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, randomSearch, 0, preI);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, randomSearch, 1, curI);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, randomSearch, 2, dev_feat1);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, randomSearch, 3, dev_feat2);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, randomSearch, 4, dev_dist);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, randomSearch, 5, NX);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, randomSearch, 6, NY);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, randomSearch, 7, seed_state);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, randomSearch, 8, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, randomSearch, 9, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, randomSearch, 10, this->m_random_search_r);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, randomSearch, 11, this->m_N);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, randomSearch, 12, this->m_N_space);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, randomSearch, work_dims, global_size, local_size);
        EAGLEEYE_TIME_END(randomsearch);

        pd = -pd;
    }

    if(this->m_median_filter_size > 1){
        EAGLEEYE_TIME_START(medianfilter);
        // flow_x median filter
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, medianFilterS1D, 0, NX);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, medianFilterS1D, 1, N_TEMP);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 2, 0);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 3, this->m_median_filter_size);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 4, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 5, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, medianFilterS1D, work_dims, global_size, local_size);

        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, medianFilterS1D, 0, N_TEMP);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, medianFilterS1D, 1, NX_MEDIAN);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 2, 1);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 3, this->m_median_filter_size);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 4, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 5, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, medianFilterS1D, work_dims, global_size, local_size);

        // flow_y median filter
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, medianFilterS1D, 0, NY);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, medianFilterS1D, 1, N_TEMP);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 2, 0);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 3, this->m_median_filter_size);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 4, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 5, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, medianFilterS1D, work_dims, global_size, local_size);

        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, medianFilterS1D, 0, N_TEMP);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, medianFilterS1D, 1, NY_MEDIAN);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 2, 1);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 3, this->m_median_filter_size);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 4, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, medianFilterS1D, 5, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, medianFilterS1D, work_dims, global_size, local_size);
        EAGLEEYE_TIME_END(medianfilter);

        // convert short to float flow
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 0, NX_MEDIAN);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 1, NX_MEDIAN_F);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 2, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 3, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, convert, work_dims, global_size, local_size);

        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 0, NY_MEDIAN);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 1, NY_MEDIAN_F);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 2, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 3, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, convert, work_dims, global_size, local_size);
    }
    else{
        // convert short to float flow
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 0, NX);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 1, NX_MEDIAN_F);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 2, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 3, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, convert, work_dims, global_size, local_size);

        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 0, NY);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 1, NY_MEDIAN_F);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 2, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 3, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, convert, work_dims, global_size, local_size);
    }

    if(this->m_refine_flag){
        // convert short to float flow
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 0, NX);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 1, NX_F);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 2, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 3, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, convert, work_dims, global_size, local_size);

        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 0, NY);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, convert, 1, NY_F);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 2, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, convert, 3, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, convert, work_dims, global_size, local_size);

        // cross trilateral filter
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, crossTrilateralFilter, 0, NX_F);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, crossTrilateralFilter, 1, NY_F);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, crossTrilateralFilter, 2, NX_REFINE);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, crossTrilateralFilter, 3, NY_REFINE);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, crossTrilateralFilter, 4, NX_MEDIAN_F);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, crossTrilateralFilter, 5, NY_MEDIAN_F);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, crossTrilateralFilter, 6, preI);
        float sigma_epe = this->m_sEPE*this->m_sEPE;
        float sigma_color = this->m_scolor*this->m_scolor;
        float sigmaz_spatial = this->m_sp*this->m_sp;
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, crossTrilateralFilter, 7, sigma_epe);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, crossTrilateralFilter, 8, sigma_color);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, crossTrilateralFilter, 9, sigmaz_spatial);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, crossTrilateralFilter, 10, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, crossTrilateralFilter, 11, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, crossTrilateralFilter, work_dims, global_size, local_size);

        // resize to frame size (resizeAndCombineToFlowXY)
        global_size[0] = frame_cols;
        global_size[1] = frame_rows;        
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, resizeAndCombineToFlowXY, 0, NX_REFINE);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, resizeAndCombineToFlowXY, 1, NY_REFINE);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, resizeAndCombineToFlowXY, 2, NXY);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, resizeAndCombineToFlowXY, 3, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, resizeAndCombineToFlowXY, 4, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, resizeAndCombineToFlowXY, 5, frame_cols);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, resizeAndCombineToFlowXY, 6, frame_rows);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, resizeAndCombineToFlowXY,  work_dims, global_size, local_size);
    }
    else{
        // resize to frame size (resizeAndCombineToFlowXY)
        global_size[0] = frame_cols;
        global_size[1] = frame_rows;   

        EAGLEEYE_TIME_START(resizeandcombine);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, resizeAndCombineToFlowXY, 0, NX_MEDIAN_F);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, resizeAndCombineToFlowXY, 1, NY_MEDIAN_F);
        EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(opticalflow, resizeAndCombineToFlowXY, 2, NXY);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, resizeAndCombineToFlowXY, 3, this->m_sx);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, resizeAndCombineToFlowXY, 4, this->m_sy);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, resizeAndCombineToFlowXY, 5, frame_cols);
        EAGLEEYE_OPENCL_KERNEL_SET_ARG(opticalflow, resizeAndCombineToFlowXY, 6, frame_rows);
        EAGLEEYE_OPENCL_KERNEL_RUN(opticalflow, resizeAndCombineToFlowXY,  work_dims, global_size, local_size);
        EAGLEEYE_TIME_END(resizeandcombine);
    }
    
    Matrix<Array<float,2>> frame_flow(frame_rows, frame_cols);
    EAGLEEYE_OPENCL_COPY_TO_HOST(opticalflow, NXY, (float*)frame_flow.dataptr());
    ImageSignal<Array<float,2>>* flow_sig = (ImageSignal<Array<float,2>>*)(this->getOutputPort(0));
    flow_sig->setData(frame_flow);

    if(!this->m_isback){
        // swap pre and cur
        EAGLEEYE_OPENCL_SWAP(opticalflow, dev_feat1, dev_feat2);
        EAGLEEYE_OPENCL_SWAP(opticalflow, preI, curI);
    }

#endif
}

void OpticalFlowNode::reset(){
    Superclass::reset();

    m_isfirst = 1;
}

void OpticalFlowNode::setRefineProcess(bool refine_flag){
    m_refine_flag = refine_flag;
}
void OpticalFlowNode::getRefineProcess(bool& refine_flag){
    refine_flag = m_refine_flag;
}

void OpticalFlowNode::setIterCount(int iters){
    this->m_iters = iters;
}
void OpticalFlowNode::getIterCount(int& iters){
    iters = this->m_iters;
}

void OpticalFlowNode::setMedianFSize(int size){
    this->m_median_filter_size = size;
}
void OpticalFlowNode::getMedianFSize(int& size){
    size = this->m_median_filter_size;
}

void OpticalFlowNode::setGaussianSize(int size){
    this->m_gaussian_filter_size = size;
}
void OpticalFlowNode::getGaussianSize(int& size){
    size = this->m_gaussian_filter_size;
}

void OpticalFlowNode::setEPE(float value){
    this->m_sEPE = value;
}
void OpticalFlowNode::getEPE(float& value){
    value = this->m_sEPE;
}
void OpticalFlowNode::setColorS(float value){
    this->m_scolor = value;
}
void OpticalFlowNode::getColorS(float& value){
    value = this->m_scolor;
}
void OpticalFlowNode::setSpatialS(float value){
    this->m_sp = value;
}
void OpticalFlowNode::getSpatialS(float& value){
    value = this->m_sp;
}

void OpticalFlowNode::setBRIEFSamplingRad(float v){
    this->m_patchRad = v;
}
void OpticalFlowNode::getBRIEFSamplingRad(float& v){
    v = this->m_patchRad;
}

void OpticalFlowNode::setRandomSearchRad(float v){
    this->m_random_search_r = v;
}
void OpticalFlowNode::getRandomSearchRad(float& v){
    v = this->m_random_search_r;
}

void OpticalFlowNode::setProcessAtScale(float value){
    this->m_process_at_scale = value;
}

void OpticalFlowNode::getProcessAtScale(float& value){
    value = this->m_process_at_scale;
}
}