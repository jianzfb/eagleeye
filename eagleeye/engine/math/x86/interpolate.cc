#include "eagleeye/engine/math/x86/interpolate.h"
#include <string>
#include <vector>
#include <cmath>
#include <cstring>

namespace eagleeye{
namespace math{
namespace x86{
#ifndef QRBAR_SHIFTBITS
#define QRBAR_SHIFTBITS    8
#define QRBAR_ROUND0(x)  (x>>QRBAR_SHIFTBITS)
#endif
void bilinear_rgb_8u_3d_interp(
                    const unsigned char * pSrcImg, 
                    unsigned char * pDesImg, 
                    int srcWidth, 
                    int srcHeight, 
                    int srcOffsetX, 
                    int srcOffsetY, 
                    int srcStride, 
                    int desWidth, 
                    int desHeight,
					int desStride){
	if(desStride < 0){
		desStride = desWidth;
	}

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

	for (i = 0; i < desHeight; ++i){
		float_value = i * nRateH;
		bi_coef_y[i] = float_value & std_and_value;
		if (!bi_coef_y[i])
			bi_coef_y[i] = 1;
		sub_bi_coef_y[i] = std_1_value - bi_coef_y[i];
		coord_y[i] = QRBAR_ROUND0(float_value);
		if (coord_y[i] > srcHeight - 2)
			coord_y[i] = srcHeight - 2;
	}

	for (i = 0; i < desWidth; ++i){
		float_value = i * nRateW;
		bi_coef_x[i] = float_value & std_and_value;
		if (!bi_coef_x[i])
			bi_coef_x[i] = 1;
		sub_bi_coef_x[i] = std_1_value - bi_coef_x[i];
		coord_x[i] = QRBAR_ROUND0(float_value);
		if (coord_x[i] > srcWidth - 2)
			coord_x[i] = srcWidth - 2;
	}

    for(int i=0; i<desHeight; ++i){
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride*3 + srcOffsetX*3;
        for(int j=0; j<desWidth; ++j){

        }
    }

	for (i = 0; i < desHeight; i++){
		// pSrc = pSrcImg + coord_y[i] * srcWidth * 3;
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride*3 + srcOffsetX*3;
		y = coord_y[i];
		for (j = 0; j < desWidth; j++){
			pSrc1 = pSrc;
			pSrc1 += coord_x[j] * 3;
			memcpy(pImg_left_top + j * 3, pSrc1, 3);
			memcpy(pImg_right_top + j * 3, pSrc1 + 3, 3);
			// pSrc1 += srcWidth * 3;
			pSrc1 += srcStride*3;
			memcpy(pImg_left_down + j * 3, pSrc1, 3);
			memcpy(pImg_right_down + j * 3, pSrc1 + 3, 3);
		}

		for (j = 0; j < desWidth; j++){
			res_x0 = ((pImg_left_top[j * 3] * sub_bi_coef_x[j] + pImg_right_top[j * 3] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3] * sub_bi_coef_x[j] + pImg_right_down[j * 3] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desStride + j) * 3] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = ((pImg_left_top[j * 3 + 1] * sub_bi_coef_x[j] + pImg_right_top[j * 3 + 1] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3 + 1] * sub_bi_coef_x[j] + pImg_right_down[j * 3 + 1] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desStride + j) * 3 + 1] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = ((pImg_left_top[j * 3 + 2] * sub_bi_coef_x[j] + pImg_right_top[j * 3 + 2] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3 + 2] * sub_bi_coef_x[j] + pImg_right_down[j * 3 + 2] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desStride + j) * 3 + 2] = (res_xy >> QRBAR_SHIFTBITS);
		}
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

void bilinear_gray_8u_1d_interp(
                    const unsigned char * pSrcImg, 
                    unsigned char * pDesImg, 
                    int srcWidth, 
                    int srcHeight, 
                    int srcOffsetX, 
                    int srcOffsetY, 
                    int srcStride, 
                    int desWidth, 
                    int desHeight,
					int desStride){
	if(desStride < 0){
		desStride = desWidth;
	}

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

	for (i = 0; i < desHeight; ++i){
		float_value = i * nRateH;
		bi_coef_y[i] = float_value & std_and_value;
		if (!bi_coef_y[i])
			bi_coef_y[i] = 1;
		sub_bi_coef_y[i] = std_1_value - bi_coef_y[i];
		coord_y[i] = QRBAR_ROUND0(float_value);
		if (coord_y[i] > srcHeight - 2)
			coord_y[i] = srcHeight - 2;
	}

	for (i = 0; i < desWidth; ++i){
		float_value = i * nRateW;
		bi_coef_x[i] = float_value & std_and_value;
		if (!bi_coef_x[i])
			bi_coef_x[i] = 1;
		sub_bi_coef_x[i] = std_1_value - bi_coef_x[i];
		coord_x[i] = QRBAR_ROUND0(float_value);
		if (coord_x[i] > srcWidth - 2)
			coord_x[i] = srcWidth - 2;
	}

	for (i = 0; i < desHeight; i++){
		// pSrc = pSrcImg + coord_y[i] * srcWidth;
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride + srcOffsetX;
		y = coord_y[i];
		for (j = 0; j < desWidth; j++){
			pSrc1 = pSrc;
			pSrc1 += coord_x[j];
			pImg_left_top[j] = *pSrc1;
			pImg_right_top[j] = *(pSrc1 + 1);
			// pSrc1 += srcWidth;
			pSrc1 += srcStride;
			pImg_left_down[j] = *pSrc1;
			pImg_right_down[j] = *(pSrc1 + 1);
		}

		for (j = 0; j < desWidth; j++){
			res_x0 = ((pImg_left_top[j] * sub_bi_coef_x[j] + pImg_right_top[j] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j] * sub_bi_coef_x[j] + pImg_right_down[j] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[i*desStride + j] = (res_xy >> QRBAR_SHIFTBITS);
		}
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
}    
}
}