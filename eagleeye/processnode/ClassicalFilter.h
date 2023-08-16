#ifndef _EAGLEEYE_CLASSIFICALFILTER_H_
#define _EAGLEEYE_CLASSIFICALFILTER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye
{
class ClassicalFilter:public AnyNode, DynamicNodeCreator<ClassicalFilter>{
public:
    typedef ClassicalFilter                 Self;
    typedef AnyNode                         Superclass;

    /**
	 *	@brief Get class identity
	 */
    EAGLEEYE_CLASSIDENTITY(ClassicalFilter);

    /**
     *  @brief constructor/destructor
     */
    ClassicalFilter();
    virtual ~ClassicalFilter();

    virtual void executeNodeInfo();

    void setFilterCategory(int category);
    void getFilterCategory(int& category);

private:
    ClassicalFilter(const ClassicalFilter&);
    void operator=(const ClassicalFilter&);

    Matrix<Array<unsigned char, 3>> filterByNostalgic(Matrix<Array<unsigned char, 3>> img, bool is_rgb);
    Matrix<Array<unsigned char,3>> filterBySketch(Matrix<Array<unsigned char,3>> img, bool is_rgb);
    Matrix<Array<unsigned char,3>> filterByIce(Matrix<Array<unsigned char, 3>> img, bool is_rgb);
    
    int m_filter_category;
};
} // namespace eagleeye

#endif