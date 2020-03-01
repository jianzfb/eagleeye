#ifndef _EAGLEEYE_MOVINGDETNODE_H_
#define _EAGLEEYE_MOVINGDETNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/algorithm/homographymodel.h"

namespace eagleeye{
class MovingDetNode:public ImageProcessNode<ImageSignal<Array<unsigned char,3>>,ImageSignal<float>>{
public:
    typedef MovingDetNode       Self;
    typedef ImageProcessNode<ImageSignal<Array<unsigned char,3>>,ImageSignal<float>> Superclass;

    typedef ImageSignal<Array<unsigned char, 3>>                InputSigType;
    typedef ImageSignal<float>                                  OutputSigType;

    typedef typename InputSigType::MetaType                     InputPixelType;
    typedef typename OutputSigType::MetaType                    OutputPixelType;

    EAGLEEYE_CLASSIDENTITY(MovingDetNode);

    EAGLEEYE_INPUT_PORT_TYPE(InputSigType,      0, FRAME);
    EAGLEEYE_OUTPUT_PORT_TYPE(OutputPixelType,  0, BOX);

    MovingDetNode();
    virtual ~MovingDetNode();

    virtual void executeNodeInfo();

    void setAlpha(float a1);
    void getAlpha(float& a1);

    void setBeta(float a2);
    void getBeta(float& a2);

    void setP(float p);
    void getP(float& p);

    /**
     * @brief Set/Get the Smooth Diff object
     * 
     * @param diff_sig 
     */
    void setSmoothDiff(float diff_sig);
    void getSmoothDiff(float& diff_sig);

    /**
     * @brief Set/Get the Smooth Spatial object
     * 
     * @param spatial_sig 
     */
    void setSmoothSpatial(float spatial_sig);
    void getSmoothSpatial(float& spatial_sig);

private:
    MovingDetNode(const MovingDetNode&);
    void operator=(const MovingDetNode&);

    Matrix<unsigned char> m_pre_frame;
    float m_alpha;
    float m_beta;
    float m_p;

    float m_smooth_diff_sig;
    float m_smooth_spatial_sig;
    RANSAC<HomographyModel>* m_h_estimator;
};
}
#endif