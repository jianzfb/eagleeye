namespace eagleeye{
template<typename T>
Tensor<T>::Tensor()
	:Blob(0){
}

template<typename T>
Tensor<T>::Tensor(std::vector<int64_t> shape, EagleeyeRuntime runtime,void* data, bool copy)
	:Blob(sizeof(T)*std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;}), runtime, data, copy){
	
	this->m_shape=shape;
	this->m_range.clear();
	for(int i=0; i<this->m_shape.size(); ++i){
		this->m_range.push_back(Range(0, this->m_shape[i]));
	}
}

template<typename T>
Tensor<T>::~Tensor(){
}

template<typename T>
const std::vector<int64_t>& Tensor<T>::shape(){
	return this->m_shape;
}

template<typename T>
int64_t Tensor<T>::size(){
	if(this->m_shape.size() == 0){
		return 0;
	}

	int64_t input_size = 1;
	for(int i=0; i<this->ndim(); ++i){
		input_size *= (this->m_range[i].e - this->m_range[i].s);
	}
	return input_size;
}

template<typename T>
int64_t Tensor<T>::ndim(){
	return this->m_shape.size();
}

template<typename T>
T* Tensor<T>::dataptr(){
	return (T*)(this->cpu());
}

template<typename T>
Tensor<T> Tensor<T>::clone(){
	std::vector<int64_t> local_shape = this->shape();
	Tensor<T> target_tensor(local_shape);
	T* target_tensor_ptr = target_tensor.dataptr();
	int64_t num = this->size();

	if(this->isContinue()){
		memcpy(target_tensor_ptr, this->dataptr(), sizeof(T)*num);
		return target_tensor;
	}
	else{
		for (int64_t i = 0; i < num; ++i){
			target_tensor_ptr[i] = this->at(i);
		}

		return target_tensor;
	}
}

template<typename T>
Tensor<T> Tensor<T>::flatten(){
	int64_t num = this->size();
	std::vector<int64_t> flatten_shape{num};
	Tensor<T> target_tensor(flatten_shape);
	T* target_tensor_ptr = target_tensor.dataptr();

	if(this->isContinue()){
		memcpy(target_tensor_ptr, this->dataptr(), sizeof(T)*num);
		return target_tensor;
	}
	else{
		for (int64_t i = 0; i < num; ++i){
			target_tensor_ptr[i] = this->at(i);
		}

		return target_tensor;
	}
}

template<typename T>
bool Tensor<T>::isContinue(){
	bool is_continue = true;
	for(int i=0; i<this->m_shape.size(); ++i){
		if(this->m_shape[i] != (this->m_range[i].e - this->m_range[i].s)){
			is_continue = false;
			break;
		}
	}
	return is_continue;
}

template<typename T>
int64_t Tensor<T>::offset(int64_t i){
	assert(i < this->ndim());
	int64_t step = 1;
	for(int64_t k=i+1; k<this->ndim(); ++k){
		step *= this->m_shape[k];
	}
	return step;
}

template<typename T>
int64_t Tensor<T>::offset(const std::vector<int64_t>& v, int64_t i){
	int width = 1;
	for(int k=i; k<v.size(); k++){
		width *= v[k];
	}
	return width;
}

template<typename T>
T& Tensor<T>::at(int64_t x){
	int64_t x_p = 0;
	if(this->m_shape.size() == 1){
		x_p = this->m_range[0].s + x;
	}
	else{
		int ndim = this->ndim();
		std::vector<int64_t> local_shape;
		for(int64_t i=0; i<ndim; ++i){
			local_shape.push_back(this->m_range[i].e - this->m_range[i].s);
		}

		int64_t x_remain = x;
		for(int64_t i=0; i<ndim; ++i){
			int64_t local_width = this->offset(local_shape, i+1);
			int64_t global_width = this->offset(this->m_shape, i+1);
			int64_t i_index = x_remain / local_width;
			x_remain = x_remain - i_index * local_width;
			x_p += (this->m_range[i].s + i_index) * global_width;
		}
		assert(x_remain == 0);
	}

	T* data = (T*)(this->cpu());
	return *(data + x_p);
}

template<typename T>
T& Tensor<T>::at(int64_t x, int64_t y){
	assert(this->m_shape.size() == 2);
	assert(x >= 0 && x < this->m_range[0].e - this->m_range[0].s);
	assert(y >= 0 && y < this->m_range[1].e - this->m_range[1].s);

	int64_t x_p = this->m_range[0].s + x;
	int64_t y_p = this->m_range[1].s + y;
	T* data = (T*)(this->cpu());
	return *(data + x_p*this->offset(0) + y_p);
}

template<typename T>
T& Tensor<T>::at(int64_t x, int64_t y, int64_t z){
	assert(this->m_shape.size() == 3);
	assert(x >= 0 && x < this->m_range[0].e - this->m_range[0].s);
	assert(y >= 0 && y < this->m_range[1].e - this->m_range[1].s);
	assert(z >= 0 && z < this->m_range[2].e - this->m_range[2].s);

	int64_t x_p = this->m_range[0].s + x;
	int64_t y_p = this->m_range[1].s + y;
	int64_t z_p = this->m_range[2].s + z;
	T* data = (T*)(this->cpu());
	return *(data + x_p*this->offset(0) + y_p*this->offset(1) + z_p);
}

template<typename T>
T& Tensor<T>::at(int64_t x, int64_t y, int64_t z, int64_t m){
	assert(this->m_shape.size() == 4);
	assert(x >= 0 && x < this->m_range[0].e - this->m_range[0].s);
	assert(y >= 0 && y < this->m_range[1].e - this->m_range[1].s);
	assert(z >= 0 && z < this->m_range[2].e - this->m_range[2].s);
	assert(m >= 0 && m < this->m_range[3].e - this->m_range[3].s);

	int64_t x_p = this->m_range[0].s + x;
	int64_t y_p = this->m_range[1].s + y;
	int64_t z_p = this->m_range[2].s + z;
	int64_t m_p = this->m_range[3].s + m;
	T* data = (T*)(this->cpu());
	return *(data + x_p*this->offset(0) + y_p*this->offset(1) + z_p*this->offset(2) + m_p);
}

template<typename T>
Tensor<T> Tensor<T>::slice(Range x){
	assert(x.s >= this->m_range[0].s && x.e <= this->m_range[0].e);
	Tensor<T> slice_tensor = *this;
	slice_tensor.m_range[0] = x;
	return slice_tensor;
}

template<typename T>
Tensor<T> Tensor<T>::slice(Range x, Range y){
	assert(x.s >= this->m_range[0].s && x.e <= this->m_range[0].e);
	assert(y.s >= this->m_range[1].s && y.e <= this->m_range[1].e);

	Tensor<T> slice_tensor = *this;
	slice_tensor.m_range[0] = x;
	slice_tensor.m_range[1] = y;
	return slice_tensor;
}
template<typename T>
Tensor<T> Tensor<T>::slice(Range x, Range y, Range z){
	assert(x.s >= this->m_range[0].s && x.e <= this->m_range[0].e);
	assert(y.s >= this->m_range[1].s && y.e <= this->m_range[1].e);
	assert(z.s >= this->m_range[2].s && z.e <= this->m_range[2].e);

	Tensor<T> slice_tensor = *this;
	slice_tensor.m_range[0] = x;
	slice_tensor.m_range[1] = y;
	slice_tensor.m_range[2] = z;
	return slice_tensor;
}
template<typename T>
Tensor<T> Tensor<T>::slice(Range x, Range y, Range z, Range m){
	assert(x.s >= this->m_range[0].s && x.e <= this->m_range[0].e);
	assert(y.s >= this->m_range[1].s && y.e <= this->m_range[1].e);
	assert(z.s >= this->m_range[2].s && z.e <= this->m_range[2].e);
	assert(m.s >= this->m_range[3].s && m.e <= this->m_range[3].e);

	Tensor<T> slice_tensor = *this;
	slice_tensor.m_range[0] = x;
	slice_tensor.m_range[1] = y;
	slice_tensor.m_range[2] = z;
	slice_tensor.m_range[3] = m;
	return slice_tensor;
}

template<typename T>
int64_t Tensor<T>::dim(int index){
	return this->m_shape[index];
}

template<typename T>
float Tensor<T>::scale(){
	return this->m_scale;
}
}