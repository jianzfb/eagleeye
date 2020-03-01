namespace eagleeye{
template<typename T>
Tensor<T> concat(std::vector<Tensor<T>> tensors, int axis){
	assert(axis==0);
	int num = tensors.size();
	if(num == 0){
		return Tensor<T>();
	}

	std::vector<int64_t> shape = tensors[0].shape();
	shape[axis] = num * shape[axis];

	Tensor<T> concat_tensor(shape);
	T* data = concat_tensor.dataptr();

	int offset = 0;
	for(int i=0; i<num; ++i){
		offset  = i*tensors[i].size();
		memcpy(data+offset, tensors[i].dataptr(), sizeof(T)*tensors[i].size());
	}

	return concat_tensor;
}	
}