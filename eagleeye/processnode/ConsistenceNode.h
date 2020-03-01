#ifndef _EAGLEEYE_CONSISTENCENODE_H_
#define _EAGLEEYE_CONSISTENCENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"

namespace eagleeye{
class ConsistenceNode:public AnyNode{
public:
    typedef ConsistenceNode         Self;
    typedef AnyNode                 Superclass;

    ConsistenceNode();
    virtual ~ConsistenceNode();

    EAGLEEYE_CLASSIDENTITY(ConsistenceNode);

    virtual void executeNodeInfo();

    void setLambda(float lambda);
    void getLambda(float& lambda);

protected:
    inline float sqr(float x) { return x*x; };
    float getWeight(const unsigned char* cur_frame, const unsigned char* prev_frame, int W, int H, const float* flow, int pix);
    void gaussSeidel(float* result_init, 
                                  const float* processed, 
                                  const float* diag, 
                                  const float* rhs, const int W, const int H, const int niter);

    void multiscaleSolver(float* result_init, float* processed, int W, int H, float* diag, float* rhs);
    void solveFrame(unsigned char* prevInput, 
                                 unsigned char* curInput, 
                                 float* curProcessed, 
                                 float* prevSolution, 
                                 float* curSolution, 
                                 float* optical_flow,
                                 int W, int H, float lambda_t);

    float bilinear(const unsigned char* table, int W, int H, float x, float y, int stride = 1);
    float bilinear(const float* table, int W, int H, float x, float y, int stride = 1);


private:
    ConsistenceNode(const ConsistenceNode &);
    void operator=(const ConsistenceNode &);

    Matrix<Array<unsigned char,3>> m_pre_input;
    Matrix<float> m_pre_solution;
    bool m_is_first;
    float m_lambda;
};
}
#endif