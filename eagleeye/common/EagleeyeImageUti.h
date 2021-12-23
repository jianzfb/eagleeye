#ifndef _EAGLEEYE_IMAGE_UTI_H_
#define _EAGLEEYE_IMAGE_UTI_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
namespace eagleeye
{
/**
 * @brief read image
 * 
 * @param image_path 
 * @param image 
 */
void imread(std::string image_path, Matrix<Array<unsigned char,3>>& image);

/**
 * @brief write image
 * 
 * @param image_path 
 * @param image 
 */
void imwrite(std::string folder, std::string name, Matrix<Array<unsigned char,3>> image);
void imwrite(std::string folder, std::string name, Matrix<unsigned char> image);

} // namespace eagleeye


#endif