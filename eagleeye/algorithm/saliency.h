#ifndef _EAGLEEYE_SALIENCY_H_
#define _EAGLEEYE_SALIENCY_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/MetaOperation.h"
#include "opencv2/opencv.hpp"
#include "eagleeye/algorithm/srm.h"
#include <ext/hash_map>

namespace eagleeye{
enum SalienceType
{
	HC = 0,
	RC
};

class Saliency{
public:
    typedef std::pair<float,int>							CostIdx;
    struct _Region
    {
        _Region(){pix_num=0;};
        int pix_num;						/**<number of pixels*/
        std::vector<CostIdx> fre_idx;		/**<Frequency of each color and its index*/
        cv::Point2d centroid;
    };


    Saliency();
    ~Saliency();

    Matrix<float> run(Matrix<unsigned char> img);

protected:
	/**
	 *	@brief Complete image quantization. 
	 *	@detail After running this routine, m_hist_color, m_hist_color_num and
	 *	m_img_color_index would be generated.
	 */
	void quantize();

	/**
	 *	@brief compute saliency image by HC algorithm
	 */
	void computeHC();

	/**
	 *	@brief smooth saliency image
	 */
	void smoothSaliency(const cv::Mat& bin_color,cv::Mat& scal,float delta,const std::vector<std::vector<CostIdx>> &similarity);
	
	/**
	 *	@brief compute saliency map based "Region Contrast"
	 */
	void computeRC();
	void buildRegion(const cv::Mat& seg_label,std::vector<_Region>& regs,const cv::Mat& hist_color, int color_num);
	void regionContrast(const std::vector<_Region> &regs,const cv::Mat& img,cv::Mat& saliency,float sigma_dist);

private:
	cv::Mat m_img;					/**< one single channel,range from 0 to 1*/
	cv::Mat m_img_color_index;		/**< store the color index of m_img*/
	cv::Mat m_img_saliency;			/**< image saliency*/

	cv::Mat m_hist_color;
	cv::Mat m_hist_color_num;

	float m_ratio;					/**< the ratio of clipping*/
	int m_quantized_num;			/**< for 3 channels*/

	float m_sigma_dist;

	SalienceType m_saliency_type;

}; 
}

#endif