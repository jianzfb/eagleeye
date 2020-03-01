#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include "eagleeye/ml/kmeans.h"
using namespace std;


KMeans::KMeans(int dimNum, int clusterNum)
{
	m_dimNum = dimNum;
	m_clusterNum = clusterNum;

	m_means = new float*[m_clusterNum];
	for (int i = 0; i < m_clusterNum; i++)
	{
		m_means[i] = new float[m_dimNum];
		memset(m_means[i], 0, sizeof(float) * m_dimNum);
	}

	m_initMode = InitRandom;
	m_maxIterNum = 100;
	m_endError = 0.001;
}

KMeans::~KMeans()
{
	for (int i = 0; i < m_clusterNum; i++)
	{
		delete[] m_means[i];
	}
	delete[] m_means;
}

void KMeans::Cluster(const char* sampleFileName, const char* labelFileName)
{
	// Check the sample file
	ifstream sampleFile(sampleFileName, ios_base::binary);
	assert(sampleFile);

	int size = 0;
	int dim = 0;
	sampleFile.read((char*)&size, sizeof(int));
	sampleFile.read((char*)&dim, sizeof(int));
	assert(size >= m_clusterNum);
	assert(dim == m_dimNum);

	// Initialize model
	Init(sampleFile);

	// Recursion
	float* x = new float[m_dimNum];	// Sample data
	int label = -1;		// Class index
	float iterNum = 0;
	float lastCost = 0;
	float currCost = 0;
	int unchanged = 0;
	bool loop = true;
	int* counts = new int[m_clusterNum];
	float** next_means = new float*[m_clusterNum];	// New model for reestimation
	for (int i = 0; i < m_clusterNum; i++)
	{
		next_means[i] = new float[m_dimNum];
	}

	while (loop)
	{
		memset(counts, 0, sizeof(int) * m_clusterNum);
		for (int i = 0; i < m_clusterNum; i++)
		{
			memset(next_means[i], 0, sizeof(float) * m_dimNum);
		}

		lastCost = currCost;
		currCost = 0;

		sampleFile.clear();
		sampleFile.seekg(sizeof(int) * 2, ios_base::beg);

		// Classification
		for (int i = 0; i < size; i++)
		{
			sampleFile.read((char*)x, sizeof(float) * m_dimNum);
			currCost += GetLabel(x, &label);

			counts[label]++;
			for (int d = 0; d < m_dimNum; d++)
			{
				next_means[label][d] += x[d];
			}
		}
		currCost /= size;

		// Reestimation
		for (int i = 0; i < m_clusterNum; i++)
		{
			if (counts[i] > 0)
			{
				for (int d = 0; d < m_dimNum; d++)
				{
					next_means[i][d] /= counts[i];
				}
				memcpy(m_means[i], next_means[i], sizeof(float) * m_dimNum);
			}
		}

		// Terminal conditions
		iterNum++;
		if (fabs(lastCost - currCost) < m_endError * lastCost)
		{
			unchanged++;
		}
		if (iterNum >= m_maxIterNum || unchanged >= 3)
		{
			loop = false;
		}
		//DEBUG
		//cout << "Iter: " << iterNum << ", Average Cost: " << currCost << endl;
	}

	// Output the label file
	ofstream labelFile(labelFileName, ios_base::binary);
	assert(labelFile);

	labelFile.write((char*)&size, sizeof(int));
	sampleFile.clear();
	sampleFile.seekg(sizeof(int) * 2, ios_base::beg);

	for (int i = 0; i < size; i++)
	{
		sampleFile.read((char*)x, sizeof(float) * m_dimNum);
		GetLabel(x, &label);
		labelFile.write((char*)&label, sizeof(int));
	}

	sampleFile.close();
	labelFile.close();

	delete[] counts;
	delete[] x;
	for (int i = 0; i < m_clusterNum; i++)
	{
		delete[] next_means[i];
	}
	delete[] next_means;
}

//N Ϊ����������
void KMeans::Cluster(float *data, int N, int *Label)
{
	int size = 0;
	size=N;

	assert(size >= m_clusterNum);

	// Initialize model
	Init(data,N);

	// Recursion
	float* x = new float[m_dimNum];	// Sample data
	int label = -1;		// Class index
	float iterNum = 0;
	float lastCost = 0;
	float currCost = 0;
	int unchanged = 0;
	bool loop = true;
	int* counts = new int[m_clusterNum];
	float** next_means = new float*[m_clusterNum];	// New model for reestimation
	for (int i = 0; i < m_clusterNum; i++)
	{
		next_means[i] = new float[m_dimNum];
	}

	while (loop)
	{
		memset(counts, 0, sizeof(int) * m_clusterNum);
		for (int i = 0; i < m_clusterNum; i++)
		{
			memset(next_means[i], 0, sizeof(float) * m_dimNum);
		}

		lastCost = currCost;
		currCost = 0;

		// Classification
		for (int i = 0; i < size; i++)
		{
			for(int j = 0; j < m_dimNum; j++)
				x[j] = data[i*m_dimNum+j];

			currCost += GetLabel(x, &label);

			counts[label]++;
			for (int d = 0; d < m_dimNum; d++)
			{
				next_means[label][d] += x[d];
			}
		}
		currCost /= size;

		// Reestimation
		for (int i = 0; i < m_clusterNum; i++)
		{
			if (counts[i] > 0)
			{
				for (int d = 0; d < m_dimNum; d++)
				{
					next_means[i][d] /= counts[i];
				}
				memcpy(m_means[i], next_means[i], sizeof(float) * m_dimNum);
			}
		}

		// Terminal conditions
		iterNum++;
		if (fabs(lastCost - currCost) < m_endError * lastCost)
		{
			unchanged++;
		}
		if (iterNum >= m_maxIterNum || unchanged >= 3)
		{
			loop = false;
		}

		//DEBUG
		//cout << "Iter: " << iterNum << ", Average Cost: " << currCost << endl;
	}

	// Output the label file
	for (int i = 0; i < size; i++)
	{
		for(int j = 0; j < m_dimNum; j++)
				x[j] = data[i*m_dimNum+j];
		GetLabel(x, &label);
		Label[i] = label;
	}
	delete[] counts;
	delete[] x;
	for (int i = 0; i < m_clusterNum; i++)
	{
		delete[] next_means[i];
	}
	delete[] next_means;
}

void KMeans::Init(float *data, int N)
{
	int size = N;

	if (m_initMode ==  InitRandom)
	{
		int inteval = size / m_clusterNum;
		float* sample = new float[m_dimNum];

		// Seed the random-number generator with current time
		srand((unsigned)time(NULL));

		for (int i = 0; i < m_clusterNum; i++)
		{
			int select = inteval * i + (inteval - 1) * rand() / RAND_MAX;
			for(int j = 0; j < m_dimNum; j++)
				sample[j] = data[select*m_dimNum+j];
			memcpy(m_means[i], sample, sizeof(float) * m_dimNum);
		}

		delete[] sample;
	}
	else if (m_initMode == InitUniform)
	{
		float* sample = new float[m_dimNum];

		for (int i = 0; i < m_clusterNum; i++)
		{
			int select = i * size / m_clusterNum;
			for(int j = 0; j < m_dimNum; j++)
				sample[j] = data[select*m_dimNum+j];
			memcpy(m_means[i], sample, sizeof(float) * m_dimNum);
		}

		delete[] sample;
	}
	else if (m_initMode == InitManual)
	{
		// Do nothing
	}
}

void KMeans::Init(ifstream& sampleFile)
{
	int size = 0;
	sampleFile.seekg(0, ios_base::beg);
	sampleFile.read((char*)&size, sizeof(int));

	if (m_initMode ==  InitRandom)
	{
		int inteval = size / m_clusterNum;
		float* sample = new float[m_dimNum];

		// Seed the random-number generator with current time
		srand((unsigned)time(NULL));

		for (int i = 0; i < m_clusterNum; i++)
		{
			int select = inteval * i + (inteval - 1) * rand() / RAND_MAX;
			int offset = sizeof(int) * 2 + select * sizeof(float) * m_dimNum;

			sampleFile.seekg(offset, ios_base::beg);
			sampleFile.read((char*)sample, sizeof(float) * m_dimNum);
			memcpy(m_means[i], sample, sizeof(float) * m_dimNum);
		}

		delete[] sample;
	}
	else if (m_initMode == InitUniform)
	{
		float* sample = new float[m_dimNum];

		for (int i = 0; i < m_clusterNum; i++)
		{
			int select = i * size / m_clusterNum;
			int offset = sizeof(int) * 2 + select * sizeof(float) * m_dimNum;

			sampleFile.seekg(offset, ios_base::beg);
			sampleFile.read((char*)sample, sizeof(float) * m_dimNum);
			memcpy(m_means[i], sample, sizeof(float) * m_dimNum);
		}

		delete[] sample;
	}
	else if (m_initMode == InitManual)
	{
		// Do nothing
	}
}

float KMeans::GetLabel(const float* sample, int* label)
{
	float dist = -1;
	for (int i = 0; i < m_clusterNum; i++)
	{
		float temp = CalcDistance(sample, m_means[i], m_dimNum);
		if (temp < dist || dist == -1)
		{
			dist = temp;
			*label = i;
		}
	}
	return dist;
}

float KMeans::CalcDistance(const float* x, const float* u, int dimNum)
{
	float temp = 0;
	for (int d = 0; d < dimNum; d++)
	{
		temp += (x[d] - u[d]) * (x[d] - u[d]);
	}
	return sqrt(temp);
}

ostream& operator<<(ostream& out, KMeans& kmeans)
{
	out << "<KMeans>" << endl;
	out << "<DimNum> " << kmeans.m_dimNum << " </DimNum>" << endl;
	out << "<ClusterNum> " << kmeans.m_clusterNum << " </CluterNum>" << endl;

	out << "<Mean>" << endl;
	for (int i = 0; i < kmeans.m_clusterNum; i++)
	{
		for (int d = 0; d < kmeans.m_dimNum; d++)
		{
			out << kmeans.m_means[i][d] << " ";
		}
		out << endl;
	}
	out << "</Mean>" << endl;

	out << "</KMeans>" << endl;
	return out;
}
