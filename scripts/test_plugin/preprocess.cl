#define BLOCK_SIZE 32
#define LOCAL_MEMORY_SIZE 33
#define LOCAL_MEMORY_TOTAL 33*33*3
__kernel void preprocess_kernel(__read_only image2d_t input,
                                __write_only float* output, 
                                __private const float height_scale,
                                __private const float width_scale,
                                __private const int in_height,
                                __private const int in_width,
                                __private const int out_height,
                                __private const int out_width){
    int y = get_global_id(0);
    int x = get_global_id(1);

    if(y >= out_height || x >= out_width){
        return;
    }

    const float h_in = y * height_scale;
    const float w_in = x * width_scale;
    const int h_lower = max(0, (int) floor(h_in));
    const int h_upper = min(in_height - 1, h_lower + 1);
    const int w_lower = max(0, (int) floor(w_in));
    const int w_upper = min(in_width - 1, w_lower + 1);

    const float h_lerp = h_in - h_lower;
    const float w_lerp = w_in - w_lower;

    const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE|CLK_ADDRESS_CLAMP_TO_EDGE|CLK_FILTER_LINEAR;

    uchar4 top_left = read_imageui(input, smp,
            (int2)(w_lower, h_lower));
    uchar4 top_right = read_imageui(input, smp,
            (int2)(w_upper, h_lower));
    uchar4 bottom_left = read_imageui(input, smp,
            (int2)(w_lower, h_upper));
    uchar4 bottom_right = read_imageui(input, smp,
            (int2)(w_upper, h_upper));

    uchar4 top = mad((top_right - top_left), w_lerp, top_left);
    uchar4 bottom = mad((bottom_right - bottom_left), w_lerp, bottom_left);
    uchar4 out = mad((bottom - top), h_lerp, top);

    output[y*out_width*3+x*3] = out.x;
    output[y*out_width*3+x*3+1] = out.y;
    output[y*out_width*3+x*3+2] = out.z;
}