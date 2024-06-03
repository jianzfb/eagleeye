#ifndef _EAGLEEYE_SHAPE_NODE_H_
#define _EAGLEEYE_SHAPE_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include <GLES3/gl3.h>


namespace eagleeye
{
class ShapeNode:public RenderNode, DynamicNodeCreator<ShapeNode>{
public:
    typedef ShapeNode                   Self;
    typedef RenderNode                  Superclass;

    EAGLEEYE_CLASSIDENTITY(ShapeNode);

    ShapeNode();
    virtual ~ShapeNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief init gl 
     */ 
    virtual void init();

private:
    ShapeNode(const ShapeNode&);
    void operator=(const ShapeNode&);

    /**
     * @brief 绘制矩形
     * 
     * @param rects 
     */
    void drawRect(Matrix<float> rects);

    /**
     * @brief 绘制检测
     * 
     * @param dets 
     */
    void drawDet(Matrix<float> dets);

    /**
     * @brief 绘制线
     * 
     * @param lines 
     */
    void drawLine(Matrix<float> lines);

    /**
     * @brief 绘制点
     * 
     * @param points 
     */
    void drawPoint(Matrix<float> points);

    /**
     * @brief 绘制landmarks
     * 
     * @param landmarks 
     */
    void drawLandmarks(Matrix<float> landmarks);

    /**
     * @brief 处理检测信号
     * 
     * @param sig_0
     * @param sig_1
     */
    void processDetSignal(AnySignal* sig_0, ImageSignal<float>* sig_1);
    
    /**
     * @brief 处理跟踪信号
     * 
     * @param sig_0
     * @param sig_1
     */
    void processTrackingSignal(AnySignal* sig_0, ImageSignal<float>* sig_1);
    
    /**
     * @brief 处理Landmark信号
     * 
     * @param sig_0
     * @param sig_1
     */
    void processLandmarkSignal(AnySignal* sig_0, ImageSignal<float>* sig_1);

    /**
     * @brief 处理点信号
     * 
     * @param sig_0
     * @param sig_1
     */
    void processPointSignal(AnySignal* sig_0, ImageSignal<float>* sig_1);
    
    /**
     * @brief 处理线信号
     * 
     * @param sig_0
     * @param sig_1
     */
    void processLineSignal(AnySignal* sig_0, ImageSignal<float>* sig_1);
    
    /**
     * @brief 处理矩形信号
     * 
     * @param sig_0
     * @param sig_1
     */
    void processRectSignal(AnySignal* sig_0, ImageSignal<float>* sig_1);

    float m_color_map[12];
    int m_color_map_num;
};    
} // namespace eagleeye

#endif