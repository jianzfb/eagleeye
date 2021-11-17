#ifndef _EAGLEEYE_NNNODE_H_
#define _EAGLEEYE_NNNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"

namespace eagleeye{
enum PreProcessMode{
    PREPROCESS_SQUARE = 0,
}; 
class NNNode:public AnyNode{
public:    
    NNNode();
    virtual ~NNNode();

    /**
     * @brief modify input dagta
     * 
     * @param input_image 
     * @param output_image 
     * @param max_dim
     * @param window 
     * @param scale 
     * @param padding 
     * @param crop 
     * @param mode
     */
    void moldInput(Matrix<Array<unsigned char, 3>> input_image, 
                    Matrix<Array<unsigned char,3>>& output_image,
                    int min_dim, 
                    int max_dim,
                    Matrix<int>& window, 
                    float& scale, 
                    Matrix<int>& padding, 
                    Matrix<int>& crop,
                    PreProcessMode mode=PREPROCESS_SQUARE);

    /**
     * @brief change/unchange input data
     * 
     * @param input_image 
     * @param mean 
     * @param var 
     * @return Matrix<Array<float,3>> 
     */
    Matrix<Array<float,3>> moldImage(const Matrix<Array<unsigned char,3>>& input_image, float* mean, float* var);
    Matrix<Array<unsigned char,3>> unmoldImage(const Matrix<Array<float,3>>& normalized_images, float* mean, float* var);

    /**
     * @brief Get the Anchors object
     * 
     * @param image_height 
     * @param image_width 
     * @param anchor_scales 
     * @param anchor_ratios 
     * @param shapes 
     * @param strides 
     * @param anchor_stride 
     * @return Matrix<float> 
     */
    Matrix<float> getAnchors(int image_height, 
                             int image_width, 
                             std::vector<float> anchor_scales,
                             std::vector<float> anchor_ratios,
                             std::vector<std::vector<int>> shapes,
                             std::vector<int> strides,
                             int anchor_stride);

    /**
     * @brief normalize/denormalize bbox 
     * 
     * @param bboxs 
     * @param height 
     * @param width 
     * @return Matrix<float> 
     */
    Matrix<float> normBoxes(const Matrix<float>& bboxs, int height, int width);
    Matrix<float> normBoxes(const Matrix<int>& bboxs, int height, int width);
    Matrix<int> denormBoxes(const Matrix<float>& norm_bboxs, int height, int width);

private:
    NNNode(const NNNode&);
    void operator=(const NNNode&);
};
}
#endif