#include "eagleeye/processnode/ConsistenceNode.h"
#include <math.h>
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/common/EagleeyeStr.h"

namespace eagleeye
{
ConsistenceNode::ConsistenceNode(){
	// 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<float>, 0);

    // 设置输入端口
    // port 0: rgb image
	// port 1: solution data
	// port 2: optical flow
	this->setNumberOfInputSignals(3);

    this->m_is_first = true;
	this->m_lambda = 2.0f;

	EAGLEEYE_MONITOR_VAR(float, setLambda, getLambda, "lambda", "0.1", "10.0");
}

ConsistenceNode::~ConsistenceNode(){

}

void ConsistenceNode::executeNodeInfo(){
	// 输入端口0：接收RGB图像
	ImageSignal<Array<unsigned char, 3>>* input_image_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getInputPort(0);
	// 输入端口1：接收处理结果
	ImageSignal<float>* input_process_sig = (ImageSignal<float>*)this->getInputPort(1);
	// 输入端口2：接收光流
	ImageSignal<Array<float,2>>* optical_flow_sig = (ImageSignal<Array<float,2>>*)this->getInputPort(2);
	
	// 输出端口0：输出一致性优化结果
	ImageSignal<float>* output_sig = (ImageSignal<float>*)this->getOutputPort(0);

	// 从输入端口0， 获得输入图像
	Matrix<Array<unsigned char,3>> input_img = input_image_sig->getData();
	// 从输入端口1，获得处理结果
	Matrix<float> input_process = input_process_sig->getData();
	// 从输入端口2，获得光流
	Matrix<Array<float,2>> optical_flow = optical_flow_sig->getData();

	int H = input_img.rows();
	int W = input_img.cols();
	// cv::Mat curframe_uchar(H,W,CV_8U);
	// for(int i=0; i<H; ++i){
	// 	for(int j=0; j<W; ++j){
	// 		curframe_uchar.at<unsigned char>(i,j) = input_process.at(i,j) > 1.0 ? 255: input_process.at(i,j)*255;
	// 	}
	// }
	// static int ss = 0;
	// std::string ss_str = std::string("./ss_")+tos(ss)+".png";
	// cv::imwrite(ss_str.c_str(), curframe_uchar);
	// ss += 1;

	if(this->m_is_first){
		std::cout<<"is first frame"<<std::endl;
		this->m_pre_input = input_img;
		this->m_pre_solution = input_process.clone();
		
		// 直接使用当前帧处理结果
		output_sig->setData(input_process);
		this->m_is_first = false;
		return;
	}

	// std::cout<<"H "<<H<<" W "<<W<<std::endl;

	// cv::Mat preframe_uchar(H,W,CV_8U);
	// for(int i=0; i<H; ++i){
	// 	for(int j=0; j<W; ++j){
	// 		preframe_uchar.at<unsigned char>(i,j) = m_pre_solution.at(i,j) > 1.0 ? 255: m_pre_solution.at(i,j)*255;
	// 	}
	// }

	// static int aa = 0;
	// std::string aa_str = std::string("./AA_")+tos(aa)+".png";
	// cv::imwrite(aa_str.c_str(), preframe_uchar);

	// aa += 1;
	Matrix<float> cur_solution(H,W);
	this->solveFrame((unsigned char*)this->m_pre_input.dataptr(),
					 (unsigned char*)input_img.dataptr(),
					 (float*)input_process.dataptr(),
					 (float*)this->m_pre_solution.dataptr(),
					 (float*)cur_solution.dataptr(),
					 (float*)optical_flow.dataptr(),
					 W,H,this->m_lambda);

	this->m_pre_input = input_img;
	this->m_pre_solution = input_process.clone();
	output_sig->setData(cur_solution);

	// output_sig->setData(input_process);
}

// void drawOptFlowMap(const cv::Mat& flow, 
//                         cv::Mat& cflowmap, 
//                         int step,
//                         double cc, 
//                         const cv::Scalar& color){
//     for(int y = 0; y < cflowmap.rows; y += step)
//         for(int x = 0; x < cflowmap.cols; x += step)
//         {
//             const cv::Point2f& fxy = flow.at<cv::Point2f>(y, x);
//             line(cflowmap, cv::Point(x,y), cv::Point(cvRound(x+fxy.x), cvRound(y+fxy.y)),
//                  color);
//             circle(cflowmap, cv::Point(x,y), 1, color, -1);
//         }
// }


void ConsistenceNode::solveFrame(unsigned char* prevInput, 
                                 unsigned char* curInput, 
                                 float* curProcessed, 
                                 float* prevSolution, 
                                 float* curSolution, 
								 float* optical_flow,
                                 int W, int H, float lambda_t) {
	// cv::Mat backward_flow;    
	// cv::Mat pre_frame(H,W,CV_8UC3, prevInput);
	// cv::Mat gray_pre_frame;
	// cv::cvtColor(pre_frame,gray_pre_frame,CV_RGB2GRAY);
	// std::cout<<"step 2"<<std::endl;
	// cv::GaussianBlur(gray_pre_frame,gray_pre_frame,cv::Size(11,11),2.0,2.0);

	// cv::Mat cur_frame(H,W,CV_8UC3, curInput);
	// cv::Mat gray_cur_frame;
	// cv::cvtColor(cur_frame,gray_cur_frame, CV_RGB2GRAY);
	// cv::GaussianBlur(gray_cur_frame,gray_cur_frame,cv::Size(11,11),2.0,2.0);

	// std::cout<<"step 3"<<std::endl;

    // cv::calcOpticalFlowFarneback(gray_cur_frame, gray_pre_frame,backward_flow,0.5,4,21,4,5,1.1,cv::OPTFLOW_FARNEBACK_GAUSSIAN);
	// std::cout<<"step 4 opt flow "<<backward_flow.rows<<" "<<backward_flow.cols<<std::endl;

	// drawOptFlowMap(backward_flow, cur_frame, 20,0,cv::Scalar(255,0,0));
	// cv::imwrite("./flow.png", cur_frame);

	Matrix<Array<float,2>> backward_opt_flow(H, W);
	for(int i=0; i<H; ++i){
		Array<float,2>* opt_flow_ptr = backward_opt_flow.row(i);
		for(int j=0; j<W; ++j){
			// const cv::Point2f& fxy = backward_flow.at<cv::Point2f>(i, j);
			opt_flow_ptr[j][0] = j + optical_flow[(i*W+j)*2];
			opt_flow_ptr[j][1] = i + optical_flow[(i*W+j)*2+1];
		}
	}

	std::cout<<"step 5"<<std::endl;
	float* backward_opt_flow_data = (float*)backward_opt_flow.dataptr();
	//build RHS and weights
	std::vector<float> rhs(W*H, 0.);
	std::vector<float> diag(W*H, 0.);
	// float max_weight = 0.0f;

	// cv::Mat dd(H,W,CV_8U);
	// cv::Mat warp_preframe(H, W, CV_32F);

// #pragma omp parallel for
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			float gw = getWeight(curInput, prevInput, W, H, backward_opt_flow_data, i*W+j);
			float w = lambda_t * gw;
			// dd.at<unsigned char>(i,j) = gw*255 > 255 ? 255 : (unsigned char)(gw*255);
			// if(w > max_weight){
			// 	max_weight = w;
			// }
			int laplace = 4;
			if (i == 0 || i == H - 1) laplace--;
			if (j == 0 || j == W - 1) laplace--;

			int pix = i*W+j;
			// for (int k = 0; k < 3; k++) {
			// 	int p = pix*3+k;
			// 	const float upProcessed = i>0?curProcessed[p - 3*W]:0.;
			// 	const float downProcessed = i<(H-1)?curProcessed[p + 3*W]:0.;
			// 	const float leftProcessed = j>0?curProcessed[p - 3]:0.;
			// 	const float rightProcessed = j<(W-1)?curProcessed[p + 3]:0.;

			// 	const float backward_color = bilinear(&prevSolution[k], W, H, backward_opt_flow_data[pix * 2], backward_opt_flow_data[pix * 2 + 1], 3);

			// 	rhs[p] = laplace*curProcessed[p] - upProcessed - downProcessed - leftProcessed - rightProcessed  + w * backward_color;
			// }

			const float upProcessed = i>0?curProcessed[pix - W]:0.;
			const float downProcessed = i<(H-1)?curProcessed[pix + W]:0.;
			const float leftProcessed = j>0?curProcessed[pix - 1]:0.;
			const float rightProcessed = j<(W-1)?curProcessed[pix + 1]:0.;
			const float backward_color = bilinear(prevSolution, W, H, backward_opt_flow_data[pix * 2], backward_opt_flow_data[pix * 2 + 1], 1);

			// warp_preframe.at<float>(i,j) = backward_color;
			rhs[pix] = (laplace*curProcessed[pix] - upProcessed - downProcessed - leftProcessed - rightProcessed) * curProcessed[pix]*2.0  + w * backward_color * backward_color + curProcessed[pix] * 0.8;
			diag[pix] = laplace*curProcessed[pix]*2.0 + w*backward_color + 0.8;
			
			// rhs[pix] = curProcessed[pix] + w*backward_color;
			// diag[pix] = 1+w;
		}
	}
	// cv::Mat wrap_preframe_uchar(H,W,CV_8U);
	// cv::Mat preframe_uchar(H,W,CV_8U);
	// cv::Mat curframe_uchar(H,W,CV_8U);
	// for(int i=0; i<H; ++i){
	// 	for(int j=0; j<W; ++j){
	// 		wrap_preframe_uchar.at<unsigned char>(i,j) = warp_preframe.at<float>(i,j) > 1.0 ? 255 : warp_preframe.at<float>(i,j)*255;
	// 		preframe_uchar.at<unsigned char>(i,j) = prevSolution[i*W+j] > 1.0?255:prevSolution[i*W+j]*255;
	// 		curframe_uchar.at<unsigned char>(i,j) = curProcessed[i*W+j] > 1.0?255:curProcessed[i*W+j]*255;
	// 	}
	// }

	// static int aa = 0;
	// std::string aa_str = std::string("./wrap/wrap_pre_")+tos(aa)+".png";
	// cv::imwrite(aa_str.c_str(), wrap_preframe_uchar);

	// aa += 1;
	// cv::imwrite("./curprocessed.png", curframe_uchar);
	// cv::imwrite("./preprocessed.png", preframe_uchar);


	// cv::imwrite("./img.png", dd);
	// std::cout<<"step 6 max weight "<<max_weight<<std::endl;
	multiscaleSolver(curSolution, curProcessed, W, H, &diag[0], &rhs[0]);
}

void ConsistenceNode::multiscaleSolver(float* result_init, float* processed, int W, int H, float* diag, float* rhs) {
	int nlevels = 5;

    Matrix<float> res_down(H,W,result_init, false);
    Matrix<float> processed_down(H, W, processed, false);
    Matrix<float> diag_down(H, W, diag, false);
    Matrix<float> rhs_down(H,W,rhs,false);

	for (int i=nlevels; i>=0; i--) {
		int Wdst = W>>i;
		int Hdst = H>>i;
		// cimg_library::CImg<float> res_down(result_init, 3, W, H, 1, false); // shared images pose problem for resize
		// cimg_library::CImg<float> processed_down(processed, 3, W, H, 1, false);
		// cimg_library::CImg<float> diag_down(diag, 1, W, H, 1, false);
		// cimg_library::CImg<float> rhs_down(rhs, 3, W, H, 1, false);

        Matrix<float> scaled_res_down = resize(res_down, Hdst, Wdst,BILINEAR_INTERPOLATION);
        Matrix<float> scaled_processed_down = resize(processed_down, Hdst, Wdst, BILINEAR_INTERPOLATION);
        Matrix<float> scaled_diag_down = resize(diag_down, Hdst, Wdst, BILINEAR_INTERPOLATION);
        Matrix<float> scaled_rhs_down = resize(rhs_down, Hdst, Wdst, BILINEAR_INTERPOLATION);

		// res_down.resize(3, Wdst, Hdst, 1, 3);  // 3: linear ; 2:moving average ; 5: bicubic		
		// processed_down.resize(3, Wdst, Hdst, 1, 3);
		// diag_down.resize(1, Wdst, Hdst, 1, 3);
		// rhs_down.resize(3, Wdst, Hdst, 1, 3);

		gaussSeidel((float*)scaled_res_down.dataptr(), (float*)scaled_processed_down.dataptr(), scaled_diag_down.dataptr(), (float*)scaled_rhs_down.dataptr(), Wdst, Hdst, 50);

		// res_down.resize(3, W, H, 1, 3);
        
        res_down = resize(scaled_res_down, H,W, BILINEAR_INTERPOLATION);
	}

	memcpy(result_init, res_down.dataptr(), W*H*sizeof(float));
}

void ConsistenceNode::gaussSeidel(float* result_init, 
                                  const float* processed, 
                                  const float* diag, 
                                  const float* rhs, const int W, const int H, const int niter) {

	std::vector<float> tmp(W*H);
	float *pxA = result_init, *pxB = &tmp[0];

	for (int iter = 0; iter<niter; iter++) {

#pragma omp parallel for
		for (int i = 0; i < H; i++) {
			for (int j = 0; j < W; j++) {
				// for (int k = 0; k < 3; k++) {
				// 	int p = (i*W+j)*3+k;
				// 	const float up = i>0?pxA[p - 3*W]:0.;
				// 	const float down = i<(H-1)?pxA[p + 3*W]:0.;
				// 	const float left = j>0?pxA[p - 3]:0.;
				// 	const float right = j<(W-1)?pxA[p + 3]:0.;

				// 	int laplace = 4;
				// 	if (i == 0 || i == H - 1) laplace--;
				// 	if (j == 0 || j == W - 1) laplace--;
				// 	pxB[p] =  up + down + left + right + rhs[p];
				// }

				int pix = i*W+j;
				const float up = i>0?pxA[pix - W]:0.;
				const float down = i<(H-1)?pxA[pix + W]:0.;
				const float left = j>0?pxA[pix - 1]:0.;
				const float right = j<(W-1)?pxA[pix + 1]:0.;

				int laplace = 4;
				if (i == 0 || i == H - 1) laplace--;
				if (j == 0 || j == W - 1) laplace--;


				pxB[pix] =  (up + down + left + right)*processed[pix]*2.0 + rhs[pix];
				// pxB[pix] = rhs[pix];
			}
		}

#pragma omp parallel for
		for (int i = 0; i < W*H; i++) {
			float invdiag = 1./diag[i];
			// pxB[i*3] *= invdiag;
			// pxB[i*3+1] *= invdiag;
			// pxB[i*3+2] *= invdiag;

			pxB[i] *= invdiag;
		}

		std::swap(pxA, pxB);
	}

	if (niter % 2 == 0) {
		memcpy(result_init, &tmp[0], W*H*sizeof(result_init[0]));
	}

}

float ConsistenceNode::getWeight(const unsigned char* cur_frame, const unsigned char* prev_frame, int W, int H, const float* flow, int pix) {

	float otherVal0 = bilinear(prev_frame+0, W, H, flow[pix*2], flow[pix*2+1], 3);
	float otherVal1 = bilinear(prev_frame+1, W, H, flow[pix*2], flow[pix*2+1], 3);
	float otherVal2 = bilinear(prev_frame+2, W, H, flow[pix*2], flow[pix*2+1], 3);

	const float s = 0.05;
	float w = exp(-(sqr((otherVal0 - cur_frame[pix*3])/255.0) + sqr((otherVal1 - cur_frame[pix*3+1])/255.0) + sqr((otherVal2 - cur_frame[pix*3+2])/255.0))/(2.*s*s));
	// std::cout<<"w "<<w<<std::endl;

	if ((flow[pix*2] >= W - 2) || (flow[pix*2] <= 2)
		|| (flow[pix*2+1] >= H - 2) || (flow[pix*2+1] <= 2)) {  // going outside of the image area
		w = 0.0000;
	}

	return w;
}

float ConsistenceNode::bilinear(const float* table, int W, int H, float x, float y, int stride) {
	int i = std::max(0, std::min((int)y, H-2)); float fi = std::min(1.f, std::max(0.f, y-i));
	int j = std::max(0, std::min((int)x, W-2)); float fj = std::min(1.f, std::max(0.f, x-j));

	return (table[(i*W + j)*stride] * (1.f - fi) + table[((i + 1)*W + j)*stride] * fi)*(1. - fj) + (table[(i*W + j + 1)*stride] * (1.f - fi) + table[((i + 1)*W + j + 1)*stride] * fi)*fj;
}

float ConsistenceNode::bilinear(const unsigned char* table, int W, int H, float x, float y, int stride) {
	int i = std::max(0, std::min((int)y, H-2)); float fi = std::min(1.f, std::max(0.f, y-i));
	int j = std::max(0, std::min((int)x, W-2)); float fj = std::min(1.f, std::max(0.f, x-j));

	return (table[(i*W + j)*stride] * (1.f - fi) + table[((i + 1)*W + j)*stride] * fi)*(1. - fj) + (table[(i*W + j + 1)*stride] * (1.f - fi) + table[((i + 1)*W + j + 1)*stride] * fi)*fj;
}

void ConsistenceNode::setLambda(float lambda){
	this->m_lambda = lambda;
}
void ConsistenceNode::getLambda(float& lambda){
	lambda = this->m_lambda;
}

} // namespace eagleeye
