#include <common.h>
__kernel void filter_buffer_to_image(GLOBAL_WORK_GROUP_SIZE_DIM2
                                     __global const DATA_TYPE *input, /* OIHW */
                                     __private const int input_offset,
                                     __private const int out_channel,
                                     __private const int filter_h,
                                     __private const int filter_w,
                                     __private const int inner_size,
                                     __write_only image2d_t output) {
  int w = get_global_id(0);
  int h = get_global_id(1);

#ifndef NON_UNIFORM_WORK_GROUP
  if (w >= global_size_dim0 || h >= global_size_dim1) {
    return;
  }
#endif

  const int in_channel_idx = w;
  const int hw_size = mul24(filter_w, filter_h);
  int out_channel_idx = h / hw_size;
  const int hw_idx = h - mul24(out_channel_idx, hw_size);
  out_channel_idx = out_channel_idx << 2;
  const int h_idx = hw_idx / filter_w;
  const int w_idx = hw_idx - mul24(h_idx, filter_w);
  const int offset = input_offset +
      mad24(out_channel_idx, inner_size,
          mad24(mad24(in_channel_idx, filter_h, h_idx), filter_w, w_idx));

  DATA_TYPE4 values = 0;
  if (out_channel_idx < out_channel) {
    const int size = out_channel - out_channel_idx;
    if (size < 4) {
      switch (size) {
        case 3:
          values.z = *(input + offset + 2 * inner_size);
        case 2:
          values.y = *(input + offset + 1 * inner_size);
        case 1:
          values.x = *(input + offset);
      }
    } else {
      values.w = *(input + offset + 3 * inner_size);
      values.z = *(input + offset + 2 * inner_size);
      values.y = *(input + offset + 1 * inner_size);
      values.x = *(input + offset);
    }
  }

  int2 coord = (int2)(w, h);
  WRITE_IMAGET(output, coord, values);
}

__kernel void filter_image_to_buffer(GLOBAL_WORK_GROUP_SIZE_DIM2
                                     __global DATA_TYPE *output, /* OIHW */
                                     __private const int out_channel,
                                     __private const int filter_h,
                                     __private const int filter_w,
                                     __private const int inner_size,
                                     __read_only image2d_t input) {
  int w = get_global_id(0);
  int h = get_global_id(1);

#ifndef NON_UNIFORM_WORK_GROUP
  if (w >= global_size_dim0 || h >= global_size_dim1) {
    return;
  }
#endif

  const int in_channel_idx = w;
  const int hw_size = mul24(filter_w, filter_h);
  int out_channel_idx = h / hw_size;
  const int hw_idx = h - mul24(out_channel_idx, hw_size);
  out_channel_idx = out_channel_idx << 2;
  const int h_idx = hw_idx / filter_w;
  const int w_idx = hw_idx - mul24(h_idx, filter_w);
  const int offset =
      mad24(out_channel_idx, inner_size,
            mad24(mad24(in_channel_idx, filter_h, h_idx), filter_w, w_idx));

  if (out_channel_idx < out_channel) {
    int2 coord = (int2)(w, h);
    DATA_TYPE4 values = READ_IMAGET(input, SAMPLER, coord);
    const int size = (out_channel - out_channel_idx);
    if (size < 4) {
      switch (size) {
        case 3:
          output[offset + (inner_size << 1)] = values.z;
        case 2:
          output[offset + inner_size] = values.y;
        case 1:
          output[offset] = values.x;
      }
    } else {
      output[offset + 3 * inner_size] = values.w;
      output[offset + 2 * inner_size] = values.z;
      output[offset + inner_size] = values.y;
      output[offset] = values.x;
    }
  }
}


__kernel void in_out_buffer_to_image(GLOBAL_WORK_GROUP_SIZE_DIM2
                                     __global const DATA_TYPE *input, /* nhwc */
                                     __private const int input_offset,
                                     __private const int height,
                                     __private const int width,
                                     __private const int channels,
                                     __write_only image2d_t output) {
  int w = get_global_id(0);
  int h = get_global_id(1);

#ifndef NON_UNIFORM_WORK_GROUP
  if (w >= global_size_dim0 || h >= global_size_dim1) {
    return;
  }
#endif

  const int batch_idx = h / height;
  const int height_idx = h - mul24(batch_idx, height);
  int channel_idx = w / width;
  const int width_idx = w - mul24(channel_idx, width);
  channel_idx = channel_idx << 2;
  const int offset =
      mad24(mad24(mad24(batch_idx, height, height_idx), width, width_idx),
            channels,
            input_offset + channel_idx);

  const int size = channels - channel_idx;
  DATA_TYPE4 values = 0;
  if (size < 4) {
    switch(size) {
      case 3:
        values.z = *(input + offset + 2);
      case 2:
        values.y = *(input + offset + 1);
      case 1:
        values.x = *(input + offset);
    }
  } else {
    values = vload4(0, input + offset);
  }
  int2 coord = (int2)(w, h);
  WRITE_IMAGET(output, coord, values);
}

__kernel void in_out_image_to_buffer(GLOBAL_WORK_GROUP_SIZE_DIM2
                                     __global DATA_TYPE *output, /* nhwc */
                                     __private const int height,
                                     __private const int width,
                                     __private const int channels,
                                     __read_only image2d_t input) {
  int w = get_global_id(0);
  int h = get_global_id(1);

#ifndef NON_UNIFORM_WORK_GROUP
  if (w >= global_size_dim0 || h >= global_size_dim1) {
    return;
  }
#endif

  const int batch_idx = h / height;
  const int height_idx = h - mul24(batch_idx, height);
  int channel_idx = w / width;
  const int width_idx = w - mul24(channel_idx, width);
  channel_idx = channel_idx << 2;
  const int offset = mad24(mad24(mad24(batch_idx, height, height_idx), width, width_idx),
                           channels,
                           channel_idx);

  int2 coord = (int2)(w, h);
  DATA_TYPE4 values = READ_IMAGET(input, SAMPLER, coord);
  const int size = channels - channel_idx;
  if (size < 4) {
    switch (size) {
      case 3:
        output[offset+2] = values.s2;
      case 2:
        output[offset+1] = values.s1;
      case 1:
        output[offset] = values.s0;
    }
  } else {
    vstore4(values, 0, output + offset);
  }
}

// TODO(liuqi): Support multiplier > 1
__kernel void dw_filter_buffer_to_image(GLOBAL_WORK_GROUP_SIZE_DIM2
                                        __global const DATA_TYPE *input, /* MIHW */
                                        __private const int input_offset,
                                        __private const int multiplier,
                                        __private const int in_channel,
                                        __private const int filter_h,
                                        __private const int filter_w,
                                        __write_only image2d_t output) { /* ic%4 * kh * kw * m, ic/4 */
  const int w = get_global_id(0);
  const int h = get_global_id(1);

#ifndef NON_UNIFORM_WORK_GROUP
  if (w >= global_size_dim0 || h >= global_size_dim1) {
    return;
  }
#endif

  DATA_TYPE4 values = 0;
  if (multiplier == 1) {
    const int in_channel_idx = h << 2;
    const int h_idx = w / filter_w;
    const int w_idx = w - mul24(h_idx, filter_w);

    const int offset = input_offset
        + mad24(mad24(in_channel_idx, filter_h, h_idx), filter_w, w_idx);

    const int hw_size = mul24(filter_h, filter_w);
    const int size = in_channel - in_channel_idx;
    if (in_channel_idx < in_channel) {
      if (size < 4) {
        switch(size) {
          case 3:
            values.z = *(input + offset + 2 * hw_size);
          case 2:
            values.y = *(input + offset + hw_size);
          case 1:
            values.x = *(input + offset);
        }
      } else {
        values.x = *(input + offset);
        values.y = *(input + offset + hw_size);
        values.z = *(input + offset + (hw_size << 1));
        values.w = *(input + offset + 3 * hw_size);
      }
    }
  }

  int2 coord = (int2)(w, h);
  WRITE_IMAGET(output, coord, values);
}