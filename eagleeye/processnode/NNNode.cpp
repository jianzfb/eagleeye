#include "eagleeye/processnode/NNNode.h"
#include "eagleeye/basic/MatrixMath.h"

namespace eagleeye
{
NNNode::NNNode(){

}   
NNNode::~NNNode(){

} 

void NNNode::moldInput(Matrix<Array<unsigned char, 3>> input_image, 
                              Matrix<Array<unsigned char,3>>& output_image, 
                              int min_dim,
                              int max_dim,
                              Matrix<int>& window, 
                              float& scale, 
                              Matrix<int>& padding, 
                              Matrix<int>& crop,
                              PreProcessMode mode){
    // resizes an image keeping the aspect ratio unchanged
    int h = input_image.rows();
    int w = input_image.cols();
    window = Matrix<int>(1,4);
    window.at(0,0) = 0;
    window.at(0,1) = 0;
    window.at(0,2) = h;
    window.at(0,3) = w;
    scale = 1.0f;
    padding = Matrix<int>(3,2);
    padding.at(0,0) = 0;
    padding.at(0,1) = 0;
    padding.at(1,0) = 0;
    padding.at(1,1) = 0;
    padding.at(2,0) = 0;
    padding.at(2,1) = 0;

    int top_pad = 0;
    int bottom_pad = 0;
    int left_pad = 0;
    int right_pad = 0;

    if(mode == PREPROCESS_SQUARE){
        int image_max = h > w ? h : w;
        if(round(image_max * scale) > max_dim){
            scale = float(max_dim) / float(image_max);
        }

        Matrix<Array<unsigned char,3>> resized_img = input_image;
        if(scale != 1.0f){
            resized_img = resize(input_image, round(h*scale),round(w*scale), BILINEAR_INTERPOLATION);
        }

        int resized_h = resized_img.rows();
        int resized_w = resized_img.cols();
        top_pad = (max_dim - resized_h) / 2;
        bottom_pad = max_dim - resized_h - top_pad;
        left_pad = (max_dim - resized_w) / 2;
        right_pad = max_dim - resized_w - left_pad;
        padding.at(0,0) = top_pad;
        padding.at(0,1) = bottom_pad;
        padding.at(1,0) = left_pad;
        padding.at(1,1) = right_pad;

        window.at(0,0) = top_pad;               // y1
        window.at(0,1) = left_pad;              // x1
        window.at(0,2) = resized_h + top_pad;   // y2
        window.at(0,3) = resized_w + left_pad;  // x2

        output_image = Matrix<Array<unsigned char, 3>>(max_dim, max_dim);
        output_image(Range(window.at(0,0),window.at(0,2)),Range(window.at(0,1),window.at(0,3))).copy(resized_img);
    }
}

Matrix<Array<float,3>> NNNode::moldImage(const Matrix<Array<unsigned char,3>>& input_image, float* mean, float* var){
    int h = input_image.rows();
    int w = input_image.cols();

    Matrix<Array<float,3>> normalized_image(h, w);
    for(int i=0; i<h; ++i){
        float* normalized_image_ptr = (float*)normalized_image.row(i);
        const unsigned char* image_ptr = (unsigned char*)input_image.row(i);
        for(int j=0; j<w; ++j){
            normalized_image_ptr[j*3] = (image_ptr[j*3]-mean[0])/var[0];
            normalized_image_ptr[j*3+1] = (image_ptr[j*3+1]-mean[1])/var[1];
            normalized_image_ptr[j*3+2] = (image_ptr[j*3+2]-mean[2])/var[2];
        }
    }

    return normalized_image;
}

Matrix<Array<unsigned char,3>> NNNode::unmoldImage(const Matrix<Array<float,3>>& normalized_images, float* mean, float* var){
    int h = normalized_images.rows();
    int w = normalized_images.cols();
    
    Matrix<Array<unsigned char,3>> image(h,w);
    for(int i=0; i<h; ++i){
        const float* normalized_image_ptr = (float*)normalized_images.row(i);
        unsigned char* image_ptr = (unsigned char*)image.row(i);
        for(int j=0; j<w; ++j){
            image_ptr[j*3] = eagleeye_clip(normalized_image_ptr[j*3] * var[0] + mean[0],0,255);
            image_ptr[j*3+1] = eagleeye_clip(normalized_image_ptr[j*3+1] * var[1] + mean[1],0,255);
            image_ptr[j*3+2] = eagleeye_clip(normalized_image_ptr[j*3+2] * var[2] + mean[2],0,255);
        }
    }
    return image;
}


Matrix<float> NNNode::getAnchors(int image_height, 
                                 int image_width, 
                                 std::vector<float> anchor_scales,
                                 std::vector<float> anchor_ratios,
                                 std::vector<std::vector<int>> shapes,
                                 std::vector<int> feature_strides,
                                 int anchor_stride){
    std::vector<Matrix<float>> anchors_list;                                     
    // [anchor_count, (y1, x1, y2, x2)]
    for(int i=0; i<anchor_scales.size(); ++i){
        float scale = anchor_scales[i];
        int stride = feature_strides[i];

        // combine scale and ratios
        Matrix<float> s(1,1);
        s.at(0,0) = scale;

        Matrix<float> r(1,anchor_ratios.size());
        for(int ri=0; ri<anchor_ratios.size(); ++ri){
            r.at(0,ri) = anchor_ratios[ri];
        }

        Matrix<float> ss,rr;
        meshgrid(s,r,ss,rr);

        ss = ss.flatten();   
        rr = rr.flatten();

        // enumerate heights and widths from scales and ratios
        Matrix<float> heights, widths;
        heights = ss.div(msqrt(rr));
        widths = ss.mul(msqrt(rr));

        // enumerate shifts in feature space
        int feature_h = shapes[i][0];
        int feature_w = shapes[i][1];
        Matrix<float> shifts_y,shifts_x;
        shifts_y = arange<float>(0, feature_h, anchor_stride) * stride;
        shifts_x = arange<float>(0, feature_w, anchor_stride) * stride;

        Matrix<float> shifts_yy,shifts_xx;
        meshgrid(shifts_x, shifts_y, shifts_xx, shifts_yy);

        // enumerate combinations of shifts, widths and heights
        Matrix<float> box_widths, box_centers_x;
        Matrix<float> box_heights, box_centers_y;
        meshgrid(widths, shifts_xx, box_widths, box_centers_x);
        meshgrid(heights, shifts_yy, box_heights, box_centers_y);

        // reshape to (y,x) and (h,w)
        // Matrix<float> box_centers = concat();
        // Matrix<float> box_sizes = concat();
        box_centers_y = box_centers_y.reshape(box_centers_y.rows()*box_centers_y.cols(),1);
        box_centers_x = box_centers_x.reshape(box_centers_x.rows()*box_centers_x.cols(),1);
        std::vector<Matrix<float>> box_centers_list={box_centers_y,box_centers_x};
        Matrix<float> box_centers = concat(box_centers_list, 1);

        box_heights = box_heights.reshape(box_heights.rows()*box_heights.cols(),1);
        box_widths = box_widths.reshape(box_widths.rows()*box_widths.cols(),1);
        std::vector<Matrix<float>> box_sizes_list={box_heights,box_widths};
        Matrix<float> box_sizes = concat(box_sizes_list, 1);

        Matrix<float> y1x1 = box_centers - box_sizes*0.5f;
        Matrix<float> y2x2 = box_centers + box_sizes*0.5f;

        std::vector<Matrix<float>> y1x1y2x2={y1x1, y2x2};
        Matrix<float> boxes = concat(y1x1y2x2, 1);

        // add 
        anchors_list.push_back(boxes);
    }

    // std::cout<<"bbox layer 1"<<std::endl;
    // std::cout<<anchors_list[0];

    Matrix<float> anchors = concat(anchors_list, 0);
    anchors = normBoxes(anchors, image_height, image_width);
    return anchors;
}

Matrix<float> NNNode::normBoxes(const Matrix<float>& bboxs, int height, int width){
    int bboxes_num = bboxs.rows();
    // y1,x1,y2,x2
    Matrix<float> norm_bboxs(bboxes_num, 4);
    for(int i=0; i<bboxes_num; ++i){
        float* norm_ptr = norm_bboxs.row(i);
        const float* ptr = bboxs.row(i);
        for(int j=0; j<4; ++j){
            // y1
            norm_ptr[0] = (ptr[0] - 0)/(height - 1);
            // x1
            norm_ptr[1] = (ptr[1] - 0)/(width - 1);
            // y2
            norm_ptr[2] = (ptr[2] - 1)/(height - 1);
            // x2
            norm_ptr[3] = (ptr[3] - 1)/(width - 1);
        }
    }

    return norm_bboxs;
}

Matrix<float> NNNode::normBoxes(const Matrix<int>& bboxs, int height, int width){
    int bboxes_num = bboxs.rows();
    // y1,x1,y2,x2
    Matrix<float> norm_bboxs(bboxes_num, 4);
    for(int i=0; i<bboxes_num; ++i){
        float* norm_ptr = norm_bboxs.row(i);
        const int* ptr = bboxs.row(i);
        // y1
        norm_ptr[0] = (ptr[0] - 0.0f)/(height - 1);
        // x1
        norm_ptr[1] = (ptr[1] - 0.0f)/(width - 1);
        // y2
        norm_ptr[2] = (ptr[2] - 1.0f)/(height - 1);
        // x2
        norm_ptr[3] = (ptr[3] - 1.0f)/(width - 1);
    }

    return norm_bboxs;
}
Matrix<int> NNNode::denormBoxes(const Matrix<float>&norm_bboxs, int height, int width){
    int bboxes_num = norm_bboxs.rows();
    // y1,x1,y2,x2
    Matrix<int> bboxs(bboxes_num, 4);
    for(int i=0; i<bboxes_num; ++i){
        const float* norm_ptr = norm_bboxs.row(i);
        int* ptr = bboxs.row(i);

        // y1
        ptr[0] = (norm_ptr[0]) * (height - 1);
        // x1
        ptr[1] = (norm_ptr[1]) * (width - 1);
        // y2
        ptr[2] = (norm_ptr[2]) * (height - 1);
        // x2
        ptr[3] = (norm_ptr[3]) * (width - 1);
    }

    return bboxs;
}


} // namespace eagleeye
