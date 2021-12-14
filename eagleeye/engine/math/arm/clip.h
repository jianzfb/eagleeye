#ifndef _EAGLEEYE_CLIP_
#define _EAGLEEYE_CLIP_

#include <algorithm>
#include <string>
#include <vector>

namespace eagleeye {
namespace math {
namespace arm {

void clip_fp32(
    const float* input, int64_t num, float min, float max, float* output);
}  // namespace math
}  // namespace arm
}  // namespace paddle

#endif