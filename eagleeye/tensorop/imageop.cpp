#include "eagleeye/tensorop/imageop.h"
#include "eagleeye/basic/MatrixMath.h"
#include <vector>
#include <iostream>

namespace eagleeye{
Tensor<float> crop_and_resize(Tensor<float> image, 
							  Tensor<float> bbox, 
							  std::vector<int> bbox_ind, 
							  std::vector<int> crop_size){
	assert(image.ndim() == 4);
	assert(bbox.ndim() == 2);
	assert(bbox.shape()[0] == bbox_ind.size());

	std::vector<int64_t> shape = image.shape();
	int image_h = shape[1];
	int image_w = shape[2];
	int image_dim = shape[3];	

	Tensor<float> cropped_images(std::vector<int64_t>{bbox.shape()[0], crop_size[0], crop_size[1], shape[3]}, EagleeyeRuntime(EAGLEEYE_CPU));
	int image_num = shape[0];
	for(int b_i=0; b_i<bbox_ind.size(); ++b_i){
		// bbox
		float bbox_y1 = bbox.at(b_i, 0);
		bbox_y1 = bbox_y1 > 0.0 ? bbox_y1 : 0.0;
		float bbox_y1i = bbox_y1 * (image_h-1);

		float bbox_x1 = bbox.at(b_i, 1);
		bbox_x1 = bbox_x1 > 0.0 ? bbox_x1 : 0.0;
		float bbox_x1i = bbox_x1 * (image_w-1);

		float bbox_y2 = bbox.at(b_i, 2);
		bbox_y2 = bbox_y2 < 1.0 ? bbox_y2 : 1.0;
		float bbox_y2i = bbox_y2 * (image_h-1);
		bbox_y2i = bbox_y2i < image_h ? bbox_y2i : image_h;

		float bbox_x2 = bbox.at(b_i, 3);
		bbox_x2 = bbox_x2 < 1.0 ? bbox_x2 : 1.0;
		float bbox_x2i = bbox_x2 * (image_w-1);
		bbox_x2i = bbox_x2i < image_w ? bbox_x2i : image_w;

		float bbox_w = bbox_x2i - bbox_x1i;
		float bbox_h = bbox_y2i - bbox_y1i;

		int image_index = bbox_ind[b_i];
		Tensor<float> image_slice = image.slice(Range(image_index, image_index+1));

		for(int i=0; i<crop_size[0]; ++i){
			for(int j=0; j<crop_size[1]; ++j){
					float r_f = (float(i) / float(crop_size[0] - 1)) * float(bbox_h) + bbox_y1i;
					float c_f = (float(j) / float(crop_size[1] - 1)) * float(bbox_w) + bbox_x1i;

					int r = int(floor(r_f));
					int c = int(floor(c_f));

					float u = r_f - r;
					float v = c_f - c;

					int first_r_index = eagleeye_min(r, image_h-1);
					int first_c_index = eagleeye_min(c, image_w-1);

					int second_r_index = first_r_index;
					int second_c_index = eagleeye_min((c + 1), image_w-1);

					int third_r_index = eagleeye_min((r + 1), image_h-1);
					int third_c_index = first_c_index;

					int fourth_r_index = third_r_index;
					int fourth_c_index = second_c_index;

					float first_weight = (1 - u) * (1 - v);
					float second_weight = (1 - u) * v;
					float third_weight = u * (1 - v);
					float fourth_weight = u * v;

					for(int k=0; k<image_dim; ++k){
						float p1 = image_slice.at(0, first_r_index, first_c_index, k);
						float p2 = image_slice.at(0, second_r_index, second_c_index, k);
						float p3 = image_slice.at(0, third_r_index, third_c_index, k);
						float p4 = image_slice.at(0, fourth_r_index, fourth_c_index, k);

						cropped_images.at(b_i, i, j, k) = p1*first_weight+p2*second_weight+p3*third_weight+p4*fourth_weight;
					}
			}
		}
	}

	return cropped_images;
}	


}