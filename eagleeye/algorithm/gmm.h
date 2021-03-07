#ifndef _GMM_H_
#define _GMM_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <fstream>
#include <memory.h>
#include <assert.h>

namespace eagleeye{
class GMM{
public:
	GMM(int dimNum = 1, int mixNum = 1, int maxIterNum=10, float tolerance=0.001);
	~GMM();

	int getDimNum()			{ return m_dimNum; }
	int getMixNum()			{ return m_mixNum; }
	int getMaxIterNum()		{ return m_maxIterNum; }
	float getEndError()	{ return m_endError; }

	float& prior(int i)	{ return m_priors[i]; }
	float* mean(int i)		{ return m_means[i]; }
	float* variance(int i)	{ return m_vars[i]; }

	void setPrior(int i,float val)	{  m_priors[i]=val; }
	void setMean(int i,float *val)		{ for(int j=0;j<m_dimNum;j++) m_means[i][j]=val[j]; }
	void setVariance(int i,float *val)	{ for(int j=0;j<m_dimNum;j++) m_vars[i][j]=val[j]; }

	float getProbability(const float* sample);

	/*	SampleFile: <size><dim><data>...*/
	void init(float *data, int N);
	void train(float *data, int N);

	friend std::ostream& operator<<(std::ostream& out, GMM& gmm);
	friend std::istream& operator>>(std::istream& in, GMM& gmm);

private:
	int m_dimNum;		// ����ά��
	int m_mixNum;		// Gaussian��Ŀ
	float* m_priors;	// GaussianȨ��
	float** m_means;	// Gaussian��ֵ
	float** m_vars;	// Gaussian����

	// A minimum variance is required. Now, it is the overall variance * 0.01.
	float* m_minVars;
	int m_maxIterNum;		// The stopping criterion regarding the number of iterations
	float m_endError;		// The stopping criterion regarding the error

private:
	// Return the "j"th pdf, p(x|j).
	float getProbability(const float* x, int j);
	void Allocate();
	void Dispose();
};
}
#endif
