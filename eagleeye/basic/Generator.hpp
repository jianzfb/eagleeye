namespace eagleeye
{
//////////////////////////////////////////////////////////////////////////
template<typename T>
UniformGenerator<T>::UniformGenerator(T low,T high)
{
	m_low = low;
	m_high = high;
}

template<typename T>
T UniformGenerator<T>::gen()
{
	return T(float(rand()) / float(RAND_MAX) * float(m_high - m_low) + float(m_low));
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//discrete distribution generator
template<typename T>
DiscreteDisGenerator<T>::DiscreteDisGenerator(DynamicArray<T> discrete_data,
	DynamicArray<float> distribution,
	UniformGenerator<float>* uniform_generator)
{
	//construct discrete distribution
	m_discrete_data = discrete_data;
	m_discrete_num = discrete_data.size();
	m_cumulative_dis = DynamicArray<float>(distribution.size() + 1);
	m_cumulative_dis[0] = 0.0f;

	int size = m_cumulative_dis.size();
	for (int i = 1; i < size; ++i)
	{
		m_cumulative_dis[i] = m_cumulative_dis[i - 1] + distribution[i - 1];
	}

	m_uniform_generator = uniform_generator;	
}
template<typename T>
DiscreteDisGenerator<T>::~DiscreteDisGenerator()
{
	if (m_uniform_generator)
	{
		delete m_uniform_generator;
	}
}

template<typename T>
T DiscreteDisGenerator<T>::gen()
{
	float uniform_rand = m_uniform_generator->gen();
	for (int i = 1; i < m_discrete_num + 1; ++i)
	{
		if (uniform_rand >= m_cumulative_dis[i - 1] && uniform_rand <= m_cumulative_dis[i])
		{
			return m_discrete_data[i-1];
		}
	}

	return T(0);
}

//////////////////////////////////////////////////////////////////////////
template<typename T>
GaussianGenerator<T>::GaussianGenerator(float mean_val,float variance_val,UniformGenerator<float>* u1,UniformGenerator<float>* u2)
{
	m_mean_val = mean_val;
	m_variance_val = variance_val;

	m_u1_generator = u1;
	m_u2_generator = u2;
}
template<typename T>
GaussianGenerator<T>::~GaussianGenerator()
{
	delete m_u1_generator;
	delete m_u2_generator;
}

template<typename T>
T GaussianGenerator<T>::gen()
{
	static float v1 = 0.0;
	static float v2 = 0.0;
	static float s = 1.0;
	static int phase = 0;
	
	float x;

	if(phase == 0) 
	{
		do 
		{			
			v1 = m_u1_generator->gen();
			v2 = m_u2_generator->gen();

			s = v1 * v1 + v2 * v2;
		} while(s >= 1 || s == 0);

		x = v1 * sqrt(-2.0 * log(s) / s);
	} else
		x = v2 * sqrt(-2.0 * log(s) / s);

	phase = 1 - phase;

	x = m_mean_val + m_variance_val * x;

	return T(x);
}
//////////////////////////////////////////////////////////////////////////
}
