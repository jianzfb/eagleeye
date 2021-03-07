#ifndef _KMEANS_H_
#define _KMEANS_H_
#include <fstream>
#include <memory.h>
#include <assert.h>

class KMeans{
public:
	enum InitMode
	{
		InitRandom,
		InitManual,
		InitUniform,
	};

	KMeans(int dimNum = 1, int clusterNum = 1);
	~KMeans();

	void SetMean(int i, const float* u){ memcpy(m_means[i], u, sizeof(float) * m_dimNum); }
	void SetInitMode(int i)				{ m_initMode = i; }
	void SetMaxIterNum(int i)			{ m_maxIterNum = i; }
	void SetEndError(float f)			{ m_endError = f; }

	float* GetMean(int i)	{ return m_means[i]; }
	int GetInitMode()		{ return m_initMode; }
	int GetMaxIterNum()		{ return m_maxIterNum; }
	float GetEndError()	{ return m_endError; }


	/*	SampleFile: <size><dim><data>...
		LabelFile:	<size><label>...
	*/
	void Cluster(const char* sampleFileName, const char* labelFileName);
	void Init(std::ifstream& sampleFile);
	void Init(float *data, int N);
	void Cluster(float *data, int N, int *Label);
	friend std::ostream& operator<<(std::ostream& out, KMeans& kmeans);

private:
	int m_dimNum;
	int m_clusterNum;
	float** m_means;

	int m_initMode;
	int m_maxIterNum;		// The stopping criterion regarding the number of iterations
	float m_endError;		// The stopping criterion regarding the error

	float GetLabel(const float* x, int* label);
	float CalcDistance(const float* x, const float* u, int dimNum);
};

#endif