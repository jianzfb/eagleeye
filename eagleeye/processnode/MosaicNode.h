#ifndef _EAGLEEYE_MOSAICNODE_H_
#define _EAGLEEYE_MOSAICNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
namespace eagleeye
{
class MosaicNode: public AnyNode{
public:
    typedef MosaicNode              Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(MosaicNode);

    MosaicNode();
    virtual ~MosaicNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    void setRadius(int radius);
    void getRadius(int& radius);

    void setMosaicCategory(int category);
    void getMosaicCategory(int& category);

private:
    MosaicNode(const MosaicNode&);
    void operator=(const MosaicNode&);

    void squareMosaic(Matrix<Array<unsigned char, 3>> source, Matrix<Array<unsigned char, 3>> result,int cx, int cy);
    void hexagonMosaic(Matrix<Array<unsigned char, 3>> source, Matrix<Array<unsigned char, 3>> result,int cx, int cy);
    void triangleMosaic(Matrix<Array<unsigned char, 3>> source, Matrix<Array<unsigned char, 3>> result,int cx, int cy);

    void getHexagonCenter(int cx, int cy, int image_cols, int image_rows, int& hex_cx, int& hex_cy);

    int m_radius;
    int m_category;

    bool m_new_run;
    bool m_is_start;
};    
} // namespace eagleeye

#endif