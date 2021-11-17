#ifndef _EAGLEEYE_VARIABLE_H_
#define _EAGLEEYE_VARIABLE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Generator.h"
#include "eagleeye/basic/DynamicArray.h"

namespace eagleeye
{
/**
 *	@brief construct random confined to any distribution.
 *	@detail The core idea comes from 
 *	@par example:
 *	@code
 *	Variable<float> random_var=Variable<float>::uniform(0.0f,1.0f);
 *	float val=random_var.var();	//generate one random confined to 
 *								//uniform distribution
 *	@endcoe
 */
template<typename T>
class Variable
{
public:
	/**
	 *	@brief construct uniform random number
	 */
	static Variable uniform(T low,T high);

	/**
	 *	@brief construct random confined to gaussian distribution
	 */
	static Variable gaussian(float mean_val,float variance_val);

	/**
	 *	@brief construct random number confined to fixed distribution
	 */
	static Variable discreteDis(DynamicArray<T> discrete_data,DynamicArray<float> distribution);

	/**
	 *	@brief get random number
	 */
	T var();

	/**
	 *	@brief switch to debug mode
	 *	@note In this mode, the random series are the same every time.
	 *	Of course, we should set the same seed.
	 */
	void switchToDebug(unsigned int seed=1000);

	~Variable();

private:
	Variable(Generator<T>* gen);
	std::shared_ptr<Generator<T>> m_gen;
};
}


#include "eagleeye/basic/Variable.hpp"
#endif