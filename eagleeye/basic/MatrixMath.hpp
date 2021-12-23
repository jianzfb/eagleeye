namespace eagleeye{
template<class CompareT>
std::vector<unsigned int> sort(const Matrix<typename CompareT::ElementType>& data){
	typedef typename CompareT::ElementType		ElementType;

	assert(data.rows() == 1 || data.cols() == 1);

	unsigned int r_num = data.rows();
	unsigned int c_num = data.cols();

	std::vector<SortElement<ElementType>> temp_vector(r_num * c_num);
	unsigned int index = 0;
	for (unsigned int i = 0; i < r_num; ++i)
	{
		const ElementType* d_ptr = data.row(i);
		for (unsigned int j = 0; j < c_num; ++j)
		{
			temp_vector[index] = SortElement<ElementType>(d_ptr[j],index);
			index++;
		}
	}

	//sort
	CompareT compare_op;
	std::sort(temp_vector.begin(),temp_vector.end(),compare_op);

	std::vector<unsigned int> result_index(r_num * c_num);
	index = 0;
	typename std::vector<SortElement<ElementType>>::iterator iter,iend(temp_vector.end());
	for (iter = temp_vector.begin(); iter != iend; ++iter)
	{
		result_index[index] = ((*iter).index);
		index++;
	}

	return result_index;
}


template<typename CompareT>
Matrix<int> where(const Matrix<typename CompareT::ElementType>& data, typename CompareT::ElementType t){
	typedef typename CompareT::ElementType		ElementType;

	//	(i0,j0)
	//	(i1,j1)

	unsigned int r_num = data.rows();
	unsigned int c_num = data.cols();
	// Matrix<int> index(r_num, c_num);

	std::vector<int> i_list;
	std::vector<int> j_list; 
	CompareT jj;
	for(unsigned int r = 0; r< r_num; ++r){
		const ElementType* data_ptr = data.row(r);
		for(unsigned int c=0; c<c_num; ++c){
			if(jj(data_ptr[c], t)){
				i_list.push_back(r);
				j_list.push_back(c);
			}
		}
	}

	int ok_num = i_list.size();
	Matrix<int> index(ok_num, 2);
	for(unsigned int i=0; i<ok_num; ++i){
		int* ptr = index.row(i);
		ptr[0] = i_list[i];
		ptr[1] = j_list[i];
	}

	return index;
}

template<typename CompareT, typename OutputT>
Matrix<OutputT> boolean(const Matrix<typename CompareT::ElementType>& data, typename CompareT::ElementType t){
	typedef typename CompareT::ElementType		ElementType;

	unsigned int r_num = data.rows();
	unsigned int c_num = data.cols();
	Matrix<OutputT> index(r_num, c_num);

	CompareT jj;
	for(unsigned int r = 0; r< r_num; ++r){
		const ElementType* data_ptr = data.row(r);
		for(unsigned int c=0; c<c_num; ++c){
			if(jj(data_ptr[c], t)){
				index.at(r,c) = 1;
			}
			else{
				index.at(r,c) = 0;	
			}
		}
	}

	return index;

}

template<typename T>
void meshgrid(const Matrix<T>& x, const Matrix<T>& y, Matrix<T>& xg, Matrix<T>& yg){
	assert(x.isContinuous() && y.isContinuous());
	unsigned int x_size = x.rows()*x.cols();
	unsigned int y_size = y.rows()*y.cols();

	xg=Matrix<T>(y_size, x_size);
	yg=Matrix<T>(y_size, x_size);


	for(unsigned int y_i=0; y_i<y_size; ++y_i){
		for(unsigned int x_i=0; x_i<x_size; ++x_i){
			xg.at(y_i,x_i) = x.at(x_i);
			yg.at(y_i,x_i) = y.at(y_i);
		}
	}
}

template<typename T>
Matrix<T> arange(int s, int e, int stride, int axis){
	int rows = 1;
	int cols = (e-s) / stride;
	if(axis == 0){
		rows = (e-s) / stride;
		cols = 1;
	}

	Matrix<T> d(rows, cols);
	T* d_ptr = d.row(0);

	int col_i = 0;
	for(int i=s; i<e; i+=stride){
		d_ptr[col_i] = i;
		col_i += 1;
	}
	return d;
}

template<typename T>
Matrix<T> norm(const Matrix<T>& x, int mode){
	// now, only support l2 norm
	assert(mode == 0);

	int rows = x.rows();
	int cols = x.cols();
	T epsilon=1e-12;
	Matrix<T> y(rows, cols);

	for(int i = 0; i < rows; ++i){
		const T* row_ptr = x.row(i);

		T denominator = 0.0;
		for(int j=0; j<cols; ++j){
			denominator += row_ptr[j] * row_ptr[j];
		}
		denominator = sqrt(denominator>epsilon?denominator:epsilon);

		T* y_row_ptr = y.row(i);
		for(int j=0; j<cols; ++j){
			y_row_ptr[j] = row_ptr[j] / denominator;
		}
	}

	return y;
}

template<typename T>
Matrix<T> resize(const Matrix<T> img, float scale, InterpMethod interp_method){
	int rows = img.rows();
	int cols = img.cols();

	int after_rows = int(ceil(rows * scale));
	int after_cols = int(ceil(cols * scale));

	return resize(img,after_rows,after_cols,interp_method);
}

template<typename T>
Matrix<T> resize(const Matrix<T> img, int after_r, int after_c, InterpMethod interp_method){
	int rows = img.rows();
	int cols = img.cols();
	Matrix<T> after_img(after_r,after_c);
	
	switch(interp_method)
	{
	case BILINEAR_INTERPOLATION:
		{
			for (int i = 0; i < after_r; ++i)
			{
				T* after_img_data = after_img.row(i);
				for (int j = 0; j < after_c; ++j)
				{
					float r_f = (float(i) / float(after_r)) * float(rows);
					float c_f = (float(j) / float(after_c)) * float(cols);

					int r = int(floor(r_f));
					int c = int(floor(c_f));

					float u = r_f - r;
					float v = c_f - c;

					int first_r_index = eagleeye_min(r,(rows - 1));
					int first_c_index = eagleeye_min(c,(cols - 1));
					T first_point = img.at(first_r_index,first_c_index);

					int second_r_index = first_r_index;
					int second_c_index = eagleeye_min((c + 1),(cols - 1));
					T second_point = img.at(second_r_index,second_c_index);

					int third_r_index = eagleeye_min((r + 1),(rows - 1));
					int third_c_index = first_c_index;
					T third_point = img.at(third_r_index,third_c_index);

					int fourth_r_index = third_r_index;
					int fourth_c_index = second_c_index;
					T fourth_point = img.at(fourth_r_index,fourth_c_index);

					float first_weight = (1 - u) * (1 - v);
					float second_weight = (1 - u) * v;
					float third_weight = u * (1 - v);
					float fourth_weight = u * v;

					after_img_data[j] = T(first_point * first_weight + 
						second_point * second_weight + 
						third_point * third_weight + 
						fourth_point * fourth_weight);
				}
			}
			break;
		}
	case NEAREST_NEIGHBOR_INTERPOLATION:
		{
			for (int i = 0; i < after_r; ++i)
			{
				T* after_img_data = after_img.row(i);
				for (int j = 0; j < after_c; ++j)
				{
					float r_f = (float(i) / float(after_r)) * float(rows);
					float c_f = (float(j) / float(after_c)) * float(cols);

					int r = int(floor(r_f));
					int c = int(floor(c_f));

					after_img_data[j] = img(r,c);
				}
			}
			break;
		}
	}

	return after_img;
}

template<typename T>
Matrix<int> argmax(const Matrix<T>& x, int axis){
	int rows = x.rows();
	int cols = x.cols();
	if(axis==0){
		Matrix<T> max_x(1, cols);
		Matrix<int> argmax_index(1, cols, (int)0);
		int* argmax_index_ptr = argmax_index.row(0);

		T* max_x_ptr = max_x.row(0);
		memcpy(max_x_ptr, x.row(0), sizeof(T)*cols);

		for(int i=1; i<rows; ++i){
			const T* x_ptr = x.row(i);
			for(int j=0; j<cols; ++j){
				if(max_x_ptr[j] < x_ptr[j]){
					argmax_index_ptr[j] = i;
					max_x_ptr[j] = x_ptr[j];
				}
			}
		}
		return argmax_index;
	}
	else if(axis==1){
		Matrix<T> max_x(rows, 1);
		Matrix<int> argmax_index(rows, 1, (int)0);
		int* argmax_index_ptr = argmax_index.row(0);

		T* max_x_ptr = max_x.row(0);
		for(int i=0; i<rows; ++i){
			max_x_ptr[i] = x.at(i,0);
		}

		for(int i=0; i<rows; ++i){
			const T* x_ptr = x.row(i);
			for(int j=1; j<cols; ++j){
				if(max_x_ptr[i] < x_ptr[j]){
					argmax_index_ptr[i] = j;
					max_x_ptr[i] = x_ptr[j];
				}
			}
		}
		return argmax_index;
	}
	else{
		return Matrix<int>();
	}
}

template<typename T>
std::vector<Matrix<T>> split(const Matrix<T> data, int num, int axis){
	if(axis == 0){
		assert(data.rows() % num == 0);
	}
	else{
		assert(data.cols() % num == 0);
	}
	int rows = data.rows();
	int cols = data.cols();

	std::vector<Matrix<T>> terms(num);
	if(axis == 0){
		int size = data.rows() / num;
		for(int i=0; i<num; ++i){
			terms[i] = data(Range(i*size, (i+1)*size),Range(0, cols));
		}
	}
	else{
		int size = data.cols() / num;
		for(int i=0; i<num; ++i){
			Matrix<T> s(rows, size);
			s.copy(data(Range(0, rows), Range(i*size, (i+1)*size)));
			terms[i] = s;
		}
	}

	return terms;
}

template<typename T>
Matrix<T> mmax(const Matrix<T>& b, T a, std::vector<unsigned int> order, int offset){
	unsigned int rows = b.rows();
	unsigned int cols = b.cols();

	Matrix<T> mm(order.size() - offset, cols);
	for (unsigned int i = offset; i < order.size(); ++i) {
		const T* b_ptr = b.row(order[i]);
		T* mm_ptr = mm.row(i - offset);
		for (unsigned int c = 0; c < cols; ++c) {
			mm_ptr[c] = a > b_ptr[c] ? a : b_ptr[c];
		}
	}

	return mm;
}

template<typename T>
Matrix<T> mmax_(Matrix<T>& b, T a, std::vector<unsigned int> order, int offset){
	unsigned int rows = b.rows();
	unsigned int cols = b.cols();

	Matrix<T> b_ = b(Range(0, order.size() - offset), Range(0, cols));
	for (unsigned int i = offset; i < order.size(); ++i) {
		const T* b_ptr = b.row(order[i]);
		T* bb_ptr = b_.row(i - offset);
		for (unsigned int c = 0; c < cols; ++c) {
			bb_ptr[c] = a > b_ptr[c] ? a : b_ptr[c];
		}
	}

	return b_;
}

template<typename T>
Matrix<T> mmax(const Matrix<T>& b, T a){
	unsigned int rows = b.rows();
	unsigned int cols = b.cols();

	Matrix<T> mm(rows, cols);
	for (unsigned int r = 0; r < rows; ++r) {
		const T* b_ptr = b.row(r);
		T* mm_ptr = mm.row(r);
		for (unsigned int c = 0; c < cols; ++c) {
			mm_ptr[c] = a > b_ptr[c] ? a : b_ptr[c];
		}
	}

	return mm;
}

template<typename T>
Matrix<T> mmax_(Matrix<T>& b, T a){
	unsigned int rows = b.rows();
	unsigned int cols = b.cols();

	for (unsigned int r = 0; r < rows; ++r) {
		T* b_ptr = b.row(r);
		for (unsigned int c = 0; c < cols; ++c) {
			b_ptr[c] = a > b_ptr[c] ? a : b_ptr[c];
		}
	}

	return b;
}

template<typename T>
Matrix<T> mmin(const Matrix<T>& b, T a, std::vector<unsigned int> order, int offset){
	unsigned int rows = b.rows();
	unsigned int cols = b.cols();

	Matrix<T> mm(order.size() - offset, cols);
	for (unsigned int i = offset; i < order.size(); ++i) {
		const T* b_ptr = b.row(order[i]);
		T* mm_ptr = mm.row(i - offset);
		for (unsigned int c = 0; c < cols; ++c) {
			mm_ptr[c] = a < b_ptr[c] ? a : b_ptr[c];
		}
	}

	return mm;
}

template<typename T>
Matrix<T> mmin_(Matrix<T>& b, T a, std::vector<unsigned int> order, int offset){
	unsigned int rows = b.rows();
	unsigned int cols = b.cols();

	Matrix<T> b_ = b(Range(0, order.size() - offset), Range(0, cols));
	for (unsigned int i = offset; i < order.size(); ++i) {
		const T* b_ptr = b.row(order[i]);
		T* bb_ptr = b_.row(i - offset);
		for (unsigned int c = 0; c < cols; ++c) {
			bb_ptr[c] = a < b_ptr[c] ? a : b_ptr[c];
		}
	}

	return b_;
}

template<typename T>
Matrix<T> mmin(const Matrix<T>& b, T a){
	unsigned int rows = b.rows();
	unsigned int cols = b.cols();

	Matrix<T> mm(rows, cols);
	for (unsigned int r = 0; r < rows; ++r) {
		const T* b_ptr = b.row(r);
		T* mm_ptr = mm.row(r);
		for (unsigned int c = 0; c < cols; ++c) {
			mm_ptr[c] = a < b_ptr[c] ? a : b_ptr[c];
		}
	}

	return mm;
}

template<typename T>
Matrix<T> mmin_(Matrix<T>& b, T a){
	unsigned int rows = b.rows();
	unsigned int cols = b.cols();

	for (unsigned int r = 0; r < rows; ++r) {
		T* b_ptr = b.row(r);
		for (unsigned int c = 0; c < cols; ++c) {
			b_ptr[c] = a < b_ptr[c] ? a : b_ptr[c];
		}
	}

	return b;
}
}