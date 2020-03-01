namespace eagleeye
{
template<typename T>
Variable<T>::Variable(Generator<T>* gen)
{
	this->m_gen = std::shared_ptr<Generator<T>>(gen);
}

template<typename T>
Variable<T>::~Variable()
{
}

template<typename T>
Variable<T> Variable<T>::uniform(T low,T high)
{
	return Variable<T>(new UniformGenerator<T>(low,high));
}

template<typename T>
Variable<T> Variable<T>::gaussian(float mean_val,float variance_val)
{
	Generator<T>* gen=new GaussianGenerator<T>(mean_val,variance_val,
		new UniformGenerator<float>(-1.0,1.0),
		new UniformGenerator<float>(-1.0,1.0));
	return Variable<T>(gen);
}

template<typename T>
Variable<T> Variable<T>::discreteDis(DynamicArray<T> discrete_data,DynamicArray<float> distribution)
{
	Generator<T>* gen = new DiscreteDisGenerator<T>(discrete_data,distribution,new UniformGenerator<float>(0.0f,1.0f));
	return Variable<T>(gen);
}

template<typename T>
T Variable<T>::var()
{
	return this->m_gen->gen();
}

template<typename T>
void Variable<T>::switchToDebug(unsigned int seed/* =1000 */)
{
	this->m_gen->setSeed(seed);
}
}
