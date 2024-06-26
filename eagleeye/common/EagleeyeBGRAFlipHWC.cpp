
#include "eagleeye/common/EagleeyeBGRARotateHWC.h"
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#include <arm_neon.h>
#endif


namespace eagleeye{

void flip_x_hwc_bgra(const unsigned char* src, unsigned char* dst, int w_in, int h_in);

void flip_y_hwc_bgra(const unsigned char* src, unsigned char* dst, int w_in, int h_in);

void flip_xy_hwc_bgra(const unsigned char* src, unsigned char* dst, int w_in, int h_in);

//x: flip_num = 1 y: flip_num = -1 xy: flip_num = 0;
void bgra_flip_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in, int flip_num){
    if (flip_num == 1){//x
        flip_x_hwc_bgra(src, dst, w_in, h_in);
    }
    if (flip_num == -1){//y
        flip_y_hwc_bgra(src, dst, w_in, h_in);
    }
    if (flip_num == 0){//xy
        flip_xy_hwc_bgra(src, dst, w_in, h_in);
    }

}

#if defined (__ARM_NEON) || defined (__ARM_NEON__)
/*
bgr1 bgr2 bgr3
bgr4 bgr5 bgr6
bgr7 bgr8 bgr9
rotate:
bgr7 bgr8 bgr9
bgr4 bgr5 bgr6
bgr1 bgr2 bgr3
*/
#ifdef __aarch64__
void flip_x_hwc_bgra(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    // printf("flip_x_hwc \n");
    int h = h_in - 1;
    int win = w_in * 4;
    uint8_t zerobuff[win];
    memset(zerobuff, 0, win * sizeof(unsigned char));
    uint8_t zerobuff2[win];
    memset(zerobuff2, 0, win * sizeof(unsigned char));
    for (int i = 0; i < h_in; i += 4){
        const unsigned char* inptr0 = src + i * win;
        const unsigned char* inptr1 = inptr0 + win;
        const unsigned char* inptr2 = inptr1 + win;
        const unsigned char* inptr3 = inptr2 + win;

        unsigned char* outptr0 = dst + (h - i) * win;//last
        unsigned char* outptr1 = outptr0 - win;
        unsigned char* outptr2 = outptr1 - win;
        unsigned char* outptr3 = outptr2 - win;

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
        if (i + 3 >= h_in){
            switch ((i + 3) - h_in){
                case 3:
                    inptr0 = zerobuff;
                    outptr0 = zerobuff2;
                case 2:
                    inptr1 = zerobuff;
                    outptr1 = zerobuff2;
                case 1:
                    inptr2 = zerobuff;
                    outptr2 = zerobuff2;
                case 0:
                    inptr3 = zerobuff;
                    outptr3 = zerobuff2;
                default:
                    break;
            }
        }
        int j = 0;
        for (; j < w_in - 7; j += 8){
            asm volatile (
                "ld4  {v0.8b, v1.8b, v2.8b, v3.8b}, [%[inptr0]], #32    \n" //v0={00,01,02, 03, 04, 05, 06, 07}"
                "ld4  {v4.8b, v5.8b, v6.8b, v7.8b}, [%[inptr1]], #32     \n" //v0={10,11,12, 13, 14, 15, 16, 17}"
                "ld4  {v8.8b, v9.8b, v10.8b, v11.8b}, [%[inptr2]], #32    \n" //v0={20,21,22, 23, 24, 25, 26, 27}"
                "ld4  {v12.8b, v13.8b, v14.8b, v15.8b}, [%[inptr3]], #32    \n" //v0={30,31,32, 33, 34, 35, 36, 37}"

                "prfm   pldl1keep, [%[inptr0]]        \n"
                "prfm   pldl1keep, [%[inptr1]]        \n"
                "prfm   pldl1keep, [%[inptr2]]        \n"
                "prfm   pldl1keep, [%[inptr3]]        \n"

                "st4 {v0.8b, v1.8b, v2.8b, v3.8b}, [%[outptr0]], #32  \n" //00 10 20 30 04 14 24 34
                "st4 {v4.8b, v5.8b, v6.8b, v7.8b}, [%[outptr1]], #32            \n" //02 12 22 32
                "st4 {v8.8b, v9.8b, v10.8b, v11.8b}, [%[outptr2]], #32             \n" //01 11 21 31
                "st4 {v12.8b, v13.8b, v14.8b, v15.8b}, [%[outptr3]], #32              \n" //03 13 23 33

            : [inptr0] "+r"(inptr0), [inptr1] "+r"(inptr1), [inptr2] "+r"(inptr2), [inptr3] "+r"(inptr3), \
              [outptr0] "+r"(outptr0), [outptr1] "+r"(outptr1), [outptr2] "+r"(outptr2), [outptr3] "+r"(outptr3)
            :
            : "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15"
            );
           // printf("outptr0: %x \n", outptr0);

           // printf("outptr0: %d, %d, %d, %d \n", outptr0[0], outptr0[1], outptr0[2], outptr0[3]);
        }
        for (; j < w_in; j++){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 0:
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                    case 1:
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                    case 2:
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                    case 3:
                        //inptr3 = zerobuff;
                    default:
                        break;
                }
            }else{
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;

                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;

                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;

                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
            }
        }
    }
}
#else
void flip_x_hwc_bgra(const unsigned char* src, unsigned char* dst, int w_in, int h_in){
    // uint8_t zerobuff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int win = w_in * 4;
    uint8_t zerobuff[win];
    memset(zerobuff, 0, win * sizeof(unsigned char));
    uint8_t zerobuff2[win];
    memset(zerobuff2, 0, win * sizeof(unsigned char));
    int h = h_in - 1;
    //4*8
    //printf("dst: %x \n", dst);
    for (int i = 0; i < h_in; i += 4){
        const unsigned char* inptr0 = src + i * win;
        const unsigned char* inptr1 = inptr0 + win;
        const unsigned char* inptr2 = inptr1 + win;
        const unsigned char* inptr3 = inptr2 + win;

        unsigned char* outptr0 = dst + (h - i) * win;//last
        unsigned char* outptr1 = outptr0 - win;
        unsigned char* outptr2 = outptr1 - win;
        unsigned char* outptr3 = outptr2 - win;
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
        if (i + 3 >= h_in){
            switch ((i + 3) - h_in){
                case 3:
                    inptr0 = zerobuff;
                    outptr0 = zerobuff2;
                case 2:
                    inptr1 = zerobuff;
                    outptr1 = zerobuff2;
                case 1:
                    inptr2 = zerobuff;
                    outptr2 = zerobuff2;
                case 0:
                    inptr3 = zerobuff;
                    outptr3 = zerobuff2;
                default:
                    break;
            }
        }
        int j = 0;
        for (; j < w_in - 7; j += 8){
            asm volatile (
                "vld4.8  {d0, d1, d2, d3}, [%[inptr0]]!   @ zip load r0, d0 =00 01 02 03 04 05 06 07\n"
                "vld4.8  {d4, d5, d6, d7}, [%[inptr1]]!   @ zip load r1, d2 =10 11 12 13 14 15 16 17\n"
                "vld4.8  {d8, d9, d10, d11}, [%[inptr2]]!   @ zip load r1, d4 =20 21 22 23 24 25 26 27\n"
                "vld4.8  {d12, d13, d14, d15}, [%[inptr3]]!   @ zip load r1, d6 = 30 31 32 33 34 35 36 37\n"

                "pld [%[inptr0]]                         @ preload a, 64byte\n"
                "pld [%[inptr1]]                         @ preload a, 64byte\n"
                "pld [%[inptr2]]                         @ preload a, 64byte\n"
                "pld [%[inptr3]]                         @ preload a, 64byte\n"

                "vst4.8  {d0, d1, d2, d3},    [%[outptr0]]!   @ write d0(q0,low),r00,r10 20 30\n"
                "vst4.8  {d4, d5, d6, d7},    [%[outptr1]]!   @ write d4(q0,low),r01,r11 21 31\n"
                "vst4.8  {d8, d9, d10, d11},    [%[outptr2]]!   @ write d4(q0,low),r01,r11 21 31\n"
                "vst4.8  {d12, d13, d14, d15},    [%[outptr3]]!   @ write d4(q0,low),r01,r11 21 31\n"

            : [inptr0] "+r"(inptr0), [inptr1] "+r"(inptr1), [inptr2] "+r"(inptr2), [inptr3] "+r"(inptr3), \
              [outptr0] "+r"(outptr0), [outptr1] "+r"(outptr1), [outptr2] "+r"(outptr2), [outptr3] "+r"(outptr3)
            :
            : "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7"
            );
        }
        for (; j < w_in; j++){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 0:
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                    case 1:
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                    case 2:
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                    case 3:
                        //inptr3 = zerobuff;
                    default:
                        break;
                }
            }else{
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;
                *outptr3++ = *inptr3++;

                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;

                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;

                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
            }
        }
    }
}
#endif
/*
bgr1 bgr2 bgr3
bgr4 bgr5 bgr6
bgr7 bgr8 bgr9
flip:
bgr3 bgr2 bgr1
bgr6 bgr5 bgr4
bgr9 bgr8 bgr7
*/
#ifdef __aarch64__
void flip_y_hwc_bgra(const unsigned char* src, unsigned char* dst, int w, int h_in){
    // uint8_t zerobuff[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    //                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int w_in = w * 4;
    uint8_t zerobuff[w_in];
    memset(zerobuff, 0, w_in * sizeof(unsigned char));
    uint8_t zerobuff2[w_in];
    memset(zerobuff2, 0, w_in * sizeof(unsigned char));
    int64_t stride_w = 32;
    // printf("src: %x, dst: %x \n", src, dst);
    for (int i = 0; i < h_in; i += 4){
        const unsigned char* inptr0 = src + i * w_in;
        const unsigned char* inptr1 = inptr0 + w_in;
        const unsigned char* inptr2 = inptr1 + w_in;
        const unsigned char* inptr3 = inptr2 + w_in;

        unsigned char* outptr0 = dst + (i + 1) * w_in - stride_w;//last col
        unsigned char* outptr1 = outptr0 + w_in;
        unsigned char* outptr2 = outptr1 + w_in;
        unsigned char* outptr3 = outptr2 + w_in;

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
        if (i + 3 >= h_in){
            switch ((i + 3) - h_in){
                case 3:
                    inptr0 = zerobuff;
                    outptr0 = zerobuff2;
                case 2:
                    inptr1 = zerobuff;
                    outptr1 = zerobuff2;
                case 1:
                    inptr2 = zerobuff;
                    outptr2 = zerobuff2;
                case 0:
                    inptr3 = zerobuff;
                    outptr3 = zerobuff2;
                default:
                    break;
            }
        }
        int j = 0;
        for (; j < w - 7; j += 8){
            // if (i + 3 >= h_in){
            //     switch ((i + 3) - h_in){
            //         case 3:
            //             inptr0 = zerobuff;
            //             outptr0 = zerobuff;
            //         case 2:
            //             inptr1 = zerobuff;
            //             outptr1 = zerobuff;
            //         case 1:
            //             inptr2 = zerobuff;
            //             outptr2 = zerobuff;
            //         case 0:
            //             inptr3 = zerobuff;
            //             outptr3 = zerobuff;
            //         default:
            //             break;
            //     }
            // }
            // uint8x8_t va = vld1_u8(inptr0);
            // uint8x8_t a = vrev64_u8(va);
            // printf("va: %d, %d, %d, %d, %d, %d, %d, %d\n", va[0], va[1], va[2], va[3],va[4],va[5],va[6],va[7]);
            // printf("a: %d, %d, %d, %d, %d, %d, %d, %d \n", a[0], a[1], a[2],a[3],a[4],a[5],a[6],a[7]);
            asm volatile (
                "ld4  {v0.8b, v1.8b, v2.8b, v3.8b}, [%[inptr0]], #32    \n" //v0={00,01,02, 03, 04, 05, 06, 07}"
                "ld4  {v4.8b, v5.8b, v6.8b, v7.8b}, [%[inptr1]], #32     \n" //v0={10,11,12, 13, 14, 15, 16, 17}"
                "ld4  {v8.8b, v9.8b, v10.8b, v11.8b}, [%[inptr2]], #32    \n" //v0={20,21,22, 23, 24, 25, 26, 27}"
                "ld4  {v12.8b, v13.8b, v14.8b, v15.8b}, [%[inptr3]], #32    \n" //v0={30,31,32, 33, 34, 35, 36, 37}"

                "rev64  v16.8b, v0.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v17.8b, v1.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v18.8b, v2.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r
                "rev64  v19.8b, v3.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "rev64  v20.8b, v4.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v21.8b, v5.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v22.8b, v6.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v23.8b, v7.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "rev64  v0.8b, v8.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v1.8b, v9.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v2.8b, v10.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v3.8b, v11.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "rev64  v4.8b, v12.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v5.8b, v13.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v6.8b, v14.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v7.8b, v15.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "prfm   pldl1keep, [%[inptr0]]        \n"
                "prfm   pldl1keep, [%[inptr1]]        \n"
                "prfm   pldl1keep, [%[inptr2]]        \n"
                "prfm   pldl1keep, [%[inptr3]]        \n"

                "st4 {v16.8b, v17.8b, v18.8b, v19.8b}, [%[outptr0]]             \n" //00 10 20 30 04 14 24 34
                "st4 {v20.8b, v21.8b, v22.8b, v23.8b}, [%[outptr1]]              \n" //02 12 22 32
                "st4 {v0.8b, v1.8b, v2.8b, v3.8b}, [%[outptr2]]             \n" //01 11 21 31
                "st4 {v4.8b, v5.8b, v6.8b, v7.8b}, [%[outptr3]]              \n" //03 13 23 33

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
        outptr3 += stride_w - 4;
        outptr2 += stride_w - 4;
        outptr1 += stride_w - 4;
        outptr0 += stride_w - 4;
        // printf("outptr0: %x, outptr1: %x, outptr2: %x, outptr3: %x \n", outptr0, outptr1, outptr2, outptr3);
        // printf("inptr0: %x, inptr1: %x, inptr2: %x, inptr3: %x \n", inptr0, inptr1, inptr2, inptr3);
        for (; j < w; j++){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 0:
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        outptr2 -= 8;
                    case 1:
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        outptr1 -= 8;
                    case 2:
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        outptr0 -= 8;
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
                *outptr3++ = *inptr3++;
                outptr3 -= 8;

                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                outptr2 -= 8;

                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                outptr1 -= 8;

                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                outptr0 -= 8;
            }
        }
    }
}
#else
void flip_y_hwc_bgra(const unsigned char* src, unsigned char* dst, int w, int h_in){
    int w_in = w * 4;
    uint8_t zerobuff[w_in];
    memset(zerobuff, 0, w_in * sizeof(unsigned char));
    uint8_t zerobuff2[w_in];
    memset(zerobuff2, 0, w_in * sizeof(unsigned char));
    int stride_w = 32;
    //4*8
    //printf("dst: %x \n", dst);
    for (int i = 0; i < h_in; i += 4){
        const unsigned char* inptr0 = src + i * w_in;
        const unsigned char* inptr1 = inptr0 + w_in;
        const unsigned char* inptr2 = inptr1 + w_in;
        const unsigned char* inptr3 = inptr2 + w_in;

        unsigned char* outptr0 = dst + (i + 1) * w_in - stride_w;//last
        unsigned char* outptr1 = outptr0 + w_in;
        unsigned char* outptr2 = outptr1 + w_in;
        unsigned char* outptr3 = outptr2 + w_in;
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
        if (i + 3 >= h_in){
            switch ((i + 3) - h_in){
                case 3:
                    inptr0 = zerobuff;
                    outptr0 = zerobuff2;
                case 2:
                    inptr1 = zerobuff;
                    outptr1 = zerobuff2;
                case 1:
                    inptr2 = zerobuff;
                    outptr2 = zerobuff2;
                case 0:
                    inptr3 = zerobuff;
                    outptr3 = zerobuff2;
                default:
                    break;
            }
        }
        for (; j < w - 7; j += 8){
            asm volatile (
                "vld4.8  {d0, d1, d2, d3}, [%[inptr0]]!   @ zip load r0, d0 =00 01 02 03 04 05 06 07\n"
                "vld4.8  {d4, d5, d6, d7}, [%[inptr1]]!   @ zip load r1, d2 =10 11 12 13 14 15 16 17\n"
                "vld4.8  {d8, d9, d10, d11}, [%[inptr2]]!   @ zip load r1, d4 =20 21 22 23 24 25 26 27\n"
                "vld4.8  {d12, d13, d14, d15}, [%[inptr3]]!   @ zip load r1, d6 = 30 31 32 33 34 35 36 37\n"

                "vrev64.8  d16, d0               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d17, d1               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d18, d2               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d19, d3               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d20, d4               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d21, d5               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d22, d6               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d23, d7               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d0, d8               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d1, d9               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d2, d10               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d3, d11               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d4, d12               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d5, d13               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d6, d14               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d7, d15               @ reverse 07 06 05 04 03 02 01 00 \n"

                "pld [%[inptr0]]                         @ preload a, 64byte\n"
                "pld [%[inptr1]]                         @ preload a, 64byte\n"
                "pld [%[inptr2]]                         @ preload a, 64byte\n"
                "pld [%[inptr3]]                         @ preload a, 64byte\n"

                "vst4.8  {d16, d17, d18, d19},    [%[outptr0]]   @ write d0(q0,low),r00,r10 20 30\n"
                "vst4.8  {d20, d21, d22, d23},    [%[outptr1]]   @ write d4(q0,low),r01,r11 21 31\n"
                "vst4.8  {d0, d1, d2, d3},    [%[outptr2]]   @ write d4(q0,low),r01,r11 21 31\n"
                "vst4.8  {d4, d5, d6, d7},    [%[outptr3]]   @ write d4(q0,low),r01,r11 21 31\n"

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
        outptr3 += stride_w - 4;
        outptr2 += stride_w - 4;
        outptr1 += stride_w - 4;
        outptr0 += stride_w - 4;
        for (; j < w; j++){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 0:
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        outptr2 -= 8;
                    case 1:
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        outptr1 -= 8;
                    case 2:
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        outptr0 -= 8;
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
                *outptr3++ = *inptr3++;
                outptr3 -= 8;

                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                outptr2 -= 8;

                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                outptr1 -= 8;

                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                outptr0 -= 8;
            }
        }
    }
}
#endif
/*
bgr1 bgr2 bgr3
bgr4 bgr5 bgr6
bgr7 bgr8 bgr9
flip:
bgr9 bgr8 bgr7
bgr6 bgr5 bgr4
bgr3 bgr2 bgr1
*/
#ifdef __aarch64__
void flip_xy_hwc_bgra(const unsigned char* src, unsigned char* dst, int w, int h_in){
    int64_t stride_w = 32;
    int w_in = w * 4;
    uint8_t zerobuff[w_in];
    memset(zerobuff, 0, w_in * sizeof(unsigned char));
    uint8_t zerobuff2[w_in];
    memset(zerobuff2, 0, w_in * sizeof(unsigned char));
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
        :[ptr0] "r" (inptr0), [ptr1] "r"(inptr1), [ptr2] "r"(inptr2), [ptr3] "r"(inptr3)
        :"memory"
        );
        if (i + 3 >= h_in){
            switch ((i + 3) - h_in){
                case 3:
                    inptr0 = zerobuff;
                    outptr0 = zerobuff2;
                case 2:
                    inptr1 = zerobuff;
                    outptr1 = zerobuff2;
                case 1:
                    inptr2 = zerobuff;
                    outptr2 = zerobuff2;
                case 0:
                    inptr3 = zerobuff;
                    outptr3 = zerobuff2;
                default:
                    break;
            }
        }
        int j = 0;
        for (; j < w - 7; j += 8){
            // if (i + 3 >= h_in){
            //     switch ((i + 3) - h_in){
            //         case 3:
            //             inptr0 = zerobuff;
            //             outptr0 = zerobuff;
            //         case 2:
            //             inptr1 = zerobuff;
            //             outptr1 = zerobuff;
            //         case 1:
            //             inptr2 = zerobuff;
            //             outptr2 = zerobuff;
            //         case 0:
            //             inptr3 = zerobuff;
            //             outptr3 = zerobuff;
            //         default:
            //             break;
            //     }
            // }
            // uint8x8_t va = vld1_u8(inptr0);
            // uint8x8_t a = vrev64_u8(va);
            // printf("va: %d, %d, %d, %d, %d, %d, %d, %d\n", va[0], va[1], va[2], va[3],va[4],va[5],va[6],va[7]);
            // printf("a: %d, %d, %d, %d, %d, %d, %d, %d \n", a[0], a[1], a[2],a[3],a[4],a[5],a[6],a[7]);
            asm volatile (
                "ld4  {v0.8b, v1.8b, v2.8b, v3.8b}, [%[inptr0]], #32    \n" //v0={00,01,02, 03, 04, 05, 06, 07}"
                "ld4  {v4.8b, v5.8b, v6.8b, v7.8b}, [%[inptr1]], #32     \n" //v0={10,11,12, 13, 14, 15, 16, 17}"
                "ld4  {v8.8b, v9.8b, v10.8b, v11.8b}, [%[inptr2]], #32    \n" //v0={20,21,22, 23, 24, 25, 26, 27}"
                "ld4  {v12.8b, v13.8b, v14.8b, v15.8b}, [%[inptr3]], #32    \n" //v0={30,31,32, 33, 34, 35, 36, 37}"

                "rev64  v16.8b, v0.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 b
                "rev64  v17.8b, v1.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 g
                "rev64  v18.8b, v2.8b                \n" //@ reverse 07 06 05 04 03 02 01 00 r
                "rev64  v19.8b, v3.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "rev64  v20.8b, v4.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v21.8b, v5.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v22.8b, v6.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v23.8b, v7.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "rev64  v0.8b, v8.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v1.8b, v9.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v2.8b, v10.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v3.8b, v11.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "rev64  v4.8b, v12.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v5.8b, v13.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v6.8b, v14.8b                \n" //@ reverse 07 06 05 04 03 02 01 00
                "rev64  v7.8b, v15.8b                \n" //@ reverse 07 06 05 04 03 02 01 00

                "prfm   pldl1keep, [%[inptr0]]        \n"
                "prfm   pldl1keep, [%[inptr1]]        \n"
                "prfm   pldl1keep, [%[inptr2]]        \n"
                "prfm   pldl1keep, [%[inptr3]]        \n"

                "st4 {v16.8b, v17.8b, v18.8b, v19.8b}, [%[outptr0]]             \n" //00 10 20 30 04 14 24 34
                "st4 {v20.8b, v21.8b, v22.8b, v23.8b}, [%[outptr1]]              \n" //02 12 22 32
                "st4 {v0.8b, v1.8b, v2.8b, v3.8b}, [%[outptr2]]             \n" //01 11 21 31
                "st4 {v4.8b, v5.8b, v6.8b, v7.8b}, [%[outptr3]]              \n" //03 13 23 33

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
        outptr3 += stride_w - 4;
        outptr2 += stride_w - 4;
        outptr1 += stride_w - 4;
        outptr0 += stride_w - 4;
        // printf("outptr0: %x, outptr1: %x, outptr2: %x, outptr3: %x \n", outptr0, outptr1, outptr2, outptr3);
        // printf("inptr0: %x, inptr1: %x, inptr2: %x, inptr3: %x \n", inptr0, inptr1, inptr2, inptr3);
        for (; j < w; j++){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 0:
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        outptr2 -= 8;
                    case 1:
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        outptr1 -= 8;
                    case 2:
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        outptr0 -= 8;
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
                *outptr3++ = *inptr3++;
                outptr3 -= 8;

                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                outptr2 -= 8;

                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                outptr1 -= 8;

                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                outptr0 -= 8;
            }
        }
    }
}
#else
void flip_xy_hwc_bgra(const unsigned char* src, unsigned char* dst, int w, int h_in){
    int w_in = w * 4;
    uint8_t zerobuff[w_in];
    memset(zerobuff, 0, w_in * sizeof(unsigned char));
    uint8_t zerobuff2[w_in];
    memset(zerobuff2, 0, w_in * sizeof(unsigned char));
    int stride_w = 32;
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
        if (i + 3 >= h_in){
            switch ((i + 3) - h_in){
                case 3:
                    inptr0 = zerobuff;
                    outptr0 = zerobuff2;
                case 2:
                    inptr1 = zerobuff;
                    outptr1 = zerobuff2;
                case 1:
                    inptr2 = zerobuff;
                    outptr2 = zerobuff2;
                case 0:
                    inptr3 = zerobuff;
                    outptr3 = zerobuff2;
                default:
                    break;
            }
        }
        int j = 0;
        for (; j < w - 7; j += 8){
            // if (i + 3 >= h_in){
            //     switch ((i + 3) - h_in){
            //         case 3:
            //             inptr0 = zerobuff;
            //             outptr0 = zerobuff;
            //         case 2:
            //             inptr1 = zerobuff;
            //             outptr1 = zerobuff;
            //         case 1:
            //             inptr2 = zerobuff;
            //             outptr2 = zerobuff;
            //         case 0:
            //             inptr3 = zerobuff;
            //             outptr3 = zerobuff;
            //         default:
            //             break;
            //     }
            // }
            // uint8x8_t va = vld1_u8(inptr0);
            // uint8x8_t a = vrev64_u8(va);
            // printf("va: %d, %d, %d, %d, %d, %d, %d, %d\n", va[0], va[1], va[2], va[3],va[4],va[5],va[6],va[7]);
            // printf("a: %d, %d, %d, %d, %d, %d, %d, %d \n", a[0], a[1], a[2],a[3],a[4],a[5],a[6],a[7]);
            asm volatile (
                "vld4.8  {d0, d1, d2, d3}, [%[inptr0]]!   @ zip load r0, d0 =00 01 02 03 04 05 06 07\n"
                "vld4.8  {d4, d5, d6, d7}, [%[inptr1]]!   @ zip load r1, d2 =10 11 12 13 14 15 16 17\n"
                "vld4.8  {d8, d9, d10, d11}, [%[inptr2]]!   @ zip load r1, d4 =20 21 22 23 24 25 26 27\n"
                "vld4.8  {d12, d13, d14, d15}, [%[inptr3]]!   @ zip load r1, d6 = 30 31 32 33 34 35 36 37\n"

                "vrev64.8  d16, d0               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d17, d1               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d18, d2               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d19, d3               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d20, d4               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d21, d5               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d22, d6               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d23, d7               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d0, d8               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d1, d9               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d2, d10               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d3, d11               @ reverse 07 06 05 04 03 02 01 00 \n"

                "vrev64.8  d4, d12               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d5, d13               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d6, d14               @ reverse 07 06 05 04 03 02 01 00 \n"
                "vrev64.8  d7, d15               @ reverse 07 06 05 04 03 02 01 00 \n"

                "pld [%[inptr0]]                         @ preload a, 64byte\n"
                "pld [%[inptr1]]                         @ preload a, 64byte\n"
                "pld [%[inptr2]]                         @ preload a, 64byte\n"
                "pld [%[inptr3]]                         @ preload a, 64byte\n"

                "vst4.8  {d16, d17, d18, d19},    [%[outptr0]]   @ write d0(q0,low),r00,r10 20 30\n"
                "vst4.8  {d20, d21, d22, d23},    [%[outptr1]]   @ write d4(q0,low),r01,r11 21 31\n"
                "vst4.8  {d0, d1, d2, d3},    [%[outptr2]]   @ write d4(q0,low),r01,r11 21 31\n"
                "vst4.8  {d4, d5, d6, d7},    [%[outptr3]]   @ write d4(q0,low),r01,r11 21 31\n"

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
        outptr3 += stride_w - 4;
        outptr2 += stride_w - 4;
        outptr1 += stride_w - 4;
        outptr0 += stride_w - 4;
        // printf("outptr0: %x, outptr1: %x, outptr2: %x, outptr3: %x \n", outptr0, outptr1, outptr2, outptr3);
        // printf("inptr0: %x, inptr1: %x, inptr2: %x, inptr3: %x \n", inptr0, inptr1, inptr2, inptr3);
        for (; j < w; j++){
            if (i + 3 >= h_in){
                switch ((i + 3) - h_in){
                    case 0:
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        *outptr2++ = *inptr2++;
                        outptr2 -= 8;
                    case 1:
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        *outptr1++ = *inptr1++;
                        outptr1 -= 8;
                    case 2:
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        *outptr0++ = *inptr0++;
                        outptr0 -= 8;
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
                *outptr3++ = *inptr3++;
                outptr3 -= 8;

                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                *outptr2++ = *inptr2++;
                outptr2 -= 8;

                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                *outptr1++ = *inptr1++;
                outptr1 -= 8;

                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                *outptr0++ = *inptr0++;
                outptr0 -= 8;
            }
        }
    }
}
#endif
#elif __linux__ || __APPLE__
#pragma message("x86 bgra flip_x/y/xy_hwc_bgra")
void flip_x_hwc_bgra(const unsigned char* src, unsigned char* dst, int w_in, int h_in){

}

void flip_y_hwc_bgra(const unsigned char* src, unsigned char* dst, int w_in, int h_in){

}

void flip_xy_hwc_bgra(const unsigned char* src, unsigned char* dst, int w_in, int h_in){

}

#endif
} //namespace easy_mobile_util
