#include "eagleeye/common/EagleeyeRGBRotate.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
// android 默认开启neon
#include <arm_neon.h>
#endif
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <algorithm>

namespace eagleeye{
void rotate90_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in);

void rotate270_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in);

void rotate180_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in);

void bgr_rotate_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in, int angle){
    if (angle == 90){
        rotate90_hwc(src, dst, w_in, h_in);
    }
    if (angle == 270){
        rotate270_hwc(src, dst, w_in, h_in);
    }
    if (angle == 180){
        rotate180_hwc(src, dst, w_in, h_in);
    }

}

#if defined (__ARM_NEON) || defined (__ARM_NEON__)
/*
bgr1 bgr2 bgr3
bgr4 bgr5 bgr6
bgr7 bgr8 bgr9
rotate:
bgr7 bgr4 bgr1
bgr8 bgr5 bgr2
bgr9 bgr6 bgr3
*/
#ifdef __aarch64__
void rotate90_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    int w_out = h_in;
    int h_out = w_in;
    int win = w_in * 3;
    int wout = w_out * 3;
    int64_t stride_h = 4 * win;
    int64_t stride_h_w = 4 * win - 24;
    int ww = w_out - 8;
    // unsigned char* dst = new unsigned char[w_out * h_out * 3];
    //block 8*8. -- 8*8
    int i = 0;
    for (i = 0; i < h_in - 7; i += 8){
        const unsigned char* inptr0 = src + i * win;
        const unsigned char* inptr1 = inptr0 + win;
        const unsigned char* inptr2 = inptr1 + win;
        const unsigned char* inptr3 = inptr2 + win;

        asm volatile(
        "prfm   pldl1keep, [%[ptr0]]                \n"
                "prfm   pldl1keep, [%[ptr0], #64]   \n"
                "prfm   pldl1keep, [%[ptr1]]        \n"
                "prfm   pldl1keep, [%[ptr1], #64]   \n"
                "prfm   pldl1keep, [%[ptr2]]        \n"
                "prfm   pldl1keep, [%[ptr2], #64]   \n"
                "prfm   pldl1keep, [%[ptr3]]        \n"
                "prfm   pldl1keep, [%[ptr3], #64]   \n"
        :
        :[ptr0] "r"(inptr0), [ptr1] "r"(inptr1), [ptr2] "r"(inptr2), [ptr3] "r"(inptr3)
        :"memory"
        );
        int j = 0;
        for (; j < w_in - 7; j += 8){
            unsigned char* outptr0 = dst + j * wout + (ww - i) * 3;
            unsigned char* outptr1 = outptr0 + wout;
            unsigned char* outptr2 = outptr1 + wout;
            unsigned char* outptr3 = outptr2 + wout;
            unsigned char* outptr4 = outptr3 + wout;
            unsigned char* outptr5 = outptr4 + wout;
            unsigned char* outptr6 = outptr5 + wout;
            unsigned char* outptr7 = outptr6 + wout;
            asm volatile (
                "ld3  {v0.8b, v1.8b, v2.8b}, [%[inptr0]]    \n" //v0={00,01,02, 03, 04, 05, 06, 07}"
                "ld3  {v3.8b, v4.8b, v5.8b}, [%[inptr1]]    \n" //v0={10,11,12, 13, 14, 15, 16, 17}"
                "ld3  {v6.8b, v7.8b, v8.8b}, [%[inptr2]]    \n" //v0={20,21,22, 23, 24, 25, 26, 27}"
                "ld3  {v9.8b, v10.8b, v11.8b}, [%[inptr3]]    \n" //v0={30,31,32, 33, 34, 35, 36, 37}"

                "add %[inptr0], %[inptr0], %[stride_h] \n" //4 + 4*w_in
                "add %[inptr1], %[inptr1], %[stride_h] \n" //5
                "add %[inptr2], %[inptr2], %[stride_h] \n" //6
                "add %[inptr3], %[inptr3], %[stride_h] \n" //7

                //b
                "trn1 v12.8b, v0.8b, v3.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v15.8b, v6.8b, v9.8b             \n" //v4={20 30 22 32 24 34 26 36 }

                "trn2 v18.8b, v0.8b, v3.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v21.8b, v6.8b, v9.8b             \n" //v7={21 31 23 33 25 35 27 37 }

                //g
                "trn1 v13.8b, v1.8b, v4.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v16.8b, v7.8b, v10.8b             \n" //v4={20 30 22 32 24 34 26 36 }

                "trn2 v19.8b, v1.8b, v4.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v22.8b, v7.8b, v10.8b             \n" //v7={21 31 23 33 25 35 27 37 }

                //r
                "trn1 v14.8b, v2.8b, v5.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v17.8b, v8.8b, v11.8b             \n" //v4={20 30 22 32 24 34 26 36 }

                "trn2 v20.8b, v2.8b, v5.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v23.8b, v8.8b, v11.8b             \n" //v7={21 31 23 33 25 35 27 37 }

                //b1
                "trn1 v24.4h, v12.4h, v15.4h             \n" //v0={00 10 20 30 04 14 24 34}
                "trn1 v27.4h, v18.4h, v21.4h             \n" //v2={01 11 21 31 05 15 25 35}

                "trn2 v0.4h, v12.4h, v15.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v3.4h, v18.4h, v21.4h             \n" //v3={03 13 23 33 07 17 27 37}

                //g1
                "trn1 v25.4h, v13.4h, v16.4h             \n" //v0={00 10 20 30 04 14 24 34}
                "trn1 v28.4h, v19.4h, v22.4h             \n" //v2={01 11 21 31 05 15 25 35}

                "trn2 v1.4h, v13.4h, v16.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v4.4h, v19.4h, v22.4h             \n" //v3={03 13 23 33 07 17 27 37}

                //r1
                "trn1 v26.4h, v14.4h, v17.4h             \n" //v0={00 10 20 30 04 14 24 34}
                "trn1 v29.4h, v20.4h, v23.4h             \n" //v2={01 11 21 31 05 15 25 35}

                "trn2 v2.4h, v14.4h, v17.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v5.4h, v20.4h, v23.4h             \n" //v3={03 13 23 33 07 17 27 37}

                "ld3  {v12.8b, v13.8b, v14.8b}, [%[inptr0]]    \n" //v0={00,01,02, 03, 04, 05, 06, 07}"
                "ld3  {v15.8b, v16.8b, v17.8b}, [%[inptr1]]    \n" //v0={10,11,12, 13, 14, 15, 16, 17}"
                "ld3  {v6.8b, v7.8b, v8.8b}, [%[inptr2]]    \n" //v0={20,21,22, 23, 24, 25, 26, 27}"
                "ld3  {v9.8b, v10.8b, v11.8b}, [%[inptr3]]    \n" //v0={30,31,32, 33, 34, 35, 36, 37}"

                "sub %[inptr0], %[inptr0], %[stride_h_w] \n" //4 - 4*w_in + 8
                "sub %[inptr1], %[inptr1], %[stride_h_w] \n" //5
                "sub %[inptr2], %[inptr2], %[stride_h_w] \n" //6
                "sub %[inptr3], %[inptr3], %[stride_h_w] \n" //7

                //b2
                "trn1 v18.8b, v12.8b, v15.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v21.8b, v6.8b, v9.8b             \n" //v4={20 30 22 32 24 34 26 36 }
                //g2
                "trn1 v19.8b, v13.8b, v16.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v22.8b, v7.8b, v10.8b             \n" //v4={20 30 22 32 24 34 26 36 }
                //r2
                "trn1 v20.8b, v14.8b, v17.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v23.8b, v8.8b, v11.8b             \n" //v4={20 30 22 32 24 34 26 36 }

                "trn2 v12.8b, v12.8b, v15.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v13.8b, v13.8b, v16.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v14.8b, v14.8b, v17.8b             \n" //v5={01 11 03 13 05 15 07 17 }

                "trn2 v15.8b, v6.8b, v9.8b             \n" //v7={21 31 23 33 25 35 27 37 }
                "trn2 v16.8b, v7.8b, v10.8b             \n" //v7={21 31 23 33 25 35 27 37 }
                "trn2 v17.8b, v8.8b, v11.8b             \n" //v7={21 31 23 33 25 35 27 37 }

                //b2
                "trn1 v6.4h, v18.4h, v21.4h             \n" //v0={00 10 20 30 04 14 24 34}
                //g2
                "trn1 v7.4h, v19.4h, v22.4h             \n" //v0={00 10 20 30 04 14 24 34}
                //r2
                "trn1 v8.4h, v20.4h, v23.4h             \n" //v0={00 10 20 30 04 14 24 34}

                //bgr
                "trn1 v9.4h, v12.4h, v15.4h             \n" //v2={01 11 21 31 05 15 25 35}
                "trn1 v10.4h, v13.4h, v16.4h             \n" //v2={01 11 21 31 05 15 25 35}
                "trn1 v11.4h, v14.4h, v17.4h             \n" //v2={01 11 21 31 05 15 25 35}

                //bgr
                "trn2 v18.4h, v18.4h, v21.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v19.4h, v19.4h, v22.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v20.4h, v20.4h, v23.4h             \n" //v1={02 12 22 32 06 16 26 36}

                //bgr
                "trn2 v21.4h, v12.4h, v15.4h             \n" //v3={03 13 23 33 07 17 27 37}
                "trn2 v22.4h, v13.4h, v16.4h             \n" //v3={03 13 23 33 07 17 27 37}
                "trn2 v23.4h, v14.4h, v17.4h             \n" //v3={03 13 23 33 07 17 27 37}

                //b1 b2
                "trn1 v12.2s, v24.2s, v6.2s             \n" //v8={00 10 20 30 40 50 60 70} b
                "trn1 v13.2s, v25.2s, v7.2s             \n" //v6={00 10 20 30 40 50 60 70} g
                "trn1 v14.2s, v26.2s, v8.2s             \n" //v6={00 10 20 30 40 50 60 70} r

                //b1 b2
                "trn2 v15.2s, v24.2s, v6.2s             \n" //v8={04 14 24 34 44 54 64 74} b
                "trn2 v16.2s, v25.2s, v7.2s             \n" //v6={04 14 24 34 44 54 64 74} g
                "trn2 v17.2s, v26.2s, v8.2s             \n" //v6={04 14 24 34 44 54 64 74} r

                //b1 b2
                "trn1 v6.2s, v27.2s, v9.2s             \n" //v8={01 11 20 30 40 50 60 70} b
                "trn1 v7.2s, v28.2s, v10.2s             \n" //v6={01 10 20 30 40 50 60 70} g
                "trn1 v8.2s, v29.2s, v11.2s             \n" //v6={01 10 20 30 40 50 60 70} r

                "rev64  v12.8b, v12.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v13.8b, v13.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v14.8b, v14.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r
                "rev64  v15.8b, v15.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v16.8b, v16.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v17.8b, v17.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r

                //b1 b2
                "trn2 v24.2s, v27.2s, v9.2s             \n" //v8={05 10 20 30 40 50 60 70} b
                "trn2 v25.2s, v28.2s, v10.2s             \n" //v6={05 10 20 30 40 50 60 70} g
                "trn2 v26.2s, v29.2s, v11.2s             \n" //v6={05 10 20 30 40 50 60 70} r

                // "st3 {v12.8b, v13.8b, v14.8b}, [%[outptr0]], #24             \n" //00 10 20 30 04 14 24 34
                // "st3 {v15.8b, v16.8b, v17.8b}, [%[outptr4]], #24             \n" //02 12 22 32
                "st3 {v12.8b, v13.8b, v14.8b}, [%[outptr0]], #24             \n" //00 10 20 30 04 14 24 34
                "st3 {v15.8b, v16.8b, v17.8b}, [%[outptr4]], #24             \n" //02 12 22 32
                //b1 b2
                "trn1 v9.2s, v0.2s, v18.2s             \n" //v8={02 11 20 30 40 50 60 70} b
                "trn1 v10.2s, v1.2s, v19.2s             \n" //v6={02 10 20 30 40 50 60 70} g
                "trn1 v11.2s, v2.2s, v20.2s             \n" //v6={02 10 20 30 40 50 60 70} r

                "trn2 v27.2s, v0.2s, v18.2s             \n" //v8={06 11 20 30 40 50 60 70} b
                "trn2 v28.2s, v1.2s, v19.2s             \n" //v6={06 10 20 30 40 50 60 70} g
                "trn2 v29.2s, v2.2s, v20.2s             \n" //v6={06 10 20 30 40 50 60 70} r

                //b1 b2
                "trn1 v0.2s, v3.2s, v21.2s             \n" //v8={03 11 20 30 40 50 60 70} b
                "trn1 v1.2s, v4.2s, v22.2s             \n" //v6={03 10 20 30 40 50 60 70} g
                "trn1 v2.2s, v5.2s, v23.2s             \n" //v6={03 10 20 30 40 50 60 70} r

                "trn2 v18.2s, v3.2s, v21.2s             \n" //v8={07 11 20 30 40 50 60 70} b
                "trn2 v19.2s, v4.2s, v22.2s             \n" //v6={07 10 20 30 40 50 60 70} g
                "trn2 v20.2s, v5.2s, v23.2s             \n" //v6={07 10 20 30 40 50 60 70} r

                "rev64  v6.8b, v6.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v7.8b, v7.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v8.8b, v8.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r

                "rev64  v24.8b, v24.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v25.8b, v25.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v26.8b, v26.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r

                "rev64  v9.8b, v9.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v10.8b, v10.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v11.8b, v11.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r

                "rev64  v27.8b, v27.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v28.8b, v28.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v29.8b, v29.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r

                "rev64  v0.8b, v0.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v1.8b, v1.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v2.8b, v2.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r

                "rev64  v18.8b, v18.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v19.8b, v19.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v20.8b, v20.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r

                "st3 {v6.8b, v7.8b, v8.8b}, [%[outptr1]], #24             \n" //00 10 20 30 04 14 24 34
                "st3 {v24.8b, v25.8b, v26.8b}, [%[outptr5]], #24             \n" //02 12 22 32

                "st3 {v9.8b, v10.8b, v11.8b}, [%[outptr2]], #24             \n" //00 10 20 30 04 14 24 34
                "st3 {v27.8b, v28.8b, v29.8b}, [%[outptr6]], #24             \n" //02 12 22 32

                "st3 {v0.8b, v1.8b, v2.8b}, [%[outptr3]], #24             \n" //00 10 20 30 04 14 24 34
                "st3 {v18.8b, v19.8b, v20.8b}, [%[outptr7]], #24             \n" //02 12 22 32

            : [inptr0] "+r"(inptr0), [inptr1] "+r"(inptr1), [inptr2] "+r"(inptr2), [inptr3] "+r"(inptr3), \
              [outptr0] "+r"(outptr0), [outptr1] "+r"(outptr1), [outptr2] "+r"(outptr2), [outptr3] "+r"(outptr3), \
              [outptr4] "+r"(outptr4), [outptr5] "+r"(outptr5), [outptr6] "+r"(outptr6), [outptr7] "+r"(outptr7), \
              [stride_h] "+r"(stride_h), [stride_h_w] "+r"(stride_h_w)
            :
            : "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11",  "v12", "v13", "v14", "v15", \
               "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",  "v28", "v29", "v30"
            );
        }
        const unsigned char* inptr4 = inptr3 + win;
        const unsigned char* inptr5 = inptr4 + win;
        const unsigned char* inptr6 = inptr5 + win;
        const unsigned char* inptr7 = inptr6 + win;
        for (; j < w_in; j++){
            int tmpx = (ww - i) * 3;
            unsigned char* outptr = dst + j * wout + tmpx;
            *outptr++ = *inptr7++;
            *outptr++ = *inptr7++;
            *outptr++ = *inptr7++;

            *outptr++ = *inptr6++;
            *outptr++ = *inptr6++;
            *outptr++ = *inptr6++;

            *outptr++ = *inptr5++;
            *outptr++ = *inptr5++;
            *outptr++ = *inptr5++;

            *outptr++ = *inptr4++;
            *outptr++ = *inptr4++;
            *outptr++ = *inptr4++;

            *outptr++ = *inptr3++;
            *outptr++ = *inptr3++;
            *outptr++ = *inptr3++;

            *outptr++ = *inptr2++;
            *outptr++ = *inptr2++;
            *outptr++ = *inptr2++;

            *outptr++ = *inptr1++;
            *outptr++ = *inptr1++;
            *outptr++ = *inptr1++;

            *outptr++ = *inptr0++;
            *outptr++ = *inptr0++;
            *outptr++ = *inptr0++;
        }

    }
    for (; i < h_in; i++){
        const unsigned char* inptr0 = src + i * win;
        for (int j = 0; j < w_in; j++){
            unsigned char* outptr0 = dst + j * wout + (w_out - 1 - i) * 3;
            *outptr0++ = *inptr0++;
            *outptr0++ = *inptr0++;
            *outptr0++ = *inptr0++;
        }
    }
}
#else
void rotate90_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    int w_out = h_in;
    int h_out = w_in;
    int win = w_in * 3;
    int wout = w_out * 3;
    int hremain = h_in % 8;
    int stride_h = 4 * win;
    int stride_h_w = 4 * win - 24;
    int ww = w_out - 8;
    // printf("rotate90_hwc \n");
    // unsigned char* dst = new unsigned char[w_out * h_out * 3];
    //block 8*8. -- 8*8
    int i = 0;
    for (i = 0; i < h_in - 7; i += 8){
        const unsigned char* inptr0 = src + i * win;
        const unsigned char* inptr1 = inptr0 + win;
        const unsigned char* inptr2 = inptr1 + win;
        const unsigned char* inptr3 = inptr2 + win;
        const unsigned char* inptr4 = inptr3 + win;
        const unsigned char* inptr5 = inptr4 + win;
        const unsigned char* inptr6 = inptr5 + win;
        const unsigned char* inptr7 = inptr6 + win;
        asm volatile(
        "pld [%[ptr0]]                         @ preload a, 64byte\n"
            "pld [%[ptr0], #64]            @ preload a, 64byte\n"
            "pld [%[ptr1]]            @ preload a, 64byte\n"
            "pld [%[ptr1], #64]            @ preload a, 64byte\n"
            "pld [%[ptr2]]            @ preload a, 64byte\n"
            "pld [%[ptr2], #64]            @ preload a, 64byte\n"
            "pld [%[ptr3]]            @ preload a, 64byte\n"
            "pld [%[ptr3], #64]            @ preload a, 64byte\n"
            "pld [%[ptr4]]            @ preload a, 64byte\n"
            "pld [%[ptr4], #64]            @ preload a, 64byte\n"
            "pld [%[ptr5]]            @ preload a, 64byte\n"
            "pld [%[ptr5], #64]            @ preload a, 64byte\n"
            "pld [%[ptr6]]            @ preload a, 64byte\n"
            "pld [%[ptr6], #64]            @ preload a, 64byte\n"
            "pld [%[ptr7]]            @ preload a, 64byte\n"
            "pld [%[ptr7], #64]            @ preload a, 64byte\n"
        :
        :[ptr0] "r"(inptr0), [ptr1] "r"(inptr1), [ptr2] "r"(inptr2), [ptr3] "r"(inptr3), \
          [ptr4] "r"(inptr4), [ptr5] "r"(inptr5), [ptr6] "r"(inptr6), [ptr7] "r"(inptr7)
        :"memory"
        );
        int j = 0;
        /*
        for (j = 0; j < w_in - 7; j += 8){
           // printf("j: %d, inptr0: %x \n", j, inptr0);
            unsigned char* outptr0 = dst + j * wout + i * 3;
            unsigned char* outptr1 = outptr0 + wout;
            unsigned char* outptr2 = outptr1 + wout;
            unsigned char* outptr3 = outptr2 + wout;
            unsigned char* outptr4 = outptr3 + wout;
            unsigned char* outptr5 = outptr4 + wout;
            unsigned char* outptr6 = outptr5 + wout;
            unsigned char* outptr7 = outptr6 + wout;
            asm volatile (
                "vld3.8  {d0, d1, d2}, [%[inptr0]]   @ zip load r0, d0 =00 01 02 03 04 05 06 07\n"
                "vld3.8  {d3, d4, d5}, [%[inptr1]]   @ zip load r1, d2 =10 11 12 13 14 15 16 17\n"
                "vld3.8  {d6, d7, d8}, [%[inptr2]]   @ zip load r1, d4 =20 21 22 23 24 25 26 27\n"
                "vld3.8  {d9, d10, d11}, [%[inptr3]]   @ zip load r1, d6 = 30 31 32 33 34 35 36 37\n"

                "add %[inptr0], %[stride_h]             @ add\n" //4 + 4*w_in
                "add %[inptr1], %[stride_h]             @ add\n" //5
                "add %[inptr2], %[stride_h]             @ add\n" //6
                "add %[inptr3], %[stride_h]             @ add\n" //7

                "vtrn.8  d0, d3                  @ trans data: d0=r00,r10,r02,r12 04 14 06 16  d4=01 11 03 13 \n"
                "vtrn.8  d1, d4                  @ trans data: d0=r00,r10,r02,r12 04 14 06 16  d4=01 11 03 13 \n"
                "vtrn.8  d2, d5                  @ trans data: d0=r00,r10,r02,r12 04 14 06 16  d4=01 11 03 13 \n"

                "vtrn.8  d6, d9                  @ trans data: d8=r20,r30,r12,r32 24 34 26 36  d12=21 31 23 33 \n"
                "vtrn.8  d7, d10                  @ trans data: d8=r20,r30,r12,r32 24 34 26 36  d12=21 31 23 33 \n"
                "vtrn.8  d8, d11                  @ trans data: d8=r20,r30,r12,r32 24 34 26 36  d12=21 31 23 33 \n"

                "vld3.8  {d12, d13, d14}, [%[inptr0]]   @ zip load r0, d0 = 40 41 02 03 04 05 06 07\n"
                "vld3.8  {d15, d16, d17}, [%[inptr1]]   @ zip load r1, d2 = 50 51 12 13 14 15 16 17\n"
                "vld3.8  {d18, d19, d20}, [%[inptr2]]   @ zip load r1, d4 = 60 61 22 23 24 25 26 27\n"
                "vld3.8  {d21, d22, d23}, [%[inptr3]]   @ zip load r1, d6 = 70 71 32 33 34 35 36 37\n"

                // d0=r00,r10,r20,r30 04 14 24 34 d8 = 02 12 22 32 06 16 26 36
                "vtrn.16 d0, d6                  @ trans data: \n"
                "vtrn.16 d1, d7                  @ trans data: \n"
                "vtrn.16 d2, d8                  @ trans data: \n"
                // d4=01 11 21 31 05 15 25 35; d12 =r03 13 23 33 07 17 27 37
                "vtrn.16  d3, d9                  @ trans data:\n"
                "vtrn.16  d4, d10                  @ trans data:\n"
                "vtrn.16  d5, d11                  @ trans data:\n"

                "vtrn.8  d12, d15                  @ trans data: d0=r40,r50,r42,r52 04 14 06 16  d4=41 51 03 13 \n"
                "vtrn.8  d13, d16                  @ trans data: d0=r40,r50,r42,r52 04 14 06 16  d4=41 51 03 13 \n"
                "vtrn.8  d14, d17                  @ trans data: d0=r40,r50,r42,r52 04 14 06 16  d4=41 51 03 13 \n"

                "vtrn.8  d18, d21                  @ trans data: d8=r60,r70,r12,r32 24 34 26 36  d12=61 71 23 33 \n"
                "vtrn.8  d19, d22                  @ trans data: d8=r60,r70,r12,r32 24 34 26 36  d12=61 71 23 33 \n"
                "vtrn.8  d20, d23                  @ trans data: d8=r60,r70,r12,r32 24 34 26 36  d12=61 71 23 33 \n"

                // d0=r40,r50,r60,r70 44 54 64 74 d8 = 42 52 62 72 46 56 66 76
                "vtrn.16 d12, d18                  @ trans data: \n"
                "vtrn.16 d13, d19                  @ trans data: \n"
                "vtrn.16 d14, d20                  @ trans data: \n"
                // d4=41 51 61 71 45 55 65 75; d12 = 43 53 63 73 47 57 67 77
                "vtrn.16  d15, d21                  @ trans data:\n"
                "vtrn.16  d16, d22                  @ trans data:\n"
                "vtrn.16  d17, d23                  @ trans data:\n"

                //d0=00,10,20,30 40 50 60 70 d4 = 04 14 24 34 44 54 64 74
                "vtrn.32 d0, d12                  @ trans data: \n"
                "vtrn.32 d1, d13                  @ trans data: \n"
                "vtrn.32 d2, d14                  @ trans data: \n"

                //d0=02,12,22,32 d4 = 06 26 16 36
                "vtrn.32 d6, d18                  @ trans data: \n"
                "vtrn.32 d7, d19                  @ trans data: \n"
                "vtrn.32 d8, d20                  @ trans data: \n"

                // d4=01 11 21 31 41 51 61 71; d12 = 05 15 25 35 45 55 65 75
                "vtrn.32  d3, d15                  @ trans data: \n"
                "vtrn.32  d4, d16                  @ trans data: \n"
                "vtrn.32  d5, d17                  @ trans data: \n"

                //d0= 03 13 23 33  d4 = 07 17 27 37
                "vtrn.32 d9, d21                  @ trans data: \n"
                "vtrn.32 d10, d22                  @ trans data: \n"
                "vtrn.32 d11, d23                  @ trans data: \n"

                "sub %[inptr0], %[inptr0], %[stride_h_w] \n" //4 - 4*w_in + 8
                "sub %[inptr1], %[inptr1], %[stride_h_w] \n" //5
                "sub %[inptr2], %[inptr2], %[stride_h_w] \n" //6
                "sub %[inptr3], %[inptr3], %[stride_h_w] \n" //7

                "vst3.8  {d0, d1, d2},    [%[outptr0]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d12, d13, d14},    [%[outptr4]]!   @ write d0(q0,low),r00,r10 20 30\n"

                "vst3.8  {d6, d7, d8},    [%[outptr2]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d18, d19, d20},    [%[outptr6]]!   @ write d0(q0,low),r00,r10 20 30\n"

                "vst3.8  {d3, d4, d5},    [%[outptr1]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d15, d16, d17},    [%[outptr5]]!   @ write d0(q0,low),r00,r10 20 30\n"

                "vst3.8  {d9, d10, d11},    [%[outptr3]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d21, d22, d23},    [%[outptr7]]!   @ write d0(q0,low),r00,r10 20 30\n"

            : [inptr0] "+r"(inptr0), [inptr1] "+r"(inptr1), [inptr2] "+r"(inptr2), [inptr3] "+r"(inptr3), \
              [outptr0] "+r"(outptr0), [outptr1] "+r"(outptr1), [outptr2] "+r"(outptr2), [outptr3] "+r"(outptr3), \
              [outptr4] "+r"(outptr4), [outptr5] "+r"(outptr5), [outptr6] "+r"(outptr6), [outptr7] "+r"(outptr7)
            : [stride_h] "r" (stride_h), [stride_h_w] "r" (stride_h_w)
            : "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12"
            );
           //  printf("j: %d, outptr0: %x \n", j, outptr0);
        }
        */
        //printf("j: %d, inptr0: %x\n", j, inptr0);
        for (; j < w_in; j++){
            int tmpx = (ww - i) * 3;
            unsigned char* outptr = dst + j * wout + tmpx;
            *outptr++ = *inptr7++;
            *outptr++ = *inptr7++;
            *outptr++ = *inptr7++;

            *outptr++ = *inptr6++;
            *outptr++ = *inptr6++;
            *outptr++ = *inptr6++;

            *outptr++ = *inptr5++;
            *outptr++ = *inptr5++;
            *outptr++ = *inptr5++;

            *outptr++ = *inptr4++;
            *outptr++ = *inptr4++;
            *outptr++ = *inptr4++;

            *outptr++ = *inptr3++;
            *outptr++ = *inptr3++;
            *outptr++ = *inptr3++;

            *outptr++ = *inptr2++;
            *outptr++ = *inptr2++;
            *outptr++ = *inptr2++;

            *outptr++ = *inptr1++;
            *outptr++ = *inptr1++;
            *outptr++ = *inptr1++;

            *outptr++ = *inptr0++;
            *outptr++ = *inptr0++;
            *outptr++ = *inptr0++;
        }
    }
    ww = w_out - 1;
    for (; i < h_in; i++){
        const unsigned char* inptr0 = src + i * win;
        for (int j = 0; j < w_in; j++){
            unsigned char* outptr0 = dst + j * wout + (ww - i) * 3;
            *outptr0++ = *inptr0++;
            *outptr0++ = *inptr0++;
            *outptr0++ = *inptr0++;
        }
    }
}
#endif
/*
bgr1 bgr2 bgr3
bgr4 bgr5 bgr6
bgr7 bgr8 bgr9
rotate:
bgr3 bgr6 bgr9
bgr2 bgr5 bgr8
bgr1 bgr4 bgr7
*/
//dst = (h_out - 1) * w_out
//类似rotate90，将输出结果倒着输出 或者先rotate90,然后沿Y轴翻转
#ifdef __aarch64__
void rotate270_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    int w_out = h_in;
    int h_out = w_in;
    int win = w_in * 3;
    int wout = w_out * 3;
    int64_t stride_h = 4 * win;
    int64_t stride_h_w = 4 * win - 24;
    int hout = h_out - 1;
    //block 8*8. -- 8*8
    int i = 0;
    for (; i < h_in - 7; i += 8){
        const unsigned char* inptr0 = src + i * win;
        const unsigned char* inptr1 = inptr0 + win;
        const unsigned char* inptr2 = inptr1 + win;
        const unsigned char* inptr3 = inptr2 + win;
        // const unsigned char* inptr4 = inptr3 + w_in;
        // const unsigned char* inptr5 = inptr4 + w_in;
        // const unsigned char* inptr6 = inptr5 + w_in;
        // const unsigned char* inptr7 = inptr6 + w_in;

        asm volatile(
        "prfm   pldl1keep, [%[ptr0]]                \n"
                "prfm   pldl1keep, [%[ptr0], #64]   \n"
                "prfm   pldl1keep, [%[ptr1]]        \n"
                "prfm   pldl1keep, [%[ptr1], #64]   \n"
                "prfm   pldl1keep, [%[ptr2]]        \n"
                "prfm   pldl1keep, [%[ptr2], #64]   \n"
                "prfm   pldl1keep, [%[ptr3]]        \n"
                "prfm   pldl1keep, [%[ptr3], #64]   \n"
                // "prfm   pldl1keep, [%[ptr4]]        \n"
                // "prfm   pldl1keep, [%[ptr4], #64]   \n"
                // "prfm   pldl1keep, [%[ptr5]]        \n"
                // "prfm   pldl1keep, [%[ptr5], #64]   \n"
        :
        :[ptr0] "r"(inptr0), [ptr1] "r"(inptr1), [ptr2] "r"(inptr2), [ptr3] "r"(inptr3)
        :"memory"
        );
        int j = 0;
        for (; j < w_in - 7; j += 8){
            unsigned char* outptr0 = dst + (hout - j) * wout + i * 3;
            unsigned char* outptr1 = outptr0 - wout;
            unsigned char* outptr2 = outptr1 - wout;
            unsigned char* outptr3 = outptr2 - wout;
            unsigned char* outptr4 = outptr3 - wout;
            unsigned char* outptr5 = outptr4 - wout;
            unsigned char* outptr6 = outptr5 - wout;
            unsigned char* outptr7 = outptr6 - wout;
            // printf("outptr0: %x, inptr0: %x \n", outptr0, inptr0);
            // printf("inptr0: %d, %d, %d, %d \n", inptr0[0], inptr0[1], inptr0[2], inptr0[3]);
            asm volatile (
                "ld3  {v0.8b, v1.8b, v2.8b}, [%[inptr0]]    \n" //v0={00,01,02, 03, 04, 05, 06, 07}"
                "ld3  {v3.8b, v4.8b, v5.8b}, [%[inptr1]]    \n" //v0={10,11,12, 13, 14, 15, 16, 17}"
                "ld3  {v6.8b, v7.8b, v8.8b}, [%[inptr2]]    \n" //v0={20,21,22, 23, 24, 25, 26, 27}"
                "ld3  {v9.8b, v10.8b, v11.8b}, [%[inptr3]]    \n" //v0={30,31,32, 33, 34, 35, 36, 37}"

                "add %[inptr0], %[inptr0], %[stride_h] \n" //4 + 4*w_in
                "add %[inptr1], %[inptr1], %[stride_h] \n" //5
                "add %[inptr2], %[inptr2], %[stride_h] \n" //6
                "add %[inptr3], %[inptr3], %[stride_h] \n" //7

                //b
                "trn1 v12.8b, v0.8b, v3.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v15.8b, v6.8b, v9.8b             \n" //v4={20 30 22 32 24 34 26 36 }

                "trn2 v18.8b, v0.8b, v3.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v21.8b, v6.8b, v9.8b             \n" //v7={21 31 23 33 25 35 27 37 }

                //g
                "trn1 v13.8b, v1.8b, v4.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v16.8b, v7.8b, v10.8b             \n" //v4={20 30 22 32 24 34 26 36 }

                "trn2 v19.8b, v1.8b, v4.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v22.8b, v7.8b, v10.8b             \n" //v7={21 31 23 33 25 35 27 37 }

                //r
                "trn1 v14.8b, v2.8b, v5.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v17.8b, v8.8b, v11.8b             \n" //v4={20 30 22 32 24 34 26 36 }

                "trn2 v20.8b, v2.8b, v5.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v23.8b, v8.8b, v11.8b             \n" //v7={21 31 23 33 25 35 27 37 }

                //b1
                "trn1 v24.4h, v12.4h, v15.4h             \n" //v0={00 10 20 30 04 14 24 34}
                "trn1 v27.4h, v18.4h, v21.4h             \n" //v2={01 11 21 31 05 15 25 35}

                "trn2 v0.4h, v12.4h, v15.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v3.4h, v18.4h, v21.4h             \n" //v3={03 13 23 33 07 17 27 37}

                //g1
                "trn1 v25.4h, v13.4h, v16.4h             \n" //v0={00 10 20 30 04 14 24 34}
                "trn1 v28.4h, v19.4h, v22.4h             \n" //v2={01 11 21 31 05 15 25 35}

                "trn2 v1.4h, v13.4h, v16.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v4.4h, v19.4h, v22.4h             \n" //v3={03 13 23 33 07 17 27 37}

                //r1
                "trn1 v26.4h, v14.4h, v17.4h             \n" //v0={00 10 20 30 04 14 24 34}
                "trn1 v29.4h, v20.4h, v23.4h             \n" //v2={01 11 21 31 05 15 25 35}

                "trn2 v2.4h, v14.4h, v17.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v5.4h, v20.4h, v23.4h             \n" //v3={03 13 23 33 07 17 27 37}

                "ld3  {v12.8b, v13.8b, v14.8b}, [%[inptr0]]    \n" //v0={00,01,02, 03, 04, 05, 06, 07}"
                "ld3  {v15.8b, v16.8b, v17.8b}, [%[inptr1]]    \n" //v0={10,11,12, 13, 14, 15, 16, 17}"
                "ld3  {v6.8b, v7.8b, v8.8b}, [%[inptr2]]    \n" //v0={20,21,22, 23, 24, 25, 26, 27}"
                "ld3  {v9.8b, v10.8b, v11.8b}, [%[inptr3]]    \n" //v0={30,31,32, 33, 34, 35, 36, 37}"

                "sub %[inptr0], %[inptr0], %[stride_h_w] \n" //4 - 4*w_in + 8
                "sub %[inptr1], %[inptr1], %[stride_h_w] \n" //5
                "sub %[inptr2], %[inptr2], %[stride_h_w] \n" //6
                "sub %[inptr3], %[inptr3], %[stride_h_w] \n" //7

                //b2
                "trn1 v18.8b, v12.8b, v15.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v21.8b, v6.8b, v9.8b             \n" //v4={20 30 22 32 24 34 26 36 }
                //g2
                "trn1 v19.8b, v13.8b, v16.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v22.8b, v7.8b, v10.8b             \n" //v4={20 30 22 32 24 34 26 36 }
                //r2
                "trn1 v20.8b, v14.8b, v17.8b             \n" //v4={00 10 02 12 04 14 06 16 }
                "trn1 v23.8b, v8.8b, v11.8b             \n" //v4={20 30 22 32 24 34 26 36 }

                "trn2 v12.8b, v12.8b, v15.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v13.8b, v13.8b, v16.8b             \n" //v5={01 11 03 13 05 15 07 17 }
                "trn2 v14.8b, v14.8b, v17.8b             \n" //v5={01 11 03 13 05 15 07 17 }

                "trn2 v15.8b, v6.8b, v9.8b             \n" //v7={21 31 23 33 25 35 27 37 }
                "trn2 v16.8b, v7.8b, v10.8b             \n" //v7={21 31 23 33 25 35 27 37 }
                "trn2 v17.8b, v8.8b, v11.8b             \n" //v7={21 31 23 33 25 35 27 37 }

                //b2
                "trn1 v6.4h, v18.4h, v21.4h             \n" //v0={00 10 20 30 04 14 24 34}
                //g2
                "trn1 v7.4h, v19.4h, v22.4h             \n" //v0={00 10 20 30 04 14 24 34}
                //r2
                "trn1 v8.4h, v20.4h, v23.4h             \n" //v0={00 10 20 30 04 14 24 34}

                //bgr
                "trn1 v9.4h, v12.4h, v15.4h             \n" //v2={01 11 21 31 05 15 25 35}
                "trn1 v10.4h, v13.4h, v16.4h             \n" //v2={01 11 21 31 05 15 25 35}
                "trn1 v11.4h, v14.4h, v17.4h             \n" //v2={01 11 21 31 05 15 25 35}

                //bgr
                "trn2 v18.4h, v18.4h, v21.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v19.4h, v19.4h, v22.4h             \n" //v1={02 12 22 32 06 16 26 36}
                "trn2 v20.4h, v20.4h, v23.4h             \n" //v1={02 12 22 32 06 16 26 36}

                //bgr
                "trn2 v21.4h, v12.4h, v15.4h             \n" //v3={03 13 23 33 07 17 27 37}
                "trn2 v22.4h, v13.4h, v16.4h             \n" //v3={03 13 23 33 07 17 27 37}
                "trn2 v23.4h, v14.4h, v17.4h             \n" //v3={03 13 23 33 07 17 27 37}

                //b1 b2
                "trn1 v12.2s, v24.2s, v6.2s             \n" //v8={00 10 20 30 40 50 60 70} b
                "trn1 v13.2s, v25.2s, v7.2s             \n" //v6={00 10 20 30 40 50 60 70} g
                "trn1 v14.2s, v26.2s, v8.2s             \n" //v6={00 10 20 30 40 50 60 70} r

                //b1 b2
                "trn2 v15.2s, v24.2s, v6.2s             \n" //v8={04 14 24 34 44 54 64 74} b
                "trn2 v16.2s, v25.2s, v7.2s             \n" //v6={04 14 24 34 44 54 64 74} g
                "trn2 v17.2s, v26.2s, v8.2s             \n" //v6={04 14 24 34 44 54 64 74} r

                //b1 b2
                "trn1 v6.2s, v27.2s, v9.2s             \n" //v8={01 11 20 30 40 50 60 70} b
                "trn1 v7.2s, v28.2s, v10.2s             \n" //v6={01 10 20 30 40 50 60 70} g
                "trn1 v8.2s, v29.2s, v11.2s             \n" //v6={01 10 20 30 40 50 60 70} r

                //b1 b2
                "trn2 v24.2s, v27.2s, v9.2s             \n" //v8={05 10 20 30 40 50 60 70} b
                "trn2 v25.2s, v28.2s, v10.2s             \n" //v6={05 10 20 30 40 50 60 70} g
                "trn2 v26.2s, v29.2s, v11.2s             \n" //v6={05 10 20 30 40 50 60 70} r

                "st3 {v12.8b, v13.8b, v14.8b}, [%[outptr0]], #24             \n" //00 10 20 30 04 14 24 34
                "st3 {v15.8b, v16.8b, v17.8b}, [%[outptr4]], #24             \n" //02 12 22 32
                //b1 b2
                "trn1 v9.2s, v0.2s, v18.2s             \n" //v8={02 11 20 30 40 50 60 70} b
                "trn1 v10.2s, v1.2s, v19.2s             \n" //v6={02 10 20 30 40 50 60 70} g
                "trn1 v11.2s, v2.2s, v20.2s             \n" //v6={02 10 20 30 40 50 60 70} r

                "trn2 v27.2s, v0.2s, v18.2s             \n" //v8={06 11 20 30 40 50 60 70} b
                "trn2 v28.2s, v1.2s, v19.2s             \n" //v6={06 10 20 30 40 50 60 70} g
                "trn2 v29.2s, v2.2s, v20.2s             \n" //v6={06 10 20 30 40 50 60 70} r

                //b1 b2
                "trn1 v0.2s, v3.2s, v21.2s             \n" //v8={03 11 20 30 40 50 60 70} b
                "trn1 v1.2s, v4.2s, v22.2s             \n" //v6={03 10 20 30 40 50 60 70} g
                "trn1 v2.2s, v5.2s, v23.2s             \n" //v6={03 10 20 30 40 50 60 70} r

                "trn2 v18.2s, v3.2s, v21.2s             \n" //v8={07 11 20 30 40 50 60 70} b
                "trn2 v19.2s, v4.2s, v22.2s             \n" //v6={07 10 20 30 40 50 60 70} g
                "trn2 v20.2s, v5.2s, v23.2s             \n" //v6={07 10 20 30 40 50 60 70} r

                "st3 {v6.8b, v7.8b, v8.8b}, [%[outptr1]], #24             \n" //00 10 20 30 04 14 24 34
                "st3 {v24.8b, v25.8b, v26.8b}, [%[outptr5]], #24             \n" //02 12 22 32

                "st3 {v9.8b, v10.8b, v11.8b}, [%[outptr2]], #24             \n" //00 10 20 30 04 14 24 34
                "st3 {v27.8b, v28.8b, v29.8b}, [%[outptr6]], #24             \n" //02 12 22 32

                "st3 {v0.8b, v1.8b, v2.8b}, [%[outptr3]], #24             \n" //00 10 20 30 04 14 24 34
                "st3 {v18.8b, v19.8b, v20.8b}, [%[outptr7]], #24             \n" //02 12 22 32

            : [inptr0] "+r"(inptr0), [inptr1] "+r"(inptr1), [inptr2] "+r"(inptr2), [inptr3] "+r"(inptr3), \
              [outptr0] "+r"(outptr0), [outptr1] "+r"(outptr1), [outptr2] "+r"(outptr2), [outptr3] "+r"(outptr3), \
              [outptr4] "+r"(outptr4), [outptr5] "+r"(outptr5), [outptr6] "+r"(outptr6), [outptr7] "+r"(outptr7), \
              [stride_h] "+r"(stride_h), [stride_h_w] "+r"(stride_h_w)
            :
            : "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11",  "v12", "v13", "v14", "v15", \
               "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",  "v28", "v29"
            );
            //  printf("outptr0: %x, inptr0: %x \n", outptr0, inptr0);
            // // printf("outptr0: %d, %d, %d, %d \n", outptr0[0], outptr0[1], outptr0[2], outptr0[3]);
            // outptr0 -= 8;
            // printf("outptr0: %d, %d, %d, %d \n", outptr0[0], outptr0[1], outptr0[2], outptr0[3]);
            // printf("outptr0: %d, %d, %d, %d \n", outptr0[4], outptr0[5], outptr0[6], outptr0[7]);
        }
        const unsigned char* inptr4 = inptr3 + win;
        const unsigned char* inptr5 = inptr4 + win;
        const unsigned char* inptr6 = inptr5 + win;
        const unsigned char* inptr7 = inptr6 + win;
        // printf("inptr3: %x, inptr4: %x, inptr5: %x \n", inptr3, inptr4, inptr5);
        for (; j < w_in; j++){
            int tmpx = i * 3;
            unsigned char* outptr = dst + (hout - j) * wout + tmpx;
            *outptr++ = *inptr0++;
            *outptr++ = *inptr0++;
            *outptr++ = *inptr0++;

            *outptr++ = *inptr1++;
            *outptr++ = *inptr1++;
            *outptr++ = *inptr1++;

            *outptr++ = *inptr2++;
            *outptr++ = *inptr2++;
            *outptr++ = *inptr2++;

            *outptr++ = *inptr3++;
            *outptr++ = *inptr3++;
            *outptr++ = *inptr3++;

            *outptr++ = *inptr4++;
            *outptr++ = *inptr4++;
            *outptr++ = *inptr4++;

            *outptr++ = *inptr5++;
            *outptr++ = *inptr5++;
            *outptr++ = *inptr5++;

            *outptr++ = *inptr6++;
            *outptr++ = *inptr6++;
            *outptr++ = *inptr6++;

            *outptr++ = *inptr7++;
            *outptr++ = *inptr7++;
            *outptr++ = *inptr7++;
        }

    }
    for (; i < h_in; i++){
        const unsigned char* inptr0 = src + i * win;
        for (int j = 0; j < w_in; j++){
            unsigned char* outptr0 = dst + (hout - j) * wout + i * 3;
            *outptr0++ = *inptr0++;
            *outptr0++ = *inptr0++;
            *outptr0++ = *inptr0++;
        }
    }
}
#else
void rotate270_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    int w_out = h_in;
    int h_out = w_in;
    int win = w_in * 3;
    int wout = w_out * 3;
    int hremain = h_in % 8;
    int stride_h = 4 * win;
    int stride_h_w = 4 * win - 24;
    int hout = h_out - 1;
    //block 8*8. -- 8*8
    int i = 0;
    for (; i < h_in - 7; i += 8){
        const unsigned char* inptr0 = src + i * win;
        const unsigned char* inptr1 = inptr0 + win;
        const unsigned char* inptr2 = inptr1 + win;
        const unsigned char* inptr3 = inptr2 + win;
        const unsigned char* inptr4 = inptr3 + win;
        const unsigned char* inptr5 = inptr4 + win;
        const unsigned char* inptr6 = inptr5 + win;
        const unsigned char* inptr7 = inptr6 + win;
        asm volatile(
        "pld [%[ptr0]]                         @ preload a, 64byte\n"
            "pld [%[ptr0], #64]            @ preload a, 64byte\n"
            "pld [%[ptr1]]            @ preload a, 64byte\n"
            "pld [%[ptr1], #64]            @ preload a, 64byte\n"
            "pld [%[ptr2]]            @ preload a, 64byte\n"
            "pld [%[ptr2], #64]            @ preload a, 64byte\n"
            "pld [%[ptr3]]            @ preload a, 64byte\n"
            "pld [%[ptr3], #64]            @ preload a, 64byte\n"
            "pld [%[ptr4]]            @ preload a, 64byte\n"
            "pld [%[ptr4], #64]            @ preload a, 64byte\n"
            "pld [%[ptr5]]            @ preload a, 64byte\n"
            "pld [%[ptr5], #64]            @ preload a, 64byte\n"
            "pld [%[ptr6]]            @ preload a, 64byte\n"
            "pld [%[ptr6], #64]            @ preload a, 64byte\n"
            "pld [%[ptr7]]            @ preload a, 64byte\n"
            "pld [%[ptr7], #64]            @ preload a, 64byte\n"
        :
        :[ptr0] "r"(inptr0), [ptr1] "r"(inptr1), [ptr2] "r"(inptr2), [ptr3] "r"(inptr3), \
          [ptr4] "r"(inptr4), [ptr5] "r"(inptr5), [ptr6] "r"(inptr6), [ptr7] "r"(inptr7)
        :"memory"
        );
        int j = 0;

    /*for (; j < w_in - 7; j += 8){
           // printf("j: %d, inptr0: %x \n", j, inptr0);
            unsigned char* outptr0 = dst + (hout - j) * wout + i * 3;
            unsigned char* outptr1 = outptr0 - wout;
            unsigned char* outptr2 = outptr1 - wout;
            unsigned char* outptr3 = outptr2 - wout;
            unsigned char* outptr4 = outptr3 - wout;
            unsigned char* outptr5 = outptr4 - wout;
            unsigned char* outptr6 = outptr5 - wout;
            unsigned char* outptr7 = outptr6 - wout;
            asm volatile (
                "vld3.8  {d0, d1, d2}, [%[inptr0]]   @ zip load r0, d0 =00 01 02 03 04 05 06 07\n"
                "vld3.8  {d3, d4, d5}, [%[inptr1]]   @ zip load r1, d2 =10 11 12 13 14 15 16 17\n"
                "vld3.8  {d6, d7, d8}, [%[inptr2]]   @ zip load r1, d4 =20 21 22 23 24 25 26 27\n"
                "vld3.8  {d9, d10, d11}, [%[inptr3]]   @ zip load r1, d6 = 30 31 32 33 34 35 36 37\n"

                "add %[inptr0], %[stride_h]             @ add\n" //4 + 4*w_in
                "add %[inptr1], %[stride_h]             @ add\n" //5
                "add %[inptr2], %[stride_h]             @ add\n" //6
                "add %[inptr3], %[stride_h]             @ add\n" //7

                "vtrn.8  d0, d3                  @ trans data: d0=r00,r10,r02,r12 04 14 06 16  d4=01 11 03 13 \n"
                "vtrn.8  d1, d4                  @ trans data: d0=r00,r10,r02,r12 04 14 06 16  d4=01 11 03 13 \n"
                "vtrn.8  d2, d5                  @ trans data: d0=r00,r10,r02,r12 04 14 06 16  d4=01 11 03 13 \n"

                "vtrn.8  d6, d9                  @ trans data: d8=r20,r30,r12,r32 24 34 26 36  d12=21 31 23 33 \n"
                "vtrn.8  d7, d10                  @ trans data: d8=r20,r30,r12,r32 24 34 26 36  d12=21 31 23 33 \n"
                "vtrn.8  d8, d11                  @ trans data: d8=r20,r30,r12,r32 24 34 26 36  d12=21 31 23 33 \n"

                "vld3.8  {d12, d13, d14}, [%[inptr0]]   @ zip load r0, d0 = 40 41 02 03 04 05 06 07\n"
                "vld3.8  {d15, d16, d17}, [%[inptr1]]   @ zip load r1, d2 = 50 51 12 13 14 15 16 17\n"
                "vld3.8  {d18, d19, d20}, [%[inptr2]]   @ zip load r1, d4 = 60 61 22 23 24 25 26 27\n"
                "vld3.8  {d21, d22, d23}, [%[inptr3]]   @ zip load r1, d6 = 70 71 32 33 34 35 36 37\n"

                // d0=r00,r10,r20,r30 04 14 24 34 d8 = 02 12 22 32 06 16 26 36
                "vtrn.16 d0, d6                  @ trans data: \n"
                "vtrn.16 d1, d7                  @ trans data: \n"
                "vtrn.16 d2, d8                  @ trans data: \n"
                // d4=01 11 21 31 05 15 25 35; d12 =r03 13 23 33 07 17 27 37
                "vtrn.16  d3, d9                  @ trans data:\n"
                "vtrn.16  d4, d10                  @ trans data:\n"
                "vtrn.16  d5, d11                  @ trans data:\n"

                "vtrn.8  d12, d15                  @ trans data: d0=r40,r50,r42,r52 04 14 06 16  d4=41 51 03 13 \n"
                "vtrn.8  d13, d16                  @ trans data: d0=r40,r50,r42,r52 04 14 06 16  d4=41 51 03 13 \n"
                "vtrn.8  d14, d17                  @ trans data: d0=r40,r50,r42,r52 04 14 06 16  d4=41 51 03 13 \n"

                "vtrn.8  d18, d21                  @ trans data: d8=r60,r70,r12,r32 24 34 26 36  d12=61 71 23 33 \n"
                "vtrn.8  d19, d22                  @ trans data: d8=r60,r70,r12,r32 24 34 26 36  d12=61 71 23 33 \n"
                "vtrn.8  d20, d23                  @ trans data: d8=r60,r70,r12,r32 24 34 26 36  d12=61 71 23 33 \n"

                // d0=r40,r50,r60,r70 44 54 64 74 d8 = 42 52 62 72 46 56 66 76
                "vtrn.16 d12, d18                  @ trans data: \n"
                "vtrn.16 d13, d19                  @ trans data: \n"
                "vtrn.16 d14, d20                  @ trans data: \n"
                // d4=41 51 61 71 45 55 65 75; d12 = 43 53 63 73 47 57 67 77
                "vtrn.16  d15, d21                  @ trans data:\n"
                "vtrn.16  d16, d22                  @ trans data:\n"
                "vtrn.16  d17, d23                  @ trans data:\n"

                //d0=00,10,20,30 40 50 60 70 d4 = 04 14 24 34 44 54 64 74
                "vtrn.32 d0, d12                  @ trans data: \n"
                "vtrn.32 d1, d13                  @ trans data: \n"
                "vtrn.32 d2, d14                  @ trans data: \n"

                //d0=02,12,22,32 d4 = 06 26 16 36
                "vtrn.32 d6, d18                  @ trans data: \n"
                "vtrn.32 d7, d19                  @ trans data: \n"
                "vtrn.32 d8, d20                  @ trans data: \n"

                // d4=01 11 21 31 41 51 61 71; d12 = 05 15 25 35 45 55 65 75
                "vtrn.32  d3, d15                  @ trans data: \n"
                "vtrn.32  d4, d16                  @ trans data: \n"
                "vtrn.32  d5, d17                  @ trans data: \n"

                //d0= 03 13 23 33  d4 = 07 17 27 37
                "vtrn.32 d9, d21                  @ trans data: \n"
                "vtrn.32 d10, d22                  @ trans data: \n"
                "vtrn.32 d11, d23                  @ trans data: \n"

                "sub %[inptr0], %[inptr0], %[stride_h_w] \n" //4 - 4*w_in + 8
                "sub %[inptr1], %[inptr1], %[stride_h_w] \n" //5
                "sub %[inptr2], %[inptr2], %[stride_h_w] \n" //6
                "sub %[inptr3], %[inptr3], %[stride_h_w] \n" //7

                "vst3.8  {d0, d1, d2},    [%[outptr0]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d12, d13, d14},    [%[outptr4]]!   @ write d0(q0,low),r00,r10 20 30\n"

                "vst3.8  {d6, d7, d8},    [%[outptr2]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d18, d19, d20},    [%[outptr6]]!   @ write d0(q0,low),r00,r10 20 30\n"

                "vst3.8  {d3, d4, d5},    [%[outptr1]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d15, d16, d17},    [%[outptr5]]!   @ write d0(q0,low),r00,r10 20 30\n"

                "vst3.8  {d9, d10, d11},    [%[outptr3]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d21, d22, d23},    [%[outptr7]]!   @ write d0(q0,low),r00,r10 20 30\n"

            : [inptr0] "+r"(inptr0), [inptr1] "+r"(inptr1), [inptr2] "+r"(inptr2), [inptr3] "+r"(inptr3), \
              [outptr0] "+r"(outptr0), [outptr1] "+r"(outptr1), [outptr2] "+r"(outptr2), [outptr3] "+r"(outptr3), \
              [outptr4] "+r"(outptr4), [outptr5] "+r"(outptr5), [outptr6] "+r"(outptr6), [outptr7] "+r"(outptr7)
            : [stride_h] "r" (stride_h), [stride_h_w] "r" (stride_h_w)
            : "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12"
            );
           //  printf("j: %d, outptr0: %x \n", j, outptr0);
        }
        */
        for (; j < w_in; j++){
            int tmpx = i * 3;
            unsigned char* outptr = dst + (hout - j) * wout + tmpx;
            *outptr++ = *inptr0++;
            *outptr++ = *inptr0++;
            *outptr++ = *inptr0++;

            *outptr++ = *inptr1++;
            *outptr++ = *inptr1++;
            *outptr++ = *inptr1++;

            *outptr++ = *inptr2++;
            *outptr++ = *inptr2++;
            *outptr++ = *inptr2++;

            *outptr++ = *inptr3++;
            *outptr++ = *inptr3++;
            *outptr++ = *inptr3++;

            *outptr++ = *inptr4++;
            *outptr++ = *inptr4++;
            *outptr++ = *inptr4++;

            *outptr++ = *inptr5++;
            *outptr++ = *inptr5++;
            *outptr++ = *inptr5++;

            *outptr++ = *inptr6++;
            *outptr++ = *inptr6++;
            *outptr++ = *inptr6++;

            *outptr++ = *inptr7++;
            *outptr++ = *inptr7++;
            *outptr++ = *inptr7++;
        }

    }
    for (; i < h_in; i++){
        const unsigned char* inptr0 = src + i * win;
        for (int j = 0; j < w_in; j++){
            unsigned char* outptr0 = dst + (hout - j) * wout + i * 3;
            *outptr0++ = *inptr0++;
            *outptr0++ = *inptr0++;
            *outptr0++ = *inptr0++;
        }
    }
}
#endif
/*
bgr1 bgr2 bgr3
bgr4 bgr5 bgr6
bgr7 bgr8 bgr9
rotate:
bgr9 bgr8 bgr7
bgr6 bgr5 bgr4
bgr3 bgr2 bgr1
*/
//filp y
#ifdef __aarch64__
void rotate180_hwc(const unsigned char* src, unsigned char* dst, int w, int h_in){

    // uint8_t zerobuff[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    //                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int w_in = w * 3;
    uint8_t zerobuff[w_in];
    memset(zerobuff, 0, w_in * sizeof(unsigned char));
    int64_t stride_w = 24;
    // printf("src: %x, dst: %x \n", src, dst);
    for (int i = 0; i < h_in; i += 4){
        const unsigned char* inptr0 = src + i * w_in;
        const unsigned char* inptr1 = inptr0 + w_in;
        const unsigned char* inptr2 = inptr1 + w_in;
        const unsigned char* inptr3 = inptr2 + w_in;

        unsigned char* outptr0 = dst + (h_in - i) * w_in - stride_w;//last col
        unsigned char* outptr1 = outptr0 - w_in;
        unsigned char* outptr2 = outptr1 - w_in;
        unsigned char* outptr3 = outptr2 - w_in;

       // printf("outptr0: %x \n", outptr0);
        asm volatile(
        "prfm   pldl1keep, [%[ptr0]]                \n"
                "prfm   pldl1keep, [%[ptr1]]        \n"
                "prfm   pldl1keep, [%[ptr2]]        \n"
                "prfm   pldl1keep, [%[ptr3]]        \n"
        :
        :[ptr0] "r"(inptr0), [ptr1] "r"(inptr1), [ptr2] "r"(inptr2), [ptr3] "r"(inptr3)
        :"memory"
        );
        int j = 0;
        for (; j < w - 7; j += 8){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 3:
                        inptr0 = zerobuff;
                        outptr0 = zerobuff;
                    case 2:
                        inptr1 = zerobuff;
                        outptr1 = zerobuff;
                    case 1:
                        inptr2 = zerobuff;
                        outptr2 = zerobuff;
                    case 0:
                        inptr3 = zerobuff;
                        outptr3 = zerobuff;
                    default:
                        break;
                }
            }
            // uint8x8_t va = vld1_u8(inptr0);
            // uint8x8_t a = vrev64_u8(va);
            // printf("va: %d, %d, %d, %d, %d, %d, %d, %d\n", va[0], va[1], va[2], va[3],va[4],va[5],va[6],va[7]);
            // printf("a: %d, %d, %d, %d, %d, %d, %d, %d \n", a[0], a[1], a[2],a[3],a[4],a[5],a[6],a[7]);
            asm volatile (
                "ld3  {v0.8b, v1.8b, v2.8b}, [%[inptr0]], #24    \n" //v0={00,01,02, 03, 04, 05, 06, 07}"
                "ld3  {v3.8b, v4.8b, v5.8b}, [%[inptr1]], #24     \n" //v0={10,11,12, 13, 14, 15, 16, 17}"
                "ld3  {v6.8b, v7.8b, v8.8b}, [%[inptr2]], #24    \n" //v0={20,21,22, 23, 24, 25, 26, 27}"
                "ld3  {v9.8b, v10.8b, v11.8b}, [%[inptr3]], #24    \n" //v0={30,31,32, 33, 34, 35, 36, 37}"

                "rev64  v12.8b, v0.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v13.8b, v1.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v14.8b, v2.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r

                "rev64  v15.8b, v3.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v16.8b, v4.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v17.8b, v5.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "rev64  v18.8b, v6.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v19.8b, v7.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v20.8b, v8.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "rev64  v21.8b, v9.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v22.8b, v10.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v23.8b, v11.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "prfm   pldl1keep, [%[inptr0]]        \n"
                "prfm   pldl1keep, [%[inptr1]]        \n"
                "prfm   pldl1keep, [%[inptr2]]        \n"
                "prfm   pldl1keep, [%[inptr3]]        \n"

                "st3 {v12.8b, v13.8b, v14.8b}, [%[outptr0]]             \n" //00 10 20 30 04 14 24 34
                "st3 {v15.8b, v16.8b, v17.8b}, [%[outptr1]]              \n" //02 12 22 32
                "st3 {v18.8b, v19.8b, v20.8b}, [%[outptr2]]             \n" //01 11 21 31
                "st3 {v21.8b, v22.8b, v23.8b}, [%[outptr3]]              \n" //03 13 23 33

                "sub %[outptr0], %[outptr0], %[stride_w]       \n" //@ ptr - stride_w
                "sub %[outptr1], %[outptr1], %[stride_w]       \n"
                "sub %[outptr2], %[outptr2], %[stride_w]       \n"
                "sub %[outptr3], %[outptr3], %[stride_w]       \n"

            : [inptr0] "+r"(inptr0), [inptr1] "+r"(inptr1), [inptr2] "+r"(inptr2), [inptr3] "+r"(inptr3), \
              [outptr0] "+r"(outptr0), [outptr1] "+r"(outptr1), [outptr2] "+r"(outptr2), [outptr3] "+r"(outptr3), \
              [stride_w] "+r" (stride_w)
            :
            : "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", \
              "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23"
            );
           // printf("outptr0: %x \n", outptr0);

           // printf("outptr0: %d, %d, %d, %d \n", outptr0[0], outptr0[1], outptr0[2], outptr0[3]);
        }
        outptr3 += stride_w - 3;
        outptr2 += stride_w - 3;
        outptr1 += stride_w - 3;
        outptr0 += stride_w - 3;
        // printf("outptr0: %x, outptr1: %x, outptr2: %x, outptr3: %x \n", outptr0, outptr1, outptr2, outptr3);
        // printf("inptr0: %x, inptr1: %x, inptr2: %x, inptr3: %x \n", inptr0, inptr1, inptr2, inptr3);
        for (; j < w; j++){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 0:
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        outptr2 -= 6;
                    case 1:
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        outptr1 -= 6;
                    case 2:
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        outptr0 -= 6;
                    case 3:
                        //inptr3 = zerobuff;
                    default:
                        break;
                }
            }else{
                // printf("j: %d \n", j);
                // printf("outptr0: %x, outptr1: %x, outptr2: %x, outptr3: %x \n", outptr0, outptr1, outptr2, outptr3);
                // printf("inptr0: %x, inptr1: %x, inptr2: %x, inptr3: %x \n", inptr0, inptr1, inptr2, inptr3);
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;
                outptr3 -= 6;

                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                outptr2 -= 6;

                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                outptr1 -= 6;

                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                outptr0 -= 6;
            }
        }
    }
}
#else
void rotate180_hwc(const unsigned char* src, unsigned char* dst, int w, int h_in){

    int w_in = w * 3;
    uint8_t zerobuff[w_in];
    memset(zerobuff, 0, w_in * sizeof(unsigned char));
    int stride_w = 24;
    //4*8
    //printf("dst: %x \n", dst);
    for (int i = 0; i < h_in; i += 4){
        const unsigned char* inptr0 = src + i * w_in;
        const unsigned char* inptr1 = inptr0 + w_in;
        const unsigned char* inptr2 = inptr1 + w_in;
        const unsigned char* inptr3 = inptr2 + w_in;

        unsigned char* outptr0 = dst + (h_in - i) * w_in - stride_w;//last
        unsigned char* outptr1 = outptr0 - w_in;
        unsigned char* outptr2 = outptr1 - w_in;
        unsigned char* outptr3 = outptr2 - w_in;
        //printf("outptr0: %x \n", outptr0);
        asm volatile(
        "pld [%[ptr0]]                         @ preload a, 64byte\n"
                "pld [%[ptr1]]            @ preload a, 64byte\n"
                "pld [%[ptr2]]            @ preload a, 64byte\n"
                "pld [%[ptr3]]            @ preload a, 64byte\n"
        :
        :[ptr0] "r"(inptr0), [ptr1] "r"(inptr1), [ptr2] "r"(inptr2), [ptr3] "r"(inptr3)
        :"memory"
        );
        int j = 0;
        for (; j < w - 7; j += 8){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 3:
                        inptr0 = zerobuff;
                        outptr0 = zerobuff;
                    case 2:
                        inptr1 = zerobuff;
                        outptr1 = zerobuff;
                    case 1:
                        inptr2 = zerobuff;
                        outptr2 = zerobuff;
                    case 0:
                        inptr3 = zerobuff;
                        outptr3 = zerobuff;
                    default:
                        break;
                }
            }
            // uint8x8_t va = vld1_u8(inptr0);
            // uint8x8_t a = vrev64_u8(va);
            // printf("va: %d, %d, %d, %d, %d, %d, %d, %d\n", va[0], va[1], va[2], va[3],va[4],va[5],va[6],va[7]);
            // printf("a: %d, %d, %d, %d, %d, %d, %d, %d \n", a[0], a[1], a[2],a[3],a[4],a[5],a[6],a[7]);
            asm volatile (
                "vld3.8  {d0, d1, d2}, [%[inptr0]]!   @ zip load r0, d0 =00 01 02 03 04 05 06 07\n"
                "vld3.8  {d3, d4, d5}, [%[inptr1]]!   @ zip load r1, d2 =10 11 12 13 14 15 16 17\n"
                "vld3.8  {d6, d7, d8}, [%[inptr2]]!   @ zip load r1, d4 =20 21 22 23 24 25 26 27\n"
                "vld3.8  {d9, d10, d11}, [%[inptr3]]!   @ zip load r1, d6 = 30 31 32 33 34 35 36 37\n"

                "vrev64.8  d12, d0               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d13, d1               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d14, d2               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d15, d3               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d16, d4               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d17, d5               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d18, d6               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d19, d7               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d20, d8               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d21, d9               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d22, d10               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d23, d11               @ reverse 07 06 05 04 03 02 01 00 \n"

                "pld [%[inptr0]]                         @ preload a, 64byte\n"
                "pld [%[inptr1]]                         @ preload a, 64byte\n"
                "pld [%[inptr2]]                         @ preload a, 64byte\n"
                "pld [%[inptr3]]                         @ preload a, 64byte\n"

                "vst3.8  {d12, d13, d14},    [%[outptr0]]   @ write d0(q0,low),r00,r10 20 30\n"
                "vst3.8  {d15, d16, d17},    [%[outptr1]]   @ write d4(q0,low),r01,r11 21 31\n"
                "vst3.8  {d18, d19, d20},    [%[outptr2]]   @ write d4(q0,low),r01,r11 21 31\n"
                "vst3.8  {d21, d22, d23},    [%[outptr3]]   @ write d4(q0,low),r01,r11 21 31\n"

                "sub %[outptr0], %[stride_w]       @ ptr - stride_w \n"
                "sub %[outptr1], %[stride_w]       @ ptr - stride_w \n"
                "sub %[outptr2], %[stride_w]       @ ptr - stride_w \n"
                "sub %[outptr3], %[stride_w]       @ ptr - stride_w \n"

            : [inptr0] "+r"(inptr0), [inptr1] "+r"(inptr1), [inptr2] "+r"(inptr2), [inptr3] "+r"(inptr3), \
              [outptr0] "+r"(outptr0), [outptr1] "+r"(outptr1), [outptr2] "+r"(outptr2), [outptr3] "+r"(outptr3), \
              [stride_w] "+r" (stride_w)
            :
            : "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12"
            );
           // printf("outptr0: %x \n", outptr0);

           // printf("outptr0: %d, %d, %d, %d \n", outptr0[0], outptr0[1], outptr0[2], outptr0[3]);
        }
        outptr3 += stride_w - 3;
        outptr2 += stride_w - 3;
        outptr1 += stride_w - 3;
        outptr0 += stride_w - 3;
        for (; j < w; j++){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 0:
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        outptr2 -= 6;
                    case 1:
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        outptr1 -= 6;
                    case 2:
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        outptr0 -= 6;
                    case 3:
                        //inptr3 = zerobuff;
                    default:
                        break;
                }
            }else{
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;
                outptr3 -= 6;

                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                outptr2 -= 6;

                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                outptr1 -= 6;

                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                outptr0 -= 6;
            }
        }
    }
}
#endif
#elif __linux__ || __APPLE__
#pragma message("x86 bgr rotate90/270/180_hwc")
void rotate90_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    // TODO support
}

void rotate270_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    // TODO support
}

void rotate180_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    // TODO support
}
#endif

// bgr2rgb, rgb2bgr
void rgb_to_bgr_convert(unsigned char* rgb_data, int width, int height, unsigned char* bgr_data){
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            *bgr_data++ = rgb_data[2];  // r
            *bgr_data++ = rgb_data[1];  // g
            *bgr_data++ = rgb_data[0];  // b
            rgb_data += 3;
        }
    }
}
}