#ifndef _EAGLEEYE_OPTICALFLOW_H_
#define _EAGLEEYE_OPTICALFLOW_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
/**
 * @brief compute optical flow(pre(x,y) = next(x+\Delta x, y+\Delta y))
 */
class OpticalFlowNode:public AnyNode, DynamicNodeCreator<OpticalFlowNode>{
public:
    typedef OpticalFlowNode                         Self;
    typedef AnyNode                                 Superclass;

    typedef ImageSignal<Array<unsigned char, 3>>        IMAGE_SIGNAL_TYPE;
    typedef ImageSignal<Array<float,2>>                 OPTICALFLOW_SIGNAL_TYPE;

    /**
     * @brief get class identity
     * 
     */
    EAGLEEYE_CLASSIDENTITY(OpticalFlowNode);

    /**
     * @brief define input/output port
     * 
     */
    EAGLEEYE_INPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,             0,             FRAME);
    EAGLEEYE_OUTPUT_PORT_TYPE(OPTICALFLOW_SIGNAL_TYPE,      0,       OPTICALFLOW);

    OpticalFlowNode(bool isback=false);
    virtual ~OpticalFlowNode();

    /**
	 *	@brief execute opticalflow algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief reset node
     * 
     */
    virtual void reset();

    /**
     * @brief Set/Get BRIEF sampling radius
     * 
     * @param v 
     */
    void setBRIEFSamplingRad(float v);
    void getBRIEFSamplingRad(float& v);

    /**
     * @brief Set/Get the Random Search Rad object
     * 
     * @param v 
     */
    void setRandomSearchRad(float v);
    void getRandomSearchRad(float& v);

    /**
     * @brief Set/Get the Refine Process object
     * 
     * @param refine_flag 
     */
    void setRefineProcess(bool refine_flag);
    void getRefineProcess(bool& refine_flag);

    /**
     * @brief Set/Get the Iter Count object
     * 
     * @param iters 
     */
    void setIterCount(int iters);
    void getIterCount(int& iters);

    /**
     * @brief Set/Get the Median F Size object
     * 
     * @param size 
     */
    void setMedianFSize(int size);
    void getMedianFSize(int& size);

    /**
     * @brief Set/Get the Gaussian Size object
     * 
     * @param size 
     */
    void setGaussianSize(int size);
    void getGaussianSize(int& size);

    void setEPE(float value);
    void getEPE(float& value);

    void setColorS(float value);
    void getColorS(float& value);

    void setSpatialS(float value);
    void getSpatialS(float& value);

    /**
     * @brief Set/Get the Process As Scale object
     * 
     * @param value 
     */
    void setProcessAtScale(float value);
    void getProcessAtScale(float& value);

private:
    OpticalFlowNode(const OpticalFlowNode&);
    void operator=(const OpticalFlowNode&);

    int m_N;            // biref feature length
    int m_sx;           // processed image width
    int m_sy;           // processed image height
    int m_sx_in;        // image original width
    int m_sy_in;        // image original height

    bool m_has_been_ini;
    int m_N_space;
    float m_patchRad;
    int m_isfirst;
    int m_jfMax;
    int m_doubleProp;

    float m_random_search_r;

    bool m_refine_flag;
    int m_median_filter_size;
    int m_iters;
    int m_gaussian_filter_size;

    float m_sEPE;
    float m_sp;
    float m_scolor;

    bool m_isback;
    float m_process_at_scale;
};    
}
#endif