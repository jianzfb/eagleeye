#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <cmath>
#ifdef EAGLEEYE_NEON_OPTIMIZATION
#include <arm_neon.h>
#endif
namespace eagleeye{
#define QRBAR_SHIFTBITS    8
#define QRBAR_ROUND0(x)  (x>>QRBAR_SHIFTBITS)
#define QRBAR_ROUND1(x)  (ROUND0(x))+1
#ifndef QRBAR_CLIP
#define QRBAR_CLIP(x) ( x<0 ? 0 : (x>255 ? 255 : x) )
#endif

void _BilinearResize_8u_1D_1R(const unsigned char * pSrcImg, unsigned char * pDesImg, int srcWidth, int srcHeight, int srcOffsetX, int srcOffsetY, int srcStride, int desWidth, int desHeight)
{
	int i, j;
	int nRateW, nRateH;
	int y;
	unsigned short *coord_x, *coord_y;
	unsigned char *bi_coef_x, *bi_coef_y;
	unsigned char *sub_bi_coef_x, *sub_bi_coef_y;
	unsigned char *pImg_left_top, *pImg_left_down;
	unsigned char *pImg_right_top, *pImg_right_down;
	int float_value;
	const short std_1_value = (1 << QRBAR_SHIFTBITS);
	const short std_and_value = ((1 << QRBAR_SHIFTBITS) - 1);
	int res_x0, res_x1;
	int res_xy;
	const unsigned char *pSrc, *pSrc1;

	pImg_left_top = (unsigned char *)malloc(sizeof(char)*desWidth);
	pImg_left_down = (unsigned char *)malloc(sizeof(char)*desWidth);
	pImg_right_top = (unsigned char *)malloc(sizeof(char)*desWidth);
	pImg_right_down = (unsigned char *)malloc(sizeof(char)*desWidth);

	coord_x = (unsigned short *)malloc(sizeof(short)*desWidth);
	coord_y = (unsigned short *)malloc(sizeof(short)*desHeight);
	bi_coef_x = (unsigned char *)malloc(sizeof(char)*desWidth);
	bi_coef_y = (unsigned char *)malloc(sizeof(char)*desHeight);
	sub_bi_coef_x = (unsigned char *)malloc(sizeof(char)*desWidth);
	sub_bi_coef_y = (unsigned char *)malloc(sizeof(char)*desHeight);


	if (coord_x == NULL || coord_y == NULL || bi_coef_x == NULL || bi_coef_y == NULL || sub_bi_coef_x == NULL || sub_bi_coef_y == NULL ||
		pImg_left_top == NULL || pImg_left_down == NULL || pImg_right_top == NULL || pImg_right_down == NULL)
		return;

    nRateW = (srcWidth << QRBAR_SHIFTBITS) / (desWidth);
    nRateH = (srcHeight << QRBAR_SHIFTBITS) / (desHeight);

	for (i = 0; i < desHeight; ++i)
	{
		float_value = i * nRateH;
		bi_coef_y[i] = float_value & std_and_value;
		if (!bi_coef_y[i])
			bi_coef_y[i] = 1;
		sub_bi_coef_y[i] = std_1_value - bi_coef_y[i];
		coord_y[i] = QRBAR_ROUND0(float_value);
		if (coord_y[i] > srcHeight - 2)
			coord_y[i] = srcHeight - 2;
	}

	for (i = 0; i < desWidth; ++i)
	{
		float_value = i * nRateW;
		bi_coef_x[i] = float_value & std_and_value;
		if (!bi_coef_x[i])
			bi_coef_x[i] = 1;
		sub_bi_coef_x[i] = std_1_value - bi_coef_x[i];
		coord_x[i] = QRBAR_ROUND0(float_value);
		if (coord_x[i] > srcWidth - 2)
			coord_x[i] = srcWidth - 2;
	}

	for (i = 0; i < desHeight; i++)
	{
		// pSrc = pSrcImg + coord_y[i] * srcWidth;
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride + srcOffsetX;
		y = coord_y[i];
		for (j = 0; j < desWidth; j++)
		{
			pSrc1 = pSrc;
			pSrc1 += coord_x[j];
			pImg_left_top[j] = *pSrc1;
			pImg_right_top[j] = *(pSrc1 + 1);
			// pSrc1 += srcWidth;
			pSrc1 += srcStride;
			pImg_left_down[j] = *pSrc1;
			pImg_right_down[j] = *(pSrc1 + 1);
		}

#ifdef EAGLEEYE_NEON_OPTIMIZATION
		unsigned char *img1, *img2, *img3, *img4;
		unsigned char *outimg;
		unsigned char *coef1, *coef2, *coef3, *coef4;
		unsigned char *coef_x1, *coef_x2;
		uint8x8_t value_y1, value_y2;

		value_y1 = vdup_n_u8(sub_bi_coef_y[i]);
		value_y2 = vdup_n_u8(bi_coef_y[i]);
		img1 = pImg_left_top;
		img2 = pImg_right_top;
		img3 = pImg_left_down;
		img4 = pImg_right_down;
		outimg = pDesImg + i*desWidth;
		coef_x1 = sub_bi_coef_x;
		coef_x2 = bi_coef_x;
		for (j = 0; j < desWidth; j += 8)
		{
			uint8x8_t v1, v2, v3, v4;
			uint8x8_t value_x1, value_x2;
			uint8x8_t bires_x0, bires_x1, bires_xy;
			uint16x8_t multi_value;

			v1 = vld1_u8(img1);
			v2 = vld1_u8(img2);
			v3 = vld1_u8(img3);
			v4 = vld1_u8(img4);

			value_x1 = vld1_u8(coef_x1);
			value_x2 = vld1_u8(coef_x2);
			multi_value = vmull_u8(v1, value_x1);
			multi_value = vmlal_u8(multi_value, v2, value_x2);
			bires_x0 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(v3, value_x1);
			multi_value = vmlal_u8(multi_value, v4, value_x2);
			bires_x1 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(bires_x0, value_y1);
			multi_value = vmlal_u8(multi_value, bires_x1, value_y2);
			bires_xy = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			vst1_u8(outimg, bires_xy);

			outimg += 8;
			img1 += 8;
			img2 += 8;
			img3 += 8;
			img4 += 8;
			coef_x1 += 8;
			coef_x2 += 8;
		}
		for (; j < desWidth; j++)
		{
			res_x0 = (((*img1)*sub_bi_coef_x[j] + (*img2)*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = (((*img3)*sub_bi_coef_x[j] + (*img4)*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			*outimg = (res_xy >> QRBAR_SHIFTBITS);
			outimg++;
			img1++;
			img2++;
			img3++;
			img4++;
		}
#else
		for (j = 0; j < desWidth; j++)
		{
			res_x0 = ((pImg_left_top[j] * sub_bi_coef_x[j] + pImg_right_top[j] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j] * sub_bi_coef_x[j] + pImg_right_down[j] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[i*desWidth + j] = (res_xy >> QRBAR_SHIFTBITS);
		}
#endif
	}

	free(coord_x);
	free(coord_y);
	free(bi_coef_x);
	free(bi_coef_y);
	free(sub_bi_coef_x);
	free(sub_bi_coef_y);
	free(pImg_left_top);
	free(pImg_left_down);
	free(pImg_right_top);
	free(pImg_right_down);
}


void _BilinearResize_8u_1D_1R_Color(const unsigned char * pSrcImg, unsigned char * pDesImg, int srcWidth, int srcHeight, int srcOffsetX, int srcOffsetY, int srcStride, int desWidth, int desHeight)
{
	int i, j;
	int nRateW, nRateH;
	int y;
	unsigned short *coord_x, *coord_y;
	unsigned char *bi_coef_x, *bi_coef_y;
	unsigned char *sub_bi_coef_x, *sub_bi_coef_y;
	unsigned char *pImg_left_top, *pImg_left_down;
	unsigned char *pImg_right_top, *pImg_right_down;
	int float_value;
	const short std_1_value = (1 << QRBAR_SHIFTBITS);
	const short std_and_value = ((1 << QRBAR_SHIFTBITS) - 1);
	int res_x0, res_x1;
	int res_xy;
	const unsigned char *pSrc, *pSrc1;

	pImg_left_top = (unsigned char *)malloc(sizeof(char)*desWidth * 3);
	pImg_left_down = (unsigned char *)malloc(sizeof(char)*desWidth * 3);
	pImg_right_top = (unsigned char *)malloc(sizeof(char)*desWidth * 3);
	pImg_right_down = (unsigned char *)malloc(sizeof(char)*desWidth * 3);

	coord_x = (unsigned short *)malloc(sizeof(short)*desWidth);
	coord_y = (unsigned short *)malloc(sizeof(short)*desHeight);
	bi_coef_x = (unsigned char *)malloc(sizeof(char)*desWidth);
	bi_coef_y = (unsigned char *)malloc(sizeof(char)*desHeight);
	sub_bi_coef_x = (unsigned char *)malloc(sizeof(char)*desWidth);
	sub_bi_coef_y = (unsigned char *)malloc(sizeof(char)*desHeight);

	if (coord_x == NULL || coord_y == NULL || bi_coef_x == NULL || bi_coef_y == NULL || sub_bi_coef_x == NULL || sub_bi_coef_y == NULL ||
		pImg_left_top == NULL || pImg_left_down == NULL || pImg_right_top == NULL || pImg_right_down == NULL)
		return;

	nRateW = (srcWidth << QRBAR_SHIFTBITS) / (desWidth);
	nRateH = (srcHeight << QRBAR_SHIFTBITS) / (desHeight);

	for (i = 0; i < desHeight; ++i)
	{
		float_value = i * nRateH;
		bi_coef_y[i] = float_value & std_and_value;
		if (!bi_coef_y[i])
			bi_coef_y[i] = 1;
		sub_bi_coef_y[i] = std_1_value - bi_coef_y[i];
		coord_y[i] = QRBAR_ROUND0(float_value);
		if (coord_y[i] > srcHeight - 2)
			coord_y[i] = srcHeight - 2;
	}

	for (i = 0; i < desWidth; ++i)
	{
		float_value = i * nRateW;
		bi_coef_x[i] = float_value & std_and_value;
		if (!bi_coef_x[i])
			bi_coef_x[i] = 1;
		sub_bi_coef_x[i] = std_1_value - bi_coef_x[i];
		coord_x[i] = QRBAR_ROUND0(float_value);
		if (coord_x[i] > srcWidth - 2)
			coord_x[i] = srcWidth - 2;
	}

	for (i = 0; i < desHeight; i++)
	{
		// pSrc = pSrcImg + coord_y[i] * srcWidth * 3;
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride*3 + srcOffsetX*3;
		y = coord_y[i];
		for (j = 0; j < desWidth; j++)
		{
			pSrc1 = pSrc;
			pSrc1 += coord_x[j] * 3;
			memcpy(pImg_left_top + j * 3, pSrc1, 3);
			memcpy(pImg_right_top + j * 3, pSrc1 + 3, 3);
			// pSrc1 += srcWidth * 3;
			pSrc1 += srcStride*3;
			memcpy(pImg_left_down + j * 3, pSrc1, 3);
			memcpy(pImg_right_down + j * 3, pSrc1 + 3, 3);
		}

#ifdef EAGLEEYE_NEON_OPTIMIZATION
		unsigned char *img1, *img2, *img3, *img4;
		unsigned char *outimg;
		unsigned char *coef1, *coef2, *coef3, *coef4;
		unsigned char *coef_x1, *coef_x2;
		uint8x8_t value_y1, value_y2;

		value_y1 = vdup_n_u8(sub_bi_coef_y[i]);
		value_y2 = vdup_n_u8(bi_coef_y[i]);
		img1 = pImg_left_top;
		img2 = pImg_right_top;
		img3 = pImg_left_down;
		img4 = pImg_right_down;
		outimg = pDesImg + i*desWidth*3;
		coef_x1 = sub_bi_coef_x;
		coef_x2 = bi_coef_x;
		for (j = 0; j < desWidth; j += 8)
		{
			// uint8x8_t v1, v2, v3, v4;
			uint8x8_t value_x1, value_x2;
			uint8x8_t bires_x0, bires_x1;
			uint8x8x3_t bires_xy;
			uint16x8_t multi_value;

			// v1 = vld1_u8(img1);
			// v2 = vld1_u8(img2);
			// v3 = vld1_u8(img3);
			// v4 = vld1_u8(img4);

			uint8x8x3_t v1_3, v2_3, v3_3, v4_3;
			v1_3 = vld3_u8(img1);
			v2_3 = vld3_u8(img2);
			v3_3 = vld3_u8(img3);
			v4_3 = vld3_u8(img4);

			value_x1 = vld1_u8(coef_x1);
			value_x2 = vld1_u8(coef_x2);
			// round 1
			multi_value = vmull_u8(v1_3.val[0], value_x1);
			multi_value = vmlal_u8(multi_value, v2_3.val[0], value_x2);
			bires_x0 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(v3_3.val[0], value_x1);
			multi_value = vmlal_u8(multi_value, v4_3.val[0], value_x2);
			bires_x1 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(bires_x0, value_y1);
			multi_value = vmlal_u8(multi_value, bires_x1, value_y2);
			bires_xy.val[0] = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);

			// round 2
			multi_value = vmull_u8(v1_3.val[1], value_x1);
			multi_value = vmlal_u8(multi_value, v2_3.val[1], value_x2);
			bires_x0 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(v3_3.val[1], value_x1);
			multi_value = vmlal_u8(multi_value, v4_3.val[1], value_x2);
			bires_x1 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(bires_x0, value_y1);
			multi_value = vmlal_u8(multi_value, bires_x1, value_y2);
			bires_xy.val[1] = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);

			// round 3
			multi_value = vmull_u8(v1_3.val[2], value_x1);
			multi_value = vmlal_u8(multi_value, v2_3.val[2], value_x2);
			bires_x0 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(v3_3.val[2], value_x1);
			multi_value = vmlal_u8(multi_value, v4_3.val[2], value_x2);
			bires_x1 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(bires_x0, value_y1);
			multi_value = vmlal_u8(multi_value, bires_x1, value_y2);
			bires_xy.val[2] = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);			
			vst3_u8(outimg, bires_xy);

			outimg += 24;
			img1 += 24;
			img2 += 24;
			img3 += 24;
			img4 += 24;
			coef_x1 += 8;
			coef_x2 += 8;
		}

		for (; j < desWidth; j++)
		{
			res_x0 = (((img1[0])*sub_bi_coef_x[j] + (img2[0])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = (((img3[0])*sub_bi_coef_x[j] + (img4[0])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			outimg[0] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = (((img1[1])*sub_bi_coef_x[j] + (img2[1])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = (((img3[1])*sub_bi_coef_x[j] + (img4[1])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			outimg[1] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = (((img1[2])*sub_bi_coef_x[j] + (img2[2])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = (((img3[2])*sub_bi_coef_x[j] + (img4[2])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			outimg[2] = (res_xy >> QRBAR_SHIFTBITS);

			outimg += 3;
			img1 += 3;
			img2 += 3;
			img3 += 3;
			img4 += 3;
		}
#else
		for (j = 0; j < desWidth; j++)
		{
			res_x0 = ((pImg_left_top[j * 3] * sub_bi_coef_x[j] + pImg_right_top[j * 3] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3] * sub_bi_coef_x[j] + pImg_right_down[j * 3] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desWidth + j) * 3] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = ((pImg_left_top[j * 3 + 1] * sub_bi_coef_x[j] + pImg_right_top[j * 3 + 1] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3 + 1] * sub_bi_coef_x[j] + pImg_right_down[j * 3 + 1] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desWidth + j) * 3 + 1] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = ((pImg_left_top[j * 3 + 2] * sub_bi_coef_x[j] + pImg_right_top[j * 3 + 2] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3 + 2] * sub_bi_coef_x[j] + pImg_right_down[j * 3 + 2] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desWidth + j) * 3 + 2] = (res_xy >> QRBAR_SHIFTBITS);
		}
#endif
	}

	free(coord_x);
	free(coord_y);
	free(bi_coef_x);
	free(bi_coef_y);
	free(sub_bi_coef_x);
	free(sub_bi_coef_y);
	free(pImg_left_top);
	free(pImg_left_down);
	free(pImg_right_top);
	free(pImg_right_down);
}

Matrix<Array<unsigned char, 3>> resize(const Matrix<Array<unsigned char, 3>> img,
										int after_r,int after_c,
										InterpMethod interp_method){
    Matrix<Array<unsigned char, 3>> dst(after_r, after_c);
    Matrix<Array<unsigned char, 3>> img_cp = img;
	unsigned int offset_r, offset_c;
	img.offset(offset_r, offset_c);

    unsigned char* dst_ptr = (unsigned char*)dst.dataptr();

    _BilinearResize_8u_1D_1R_Color((unsigned char*)img.dataptr(), dst_ptr,img.cols(), img.rows(),offset_c,offset_r,img.stride(),after_c, after_r);

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

    _BilinearResize_8u_1D_1R(img_ptr, dst_ptr,img.cols(), img.rows(),offset_c,offset_r,img.stride(),after_c, after_r);  
    return dst;                                         
}

void _BilinearResize_32f_C1(const float* pSrcImg, float* pDesImg, int srcWidth, int srcHeight, int srcOffsetX, int srcOffsetY, int srcStride, int dstWidth, int dstHeight){
	int i, j;
	float nRateW, nRateH;
	int y;
	unsigned short *coord_x, *coord_y;
	float* bi_coef_x,*bi_coef_y;
	float* sub_bi_coef_x,*sub_bi_coef_y;
	float *pImg_left_top, *pImg_left_down;
	float *pImg_right_top, *pImg_right_down;
	float float_value;
	float res_x0, res_x1;
	float res_xy;
	const float *pSrc, *pSrc1;

	pImg_left_top = (float *)malloc(sizeof(float)*dstWidth);
	pImg_left_down = (float *)malloc(sizeof(float)*dstWidth);
	pImg_right_top = (float *)malloc(sizeof(float)*dstWidth);
	pImg_right_down = (float *)malloc(sizeof(float)*dstWidth);

	coord_x = (unsigned short *)malloc(sizeof(short)*dstWidth);
	coord_y = (unsigned short *)malloc(sizeof(short)*dstHeight);
	bi_coef_x = (float *)malloc(sizeof(float)*dstWidth);
	bi_coef_y = (float *)malloc(sizeof(float)*dstHeight);
	sub_bi_coef_x = (float *)malloc(sizeof(float)*dstWidth);
	sub_bi_coef_y = (float *)malloc(sizeof(float)*dstHeight);

	if (coord_x == NULL || coord_y == NULL || bi_coef_x == NULL || bi_coef_y == NULL || sub_bi_coef_x == NULL || sub_bi_coef_y == NULL ||
		pImg_left_top == NULL || pImg_left_down == NULL || pImg_right_top == NULL || pImg_right_down == NULL)
		return;

    // nRateW = (srcWidth << QRBAR_SHIFTBITS) / (dstWidth);
    // nRateH = (srcHeight << QRBAR_SHIFTBITS) / (dstHeight);
	nRateW = float(srcWidth) / float(dstWidth);
	nRateH = float(srcHeight) / float(dstHeight);

	for (i = 0; i < dstHeight; ++i)
	{
		float_value = i * nRateH;
		coord_y[i] = (unsigned short)(float_value);
		bi_coef_y[i] = float_value - coord_y[i];
		sub_bi_coef_y[i] = 1.0f - bi_coef_y[i];
		if (coord_y[i] > srcHeight - 2)
			coord_y[i] = srcHeight - 2;
	}

	for (i = 0; i < dstWidth; ++i)
	{
		float_value = i * nRateW;
		coord_x[i] = (unsigned short)(float_value);
		bi_coef_x[i] = float_value - coord_x[i];
		sub_bi_coef_x[i] = 1.0f - bi_coef_x[i];		
		if (coord_x[i] > srcWidth - 2)
			coord_x[i] = srcWidth - 2;
	}

	for (i = 0; i < dstHeight; i++)
	{
		// pSrc = pSrcImg + coord_y[i] * srcWidth;
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride + srcOffsetX;
		for (j = 0; j < dstWidth; j++)
		{
			pSrc1 = pSrc;
			pSrc1 += coord_x[j];
			pImg_left_top[j] = *pSrc1;
			pImg_right_top[j] = *(pSrc1 + 1);
			// pSrc1 += srcWidth;
			pSrc1 += srcStride;
			pImg_left_down[j] = *pSrc1;
			pImg_right_down[j] = *(pSrc1 + 1);
		}

#ifdef EAGLEEYE_NEON_OPTIMIZATION
		float *img1, *img2, *img3, *img4;
		float *outimg;
		float *coef_x1, *coef_x2;
		float32x4_t value_y1, value_y2;
		value_y1 = vdupq_n_f32(sub_bi_coef_y[i]);
		value_y2 = vdupq_n_f32(bi_coef_y[i]);
		img1 = pImg_left_top;
		img2 = pImg_right_top;
		img3 = pImg_left_down;
		img4 = pImg_right_down;
		outimg = pDesImg + i*dstWidth;
		coef_x1 = sub_bi_coef_x;
		coef_x2 = bi_coef_x;

		for(j=0; j<dstWidth; j += 4){
			float32x4_t v1,v2,v3,v4;
			v1 = vld1q_f32(img1);
			v2 = vld1q_f32(img2);
			v3 = vld1q_f32(img3);
			v4 = vld1q_f32(img4);

			float32x4_t value_x1, value_x2;
			value_x1 = vld1q_f32(coef_x1);
			value_x2 = vld1q_f32(coef_x2);

			float32x4_t multi_value;
			float32x4_t bires_x0, bires_x1, bires_xy;

			multi_value = vmulq_f32(v1, value_x1);
			bires_x0 = vmlaq_f32(multi_value, v2, value_x2);

			multi_value = vmulq_f32(v3, value_x1);
			bires_x1 = vmlaq_f32(multi_value, v4, value_x2);

			multi_value = vmulq_f32(bires_x0, value_y1);
			bires_xy = vmlaq_f32(multi_value, bires_x1, value_y2);

			vst1q_f32(outimg, bires_xy);

			outimg += 4;
			img1 += 4;
			img2 += 4;
			img3 += 4;
			img4 += 4;
			coef_x1 += 4;
			coef_x2 += 4;
		}

		for (; j < dstWidth; j++)
		{
			res_x0 = ((*img1)*sub_bi_coef_x[j] + (*img2)*bi_coef_x[j]);
			res_x1 = ((*img3)*sub_bi_coef_x[j] + (*img4)*bi_coef_x[j]);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			*outimg = res_xy;
			outimg++;
			img1++;
			img2++;
			img3++;
			img4++;
		}
#else
		for (j = 0; j < dstWidth; j++){
			res_x0 = ((pImg_left_top[j])*sub_bi_coef_x[j] + (pImg_right_top[j])*bi_coef_x[j]);
			res_x1 = ((pImg_left_down[j])*sub_bi_coef_x[j] + (pImg_right_down[j])*bi_coef_x[j]);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[i*dstWidth + j] = res_xy;
		}
#endif
	}

	free(coord_x);
	free(coord_y);
	free(bi_coef_x);
	free(bi_coef_y);
	free(sub_bi_coef_x);
	free(sub_bi_coef_y);
	free(pImg_left_top);
	free(pImg_left_down);
	free(pImg_right_top);
	free(pImg_right_down);
}

Matrix<float> resize(const Matrix<float>  img,
					 int after_r,int after_c,
					 InterpMethod interp_method){
    Matrix<float> dst(after_r, after_c);
	unsigned int offset_r, offset_c;
	img.offset(offset_r, offset_c);

    const float* img_ptr = img.dataptr();
    float* dst_ptr = dst.dataptr();
	_BilinearResize_32f_C1(img_ptr, dst_ptr, img.cols(), img.rows(),offset_c, offset_r, img.stride(), after_c, after_r);
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
    _BilinearResize_8u_1D_1R_Color((unsigned char*)input.dataptr(), 
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

    _BilinearResize_8u_1D_1R_Color((unsigned char*)input.dataptr(), 
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
	_BilinearResize_32f_C1(img_ptr, 
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
	_BilinearResize_32f_C1(img_ptr, 
							output, 
							input.cols(), 
							input.rows(),
							offset_c, 
							offset_r, 
							input.stride(), 
							output_c, 
							output_r);
}
}