#ifndef _EAGLEEYE_SALIENCYDETNODE_H_
#define _EAGLEEYE_SALIENCYDETNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"

namespace eagleeye{
class SaliencyDetNode:public ImageProcessNode<ImageSignal<Array<unsigned char, 3>>, ImageSignal<float>>{
public:
    /**
     * @brief define basic info
     * @note you must do these
     */
    typedef SaliencyDetNode                                                                 Self;
    typedef ImageProcessNode<ImageSignal<Array<unsigned char, 3>>, ImageSignal<float>>      Superclass;

    typedef Array<unsigned char, 3>			    InputPixelType;
	typedef float	            				OutputPixelType;

    typedef ImageSignal<Array<unsigned char, 3>>    IMAGE_SIGNAL_TYPE;
    typedef ImageSignal<float>                      SCORE_SIGNAL_TYPE;
    typedef ImageSignal<int>                        BOX_SIGNAL_TYPE;

    typedef std::pair<float, int> CostfIdx;
    struct Region{
		Region() { pixNum = 0; ad2c[0] = 0; ad2c[1] = 0; centroid[0] = 0; centroid[1] = 0;}
		int pixNum;  // Number of pixels
		std::vector<CostfIdx> freIdx;  // Frequency of each color and its index
		Array<float,2> centroid;
		Array<float,2> ad2c; // Average distance to image center
	};

    /**
     * @brief get class identity
     * 
     */
    EAGLEEYE_CLASSIDENTITY(SaliencyDetNode);

    EAGLEEYE_INPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,         0,      IMAGE);
    EAGLEEYE_OUTPUT_PORT_TYPE(SCORE_SIGNAL_TYPE,        0,    SALIENCY);
    EAGLEEYE_OUTPUT_PORT_TYPE(BOX_SIGNAL_TYPE,          1,       BOX);

    SaliencyDetNode();
    virtual ~SaliencyDetNode();

    /**
	 *	@brief execute Saliency algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    void setSegSigma(float v);
    void getSegSigma(float& v);

    void setSegK(float v);
    void getSegK(float& v);

    void setSegMinSize(int v);
    void getSegMinSize(int& v);

    void setSigmaDist(float v);
    void getSigmaDist(float& v);

    void setSaliencyThres(float v);
    void getSaliencyThres(float& v);

private:
    /**
     * @brief Construct a new SaliencyDetNode Node object
     * @note prohibit
     */
    SaliencyDetNode(const SaliencyDetNode&);
	void operator=(const SaliencyDetNode&);

    Matrix<float> getRC(Matrix<Array<unsigned char,3>> img);
    Matrix<float> getHC();
    
    static const int DefaultNums[3];
    int quantize(Matrix<Array<float,3>>& img3f, Matrix<int> &idx1i, Matrix<Array<float,3>> &_color3f, Matrix<int> &_colorNum, float ratio=0.95f, const int clrNums[3]=DefaultNums);
    void buildRegions(Matrix<int>& regIdx1i, std::vector<Region> &regs, Matrix<int> &colorIdx1i, int colorNum);
    void regionContrast(const std::vector<Region> &regs, Matrix<Array<float,3>> &color3fv, Matrix<float>& regSal1d, float sigmaDist);
    void smoothByHist(Matrix<Array<float,3>> &img3f, 
                        Matrix<int>& idx1i, 
                        Matrix<Array<float,3>>& binColor3f, 
                        Matrix<int>& colorNums1i,
                        Matrix<float> &sal1f, 
                        float delta);
    void smoothSaliency(Matrix<int> &colorNum1i, Matrix<float> &sal1f, float delta, const std::vector<std::vector<SaliencyDetNode::CostfIdx>> &similar);
    void smoothByRegion(Matrix<float> &sal1f, Matrix<int> &segIdx1i, int regNum);

    float m_segsigma;
    float m_segk;
    int m_segminsize;
    float m_sigmadist;
    float m_saliency_thres;
};    
}

#endif