#ifndef _EAGLEEYE_GENERATOR_H_
#define _EAGLEEYE_GENERATOR_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/DynamicArray.h"
#include <math.h>
#include <time.h>

namespace eagleeye
{
template <class T> class Variable;

class RandomSeed
{
public:
	RandomSeed()
	{
		//set random seed by system time
		srand(time(0));
	}
	~RandomSeed(){};
};

class GeneratorBase
{
public:
	void setSeed(unsigned int seed)
	{
		srand(seed);
	}

	virtual ~GeneratorBase(){};

protected:
	GeneratorBase(){};
	
	static RandomSeed m_time_seed;
};

template<typename T>
class Generator:public GeneratorBase
{
	template<class AnyT> friend class Variable;
public:
	/**
	 *	@brief generate random number
	 */
	virtual T gen()=0;
	virtual ~Generator(){};

protected:
	Generator(){};
};

//////////////////////////////////////////////////////////////////////////
/**
 *	@brief uniform distribution generator
 */
template<typename T>
class UniformGenerator:public Generator<T>
{
	template<class AnyT> friend class Variable;
public:
	/**
	 *	@brief generate random confined to uniform distribution
	 */
	virtual T gen();
	virtual ~UniformGenerator(){};

protected:
	UniformGenerator(T low=T(0),T high=T(1));

	T m_low;
	T m_high;
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
/**
 *	@brief discrete distribution generator
 */
template<typename T>
class DiscreteDisGenerator:public Generator<T>
{
	template<class AnyT> friend class Variable;
public:
	/**
	 *	@brief generate random confined to discrete distribution
	 */
	virtual T gen();
	virtual ~DiscreteDisGenerator();

protected:
	DiscreteDisGenerator(DynamicArray<T> discrete_data,
						DynamicArray<float> distribution,
						UniformGenerator<float>* uniform_gen);

	DynamicArray<T> m_discrete_data;			/**< discrete data*/
	DynamicArray<float> m_cumulative_dis;		/**< cumulative distribution*/
	int m_discrete_num;							/**< the number of discrete data*/

	UniformGenerator<float>* m_uniform_generator;	/**< uniform random generator*/
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
template<typename T>
class GaussianGenerator:public Generator<T>
{
	template<class AnyT> friend class Variable;
public:
	/**
	 *	@brief generate random confined to Gaussian distribution
	 */
	virtual T gen();
	virtual ~GaussianGenerator();

protected:
	GaussianGenerator(float mean_val,float variance_val,UniformGenerator<float>* u1,UniformGenerator<float>* u2);

	UniformGenerator<float>* m_u1_generator;
	UniformGenerator<float>* m_u2_generator;

	float m_mean_val;
	float m_variance_val;
};
//////////////////////////////////////////////////////////////////////////
}

#include "eagleeye/basic/Generator.hpp"
#endif