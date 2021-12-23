#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/engine/math/arm/interpolate.h"
#include <cmath>
#include <math.h>       /* atan2 */
namespace eagleeye{
Matrix<Array<unsigned char, 3>> resize(const Matrix<Array<unsigned char, 3>> img,
										int after_r,int after_c,
										InterpMethod interp_method){
    Matrix<Array<unsigned char, 3>> dst(after_r, after_c);
    Matrix<Array<unsigned char, 3>> img_cp = img;
	unsigned int offset_r, offset_c;
	img.offset(offset_r, offset_c);

    unsigned char* dst_ptr = (unsigned char*)dst.dataptr();

    math::arm::bilinear_rgb_8u_3d_interp((unsigned char*)img.dataptr(), dst_ptr,img.cols(), img.rows(),offset_c,offset_r,img.stride(),after_c, after_r);

    return dst;
}

Matrix<unsigned char> resize(const Matrix<unsigned char>  img,
										int after_r,int after_c,
										InterpMethod interp_method){
    Matrix<unsigned char> dst(after_r, after_c);
	unsigned int offset_r, offset_c;
	img.offset(offset_r, offset_c);

    const unsigned char* img_ptr = img.dataptr();
    unsigned char* dst_ptr = dst.dataptr();

    math::arm::bilinear_gray_8u_1d_interp(img_ptr, dst_ptr,img.cols(), img.rows(),offset_c,offset_r,img.stride(),after_c, after_r);  
    return dst;                                         
}

Matrix<float> resize(const Matrix<float>  img,
					 int after_r,int after_c,
					 InterpMethod interp_method){
    Matrix<float> dst(after_r, after_c);
	unsigned int offset_r, offset_c;
	img.offset(offset_r, offset_c);

    const float* img_ptr = img.dataptr();
    float* dst_ptr = dst.dataptr();
	math::arm::bilinear_32f_c1_interp(img_ptr, dst_ptr, img.cols(), img.rows(),offset_c, offset_r, img.stride(), after_c, after_r);
    return dst;                                         
}


Matrix<float> hanning(int size){
	Matrix<float> hanning_mat(1, size);
	for (int i = 0; i < size; i++){
        hanning_mat.at(0, i) = 0.5f * (1 - std::cos(2 * 3.14159265358979323846 * i / (size - 1)));
	}

	return hanning_mat;
}

Matrix<float> outer(Matrix<float> a, Matrix<float> b){
	assert(a.rows() == 1 || a.cols() == 1);
	assert(b.rows() == 1 || b.cols() == 1);

	int a_size = a.size();
	int b_size = b.size();
	Matrix<float> m(a_size, b_size);
	for(int a_i=0; a_i<a_size; ++a_i){
		for(int b_i=0; b_i<b_size; ++b_i){
			m.at(a_i,b_i) = a.at(a_i) * b.at(b_i);
		}
	}

	return m;
}

float _bilinear(const unsigned char* table, int W, int H, float x, float y, int stride) {
	int i = std::max(0, std::min((int)y, H-2)); float fi = std::min(1.f, std::max(0.f, y-i));
	int j = std::max(0, std::min((int)x, W-2)); float fj = std::min(1.f, std::max(0.f, x-j));

	return (table[(i*W + j)*stride] * (1.f - fi) + table[((i + 1)*W + j)*stride] * fi)*(1. - fj) + (table[(i*W + j + 1)*stride] * (1.f - fi) + table[((i + 1)*W + j + 1)*stride] * fi)*fj;
}

Matrix<Array<unsigned char,3>> warp(Matrix<Array<unsigned char,3>> img, 
									Matrix<Array<float,2>> op_flow){
	int rows = img.rows();
	int cols = img.cols();

	unsigned char* img_ptr = (unsigned char*)img.dataptr();
	Matrix<Array<unsigned char,3>> warp_img(rows, cols);
	for(int i=0; i<rows; ++i){
		Array<unsigned char,3>* warp_img_ptr = warp_img.row(i);
		Array<float,2>* flow_ptr = op_flow.row(i);
		for(int j=0; j<cols; ++j){
			warp_img_ptr[j][0] = _bilinear(img_ptr, cols, rows, j+flow_ptr[j][0],i+flow_ptr[j][1], 3);
			warp_img_ptr[j][1] = _bilinear(img_ptr+1, cols, rows, j+flow_ptr[j][0],i+flow_ptr[j][1], 3);
			warp_img_ptr[j][2] = _bilinear(img_ptr+2, cols, rows, j+flow_ptr[j][0],i+flow_ptr[j][1], 3);
		}
	}

	return warp_img;
}

void resize(const Matrix<Array<unsigned char, 3>> input,
				Matrix<Array<unsigned char, 3>>& output,
				InterpMethod interp_method){
    Matrix<Array<unsigned char, 3>> img_cp = input;
	unsigned int offset_r, offset_c;
	input.offset(offset_r, offset_c);
	int after_c = output.cols();
	int after_r = output.rows();

    unsigned char* dst_ptr = (unsigned char*)output.dataptr();
    math::arm::bilinear_rgb_8u_3d_interp((unsigned char*)input.dataptr(), 
								dst_ptr,
									input.cols(), 
									input.rows(),
									offset_c,
									offset_r,
									input.stride(),
									after_c, 
									after_r);
}

void resize(const Matrix<Array<unsigned char, 3>> input,
				unsigned char* output,
				int output_r,
				int output_c,
				InterpMethod interp_method){
    Matrix<Array<unsigned char, 3>> img_cp = input;
	unsigned int offset_r, offset_c;
	input.offset(offset_r, offset_c);

    math::arm::bilinear_rgb_8u_3d_interp((unsigned char*)input.dataptr(), 
									output,
									input.cols(), 
									input.rows(),
									offset_c,
									offset_r,
									input.stride(),
									output_c, 
									output_r);
}

void resize(const Matrix<float> input,
				Matrix<float>& output,
				InterpMethod interp_method){
	int after_c = output.cols();
	int after_r = output.rows();
	unsigned int offset_r, offset_c;
	input.offset(offset_r, offset_c);

    const float* img_ptr = input.dataptr();
    float* dst_ptr = output.dataptr();
	math::arm::bilinear_32f_c1_interp(img_ptr, 
							dst_ptr, 
							input.cols(), 
							input.rows(),
							offset_c, 
							offset_r, 
							input.stride(), 
							after_c, 
							after_r);
}

void resize(const Matrix<float> input,
				float* output,
				int output_r,
				int output_c,
				InterpMethod interp_method){
	unsigned int offset_r, offset_c;
	input.offset(offset_r, offset_c);

    const float* img_ptr = input.dataptr();
	math::arm::bilinear_32f_c1_interp(img_ptr, 
							output, 
							input.cols(), 
							input.rows(),
							offset_c, 
							offset_r, 
							input.stride(), 
							output_c, 
							output_r);
}

void _rotation90right(unsigned char* src, int x_offset, int y_offset, int stride, unsigned char* dst, int srcW, int srcH, int channel){
    int i, j, offset, start;
    if (channel == 1) {
        start = srcH - 1;
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;
            offset = start - i;
            for (j = 0; j < srcW; j++) {
                //dst[j * srcH + (srcH - 1 - i)] = *src++;
                dst[offset] = *src_ptr++;
                offset += srcH;
            }
        }
    } else if (channel == 3) {
        start = 3 * (srcH - 1);
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;
            offset = start - (i << 1) - i;
            for (j = 0; j < srcW; j++) {
                //dst[(j * srcH + (srcH - 1 - i)) * 3 + 0] = *src++;
                //dst[(j * srcH + (srcH - 1 - i)) * 3 + 1] = *src++;
                //dst[(j * srcH + (srcH - 1 - i)) * 3 + 2] = *src++;
                dst[offset + 0] = *src_ptr++;
                dst[offset + 1] = *src_ptr++;
                dst[offset + 2] = *src_ptr++;
                offset += (srcH << 1) + srcH; //3 * srcH;
            }
        }
    } else if(channel == 4){
        start = 4 * (srcH - 1);
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;
            offset = start - (i << 2);
            for (j = 0; j < srcW; j++) {
                dst[offset + 0] = *src_ptr++;
                dst[offset + 1] = *src_ptr++;
                dst[offset + 2] = *src_ptr++;
                dst[offset + 3] = *src_ptr++;
                offset += (srcH << 2); //4 * srcH;
            }
        }
    }
}

void _rotation180right(unsigned char* src, int x_offset, int y_offset, int stride, unsigned char* dst, int srcW, int srcH, int channel) {
    int i, j, offset;
    if (channel == 1) {
        offset = srcH * srcW - 1;
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;
            for (j = 0; j < srcW; j++) {
                //dst[(srcH - 1 - i) * srcW + (srcW - 1 - j)] = *src++;
                dst[offset] = *src_ptr++;
                offset -= 1;
            }
        }
    } else if (channel == 3) {
        offset = 3 * (srcH * srcW - 1);
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;			
            for (j = 0; j < srcW; j++) {
                //dst[((srcH - 1 - i) * srcW + (srcW - 1 - j)) * 3 + 0] = *src++;
                //dst[((srcH - 1 - i) * srcW + (srcW - 1 - j)) * 3 + 1] = *src++;
                //dst[((srcH - 1 - i) * srcW + (srcW - 1 - j)) * 3 + 2] = *src++;
                dst[offset + 0] = *src_ptr++;
                dst[offset + 1] = *src_ptr++;
                dst[offset + 2] = *src_ptr++;
                offset -= 3;
            }
        }
    } else if(channel == 4){
        offset = 4 * (srcH * srcW - 1);
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;			
            for (j = 0; j < srcW; j++) {
                //dst[((srcH - 1 - i) * srcW + (srcW - 1 - j)) * 3 + 0] = *src++;
                //dst[((srcH - 1 - i) * srcW + (srcW - 1 - j)) * 3 + 1] = *src++;
                //dst[((srcH - 1 - i) * srcW + (srcW - 1 - j)) * 3 + 2] = *src++;
                dst[offset + 0] = *src_ptr++;
                dst[offset + 1] = *src_ptr++;
                dst[offset + 2] = *src_ptr++;
                dst[offset + 3] = *src_ptr++;
                offset -= 4;
            }
        }
    }

}

void _rotation270right(unsigned char* src, int x_offset, int y_offset, int stride,unsigned char* dst, int srcW, int srcH, int channel) {
    int i, j, offset, start;
    if (channel == 1) {
        start = (srcW - 1) * srcH;
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;			
            offset = start + i;
            for (j = 0; j < srcW; j++) {
                //dst[(srcW - 1 - j) * srcH + i] = *src++;
                dst[offset] = *src_ptr++;
                offset -= srcH;
            }
        }
    } else if (channel == 3) {
        start = 3 * (srcW - 1) * srcH;
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;			
            offset = start + (i << 1) + i; //3 * i
            for (j = 0; j < srcW; j++) {
                //dst[((srcW - 1 - j) * srcH + i) * 3 + 0] = *src++;
                //dst[((srcW - 1 - j) * srcH + i) * 3 + 1] = *src++;
                //dst[((srcW - 1 - j) * srcH + i) * 3 + 2] = *src++;
                dst[offset + 0] = *src_ptr++;
                dst[offset + 1] = *src_ptr++;
                dst[offset + 2] = *src_ptr++;
                offset -= ((srcH << 1) + srcH); //3 * srcH;
            }
        }
    } else if(channel == 4){
        start = 4 * (srcW - 1) * srcH;
        for (i = 0; i < srcH; i++) {
			unsigned char* src_ptr = src + (y_offset+i) * stride*channel + x_offset*channel;			
            offset = start + (i << 1) + i; //3 * i
            for (j = 0; j < srcW; j++) {
                //dst[((srcW - 1 - j) * srcH + i) * 3 + 0] = *src++;
                //dst[((srcW - 1 - j) * srcH + i) * 3 + 1] = *src++;
                //dst[((srcW - 1 - j) * srcH + i) * 3 + 2] = *src++;
                dst[offset + 0] = *src_ptr++;
                dst[offset + 1] = *src_ptr++;
                dst[offset + 2] = *src_ptr++;
                dst[offset + 3] = *src_ptr++;
                offset -= ((srcH << 1) + srcH); //3 * srcH;
            }
        }
    }

}
Matrix<Array<unsigned char,3>> rotation90right(Matrix<Array<unsigned char,3>> img){
	int rows = img.rows(); int cols = img.cols();
	unsigned int y_offset, x_offset;
	img.offset(y_offset, x_offset);
	int stride = img.stride();
	Matrix<Array<unsigned char,3>> rotated_img(cols, rows);
	_rotation90right((unsigned char*)img.dataptr(), x_offset, y_offset, stride, (unsigned char*)rotated_img.dataptr(), cols, rows, 3);
	return rotated_img;
}

Matrix<Array<unsigned char,3>> rotation180right(Matrix<Array<unsigned char,3>> img){
	int rows = img.rows(); int cols = img.cols();
	unsigned int y_offset, x_offset;
	img.offset(y_offset, x_offset);
	int stride = img.stride();
	Matrix<Array<unsigned char,3>> rotated_img(rows, cols);
	_rotation180right((unsigned char*)img.dataptr(), x_offset, y_offset, stride, (unsigned char*)rotated_img.dataptr(), cols, rows, 3);
	return rotated_img;
}

Matrix<Array<unsigned char,3>> rotation270right(Matrix<Array<unsigned char,3>> img){
	int rows = img.rows(); int cols = img.cols();
	unsigned int y_offset, x_offset;
	img.offset(y_offset, x_offset);
	int stride = img.stride();
	Matrix<Array<unsigned char,3>> rotated_img(cols, rows);
	_rotation270right((unsigned char*)img.dataptr(), x_offset, y_offset, stride, (unsigned char*)rotated_img.dataptr(), cols, rows, 3);
	return rotated_img;
}

Matrix<float> matan2(Matrix<float> y, Matrix<float> x){
	int rows = y.rows();
	int cols = y.cols();

	Matrix<float> theta(rows, cols);
	for(int r=0; r<rows; ++r){
		float* y_ptr = y.row(r);
		float* x_ptr = x.row(r);
		float* theta_ptr = theta.row(r);

		for(int c=0; c<cols; ++c){
			theta_ptr[c] = atan2(y_ptr[c], x_ptr[c]);
		}
	}

	return theta;
}
}