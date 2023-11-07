namespace eagleeye
{
template<typename T>
Matrix<T>::Matrix()
	:Blob(0, 0, EAGLEEYE_UNDEFINED, CPU_BUFFER, Aligned(64)){
	m_rows = 0;
	m_cols = 0;
	m_r_range.s = 0;
	m_r_range.e = 0;
	m_c_range.s = 0;
	m_c_range.e = 0;
}

template<typename T>
Matrix<T>::Matrix(int64_t rows,int64_t cols, MemoryType memory_type, Aligned aligned)
	:Blob(rows, cols, TypeTrait<T>::type, memory_type, aligned){
	m_rows = rows;
	m_cols = cols;

	m_r_range.s = 0;
	m_r_range.e = m_rows;
	m_c_range.s = 0;
	m_c_range.e = m_cols;

	if(rows == 0 || cols == 0){
		return;
	}
}

template<typename T>
Matrix<T>::Matrix(std::vector<int64_t> shape, MemoryType memory_type, Aligned aligned)
	:Blob(shape[0], shape[1], TypeTrait<T>::type, memory_type, aligned){
	m_rows = shape[0];
	m_cols = shape[1];

	m_r_range.s = 0;
	m_r_range.e = m_rows;
	m_c_range.s = 0;
	m_c_range.e = m_cols;

	if(m_rows == 0 || m_cols == 0){
		return;
	}
}

template<typename T>
Matrix<T>::Matrix(int64_t rows,int64_t cols,T val, MemoryType memory_type, Aligned aligned)
	:Blob(rows, cols, TypeTrait<T>::type, memory_type, aligned){
	m_rows = rows;
	m_cols = cols;

	m_r_range.s = 0;
	m_r_range.e = m_rows;
	m_c_range.s = 0;
	m_c_range.e = m_cols;

	if(rows == 0 || cols == 0){
		return;
	}

	T* data = (this->cpu<T>());
	int total = rows * cols;
	for (int i = 0; i < total; ++i){
		data[i] = val;
	}
}

template<typename T>
Matrix<T>::Matrix(int64_t rows,int64_t cols,void* data,bool copy_flag, bool manage_flag, MemoryType memory_type, Aligned aligned)
	:Blob(rows, cols, TypeTrait<T>::type, memory_type, aligned, data, copy_flag, manage_flag){
	m_rows = rows;
	m_cols = cols;

	m_r_range.s = 0;
	m_r_range.e = m_rows;
	m_c_range.s = 0;
	m_c_range.e = m_cols;
}

template<typename T>
Matrix<T>::Matrix(unsigned int texture_id)
	:Blob(texture_id){
	if(TypeTrait<T>::type != this->m_data_type){
		this->m_rows = 0;
		this->m_cols = 0;
		m_r_range.s = 0;
		m_r_range.e = m_rows;
		m_c_range.s = 0;
		m_c_range.e = m_cols;

		EAGLEEYE_LOGE("Fail to create matrix by TEXTURE2D.");
		return;
	}

	this->m_rows = this->m_dims[0];
	this->m_cols = this->m_dims[1];

	m_r_range.s = 0;
	m_r_range.e = m_rows;
	m_c_range.s = 0;
	m_c_range.e = m_cols;
}

template<typename T>
T& Matrix<T>::at(unsigned int r_index,unsigned int c_index)
{
	unsigned int real_pos = pos(r_index,c_index);
	return (this->cpu<T>())[real_pos];
}

template<typename T>
const T& Matrix<T>::at(unsigned int r_index,unsigned int c_index) const
{
	unsigned int real_pos = pos(r_index,c_index);
	return (this->cpu<T>())[real_pos];
}

template<typename T>
T& Matrix<T>::at(unsigned int index)
{
	unsigned int real_pos = pos(index);
	return (this->cpu<T>())[real_pos];
}
template<typename T>
const T& Matrix<T>::at(unsigned int index) const
{
	unsigned int real_pos = pos(index);
	return (this->cpu<T>())[real_pos];
}

template<typename T>
T& Matrix<T>::operator() (int r_index,int c_index)
{
	unsigned int real_pos = pos(r_index,c_index);
	return (this->cpu<T>())[real_pos];
}
template<typename T>
const T& Matrix<T>::operator() (int r_index,int c_index) const
{
	unsigned int real_pos = pos(r_index,c_index);
	return (this->cpu<T>())[real_pos];
}

template<typename T>
T& Matrix<T>::operator ()(int index)
{
	unsigned int real_pos = pos(index);
	return (this->cpu<T>())[real_pos];
}

template<typename T>
const T& Matrix<T>::operator ()(int index) const
{
	unsigned int real_pos = pos(index);
	return (this->cpu<T>())[real_pos];
}

template<typename T>
T& Matrix<T>::operator [](int index)
{
	unsigned int real_pos = pos(index);
	return (this->cpu<T>())[real_pos];
}
template<typename T>
const T& Matrix<T>::operator [](int index) const
{
	unsigned int real_pos = pos(index);
	return (this->cpu<T>())[real_pos];
}

template<typename T>
T* Matrix<T>::row(unsigned int r_index)
{	
	unsigned int real_r = pos_r(r_index);
	return this->cpu<T>() + real_r * m_cols + m_c_range.s;
}

template<typename T>
const T* Matrix<T>::row(unsigned int r_index) const
{
	unsigned int real_r = pos_r(r_index);
	return this->cpu<T>() + real_r * m_cols + m_c_range.s;
}

template<typename T>
T* Matrix<T>::anyptr(unsigned int index)
{	
	unsigned int real_r = pos(index);
	return this->cpu<T>() + real_r;
}

template<typename T>
const T* Matrix<T>::anyptr(unsigned int index) const
{
	unsigned int real_r = pos(index);
	return this->cpu<T>() + real_r;
}

template<typename T>
Matrix<T> Matrix<T>::clone() const
{	
	int me_rows = this->rows();
	int me_cols = this->cols();
	Matrix<T> clone_body(me_rows, me_cols);
	for(int i=0; i<me_rows; ++i){
		T* clone_body_ptr = clone_body.row(i);
		memcpy(clone_body_ptr, this->row(i), sizeof(T)*me_cols);
	}
	return clone_body;
}

template<typename T>
Matrix<T> Matrix<T>::operator()(Range r_range,Range c_range)
{
	if(r_range.s < 0 || r_range.e < 0){
		r_range.s = 0;
		r_range.e = this->rows();
	}

	if(c_range.s < 0 || c_range.e < 0){
		c_range.s = 0;
		c_range.e = this->cols();
	}

	Matrix<T> sub_matrix = (*this);
	sub_matrix.m_r_range.s += r_range.s;
	sub_matrix.m_r_range.e = sub_matrix.m_r_range.s + (r_range.e - r_range.s);

	sub_matrix.m_c_range.s += c_range.s;
	sub_matrix.m_c_range.e = sub_matrix.m_c_range.s + (c_range.e - c_range.s);

	return sub_matrix;
}

template<typename T>
const Matrix<T> Matrix<T>::operator ()(Range r_range,Range c_range) const
{
	if(r_range.s < 0 || r_range.e < 0){
		r_range.s = 0;
		r_range.e = this->rows();
	}

	if(c_range.s < 0 || c_range.e < 0){
		c_range.s = 0;
		c_range.e = this->cols();
	}

	Matrix<T> sub_matrix = (*this);
	sub_matrix.m_r_range.s += r_range.s;
	sub_matrix.m_r_range.e = sub_matrix.m_r_range.s + (r_range.e - r_range.s);

	sub_matrix.m_c_range.s += c_range.s;
	sub_matrix.m_c_range.e = sub_matrix.m_c_range.s + (c_range.e - c_range.s);


	return sub_matrix;
}

template<typename T>
bool Matrix<T>::isfull() const 
{
	if ((m_r_range.e - m_r_range.s) != m_rows)
	{
		return false;
	}
	if ((m_c_range.e - m_c_range.s) != m_cols)
	{
		return false;
	}
	return true;
}

template<typename T>
bool Matrix<T>::isContinuous() const
{
	if ((m_r_range.e - m_r_range.s) != m_rows)
	{
		return false;
	}
	if ((m_c_range.e - m_c_range.s) != m_cols)
	{
		return false;
	}
	return true;
}

template<typename T>
unsigned int Matrix<T>::pos(unsigned int r_index,unsigned int c_index) const
{
	unsigned int real_r_index = pos_r(r_index);
	unsigned int real_c_index = pos_c(c_index);
	
	return real_r_index * m_cols + real_c_index;
}

template<typename T>
unsigned int Matrix<T>::pos_r(unsigned int r_index) const
{
	return r_index + m_r_range.s;
}

template<typename T>
unsigned int Matrix<T>::pos_c(unsigned int c_index) const
{
	return c_index + m_c_range.s;
}

template<typename T>
unsigned int Matrix<T>::pos(unsigned int index) const
{
	unsigned int r_index = index / (m_c_range.e - m_c_range.s);
	unsigned int c_index = index % (m_c_range.e - m_c_range.s);
	return pos(r_index,c_index);
}

template<typename T>
unsigned int Matrix<T>::rows() const
{
	return m_r_range.e - m_r_range.s;
}

template<typename T>
unsigned int Matrix<T>::cols() const
{
	return m_c_range.e - m_c_range.s;
}

template<typename T>
unsigned int Matrix<T>::size() const
{
	return (m_r_range.e - m_r_range.s) * (m_c_range.e - m_c_range.s);
}

template<typename T>
Matrix<T> Matrix<T>::t()
{
	unsigned int requested_rows = m_r_range.e - m_r_range.s;
	unsigned int requested_cols = m_c_range.e - m_c_range.s;

	unsigned int t_requested_rows = requested_cols;
	unsigned int t_requested_cols = requested_rows;

	Matrix<T> t_m(t_requested_rows,t_requested_cols,T(0));
	T* t_data = t_m.dataptr();

	for (unsigned int i = 0; i < requested_rows; ++i)
	{
		T* my_ptr = row(i);
		for (unsigned int j = 0; j < requested_cols; ++j)
		{
			t_data[j * requested_rows + i] = my_ptr[j];
		}
	}

	return t_m;
}

template<typename T>
T* Matrix<T>::dataptr()
{
	return this->cpu<T>();
}

template<typename T>
const T* Matrix<T>::dataptr() const
{
	return this->cpu<T>();
}

template<typename T>
void Matrix<T>::setzeros()
{
	unsigned int t_rows = m_r_range.e - m_r_range.s;
	unsigned int t_cols = m_c_range.e - m_c_range.s;

	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* r_ptr = row(i);
		memset(r_ptr,0,sizeof(T) * t_cols);
	}
}

template<typename T>
void Matrix<T>::setval(T val)
{
	unsigned int t_rows = m_r_range.e - m_r_range.s;
	unsigned int t_cols = m_c_range.e - m_c_range.s;

	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* r_ptr = row(i);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			r_ptr[j] = val;
		}
	}
}

template<typename T>
void Matrix<T>::copy(const Matrix<T>& src)
{
	assert(rows() <= src.rows());
	assert(cols() <= src.cols());

	int me_rows = rows();
	int me_cols = cols();

	for (int i = 0; i < me_rows; ++i)
	{
		const T* src_data = src.row(i);
		T* me_data = row(i);
		memcpy(me_data,src_data,sizeof(T) * me_cols);
	}
}

template<typename T>
Matrix<T> Matrix<T>::reshape(int r,int c) const
{
	int now_r = rows();
	int now_c = cols();

	assert(r * c == now_r * now_c);
	
	Matrix one(r,c);
	T* one_data = one.dataptr();

	for (int i = 0; i < now_r; ++i)
	{
		const T* data = row(i);
		memcpy(one_data,data,sizeof(T) * now_c);
		one_data = one_data + now_c;
	}
	return one;
}

template<typename T>
Matrix<T> Matrix<T>::reshape_(int r,int c)
{
	int now_r = rows();
	int now_c = cols();
	assert(r * c == now_r * now_c);

	if(!isContinuous()){
		*this = this->clone();
	}
	m_rows = r;
	m_cols = c;

	m_r_range.s = 0;
	m_r_range.e = m_rows;
	m_c_range.s = 0;
	m_c_range.e = m_cols;
	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::mul(const Matrix<T>& m) const
{
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();

	unsigned int r_rows = m.rows();
	unsigned int r_cols = m.cols();

	unsigned int mul_rows = t_rows > r_rows ? t_rows : r_rows;
	unsigned int mul_cols = t_cols > r_cols ? t_cols : r_cols;

	Matrix<T> product(mul_rows,mul_cols);
	for (unsigned int i = 0; i < mul_rows; ++i)
	{
		T* product_ptr = product.row(i);
		const T* left_ptr = row(i%t_rows);
		const T* right_ptr = m.row(i%r_rows);
		for (unsigned int j = 0; j < mul_cols; ++j)
		{
			product_ptr[j] = left_ptr[j%t_cols] * right_ptr[j%r_cols];
		}
	}

	return product;
}

template<typename T>
Matrix<T> Matrix<T>::mul_(const Matrix<T>& m)
{
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();

	unsigned int r_rows = m.rows();
	unsigned int r_cols = m.cols();

	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		const T* right_ptr = m.row(i%r_rows);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] *= right_ptr[j%r_cols];
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::div(const Matrix<T>& m) const
{
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();

	unsigned int r_rows = m.rows();
	unsigned int r_cols = m.cols();

	unsigned int div_rows = t_rows > r_rows ? t_rows : r_rows;
	unsigned int div_cols = t_cols > r_cols ? t_cols : r_cols;

	Matrix<T> result(t_rows,t_cols);
	for (unsigned int i = 0; i < div_rows; ++i)
	{
		T* result_ptr = result.row(i);
		const T* left_ptr = row(i%t_rows);
		const T* right_ptr = m.row(i%r_rows);
		for (unsigned int j = 0; j < div_cols; ++j)
		{
			result_ptr[j] = left_ptr[j%t_cols] / (right_ptr[j%r_cols] + eagleeye_eps);
		}
	}

	return result;
}

template<typename T>
Matrix<T> Matrix<T>::div_(const Matrix<T>& m)
{
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();

	unsigned int r_rows = m.rows();
	unsigned int r_cols = m.cols();

	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		const T* right_ptr = m.row(i%r_rows);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] /= (right_ptr[j%r_cols] + eagleeye_eps);
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::sub_(T v){
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();
	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] -= v;
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::add_(T v){
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();
	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] += v;
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::mul_(T v){
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();
	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] *= v;
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::div_(T v){
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();
	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] /= v;
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::sub_(const Matrix<T>& v){
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();

	unsigned int r_rows = v.rows();
	unsigned int r_cols = v.cols();

	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		const T* v_ptr = v.row(i%r_rows);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] -= v_ptr[j%r_cols];
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::add_(const Matrix<T>& v){
	unsigned int t_rows = rows();
	unsigned int t_cols = cols();

	unsigned int r_rows = v.rows();
	unsigned int r_cols = v.cols();

	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		const T* v_ptr = v.row(i%r_rows);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] += v_ptr[j%r_cols];
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::flatten() const
{
	int me_rows = this->rows();
	int me_cols = this->cols();
	Matrix<T> flatten_mat(1, me_rows*me_cols);
	T* 	flatten_mat_ptr = flatten_mat.dataptr();
	for(int i=0; i<me_rows; ++i){
		memcpy(flatten_mat_ptr, this->row(i), sizeof(T)*me_cols);
		flatten_mat_ptr = flatten_mat_ptr + me_cols;
	}

	return flatten_mat;
}

template<typename T>
Matrix<T> Matrix<T>::operator+(const Matrix<T>& m) const
{
	unsigned int t_rows = m_r_range.e - m_r_range.s;
	unsigned int t_cols = m_c_range.e - m_c_range.s;

	unsigned int r_rows = m.rows();
	unsigned int r_cols = m.cols();

	unsigned int add_rows = t_rows > r_rows ? t_rows : r_rows;
	unsigned int add_cols = t_cols > r_cols ? t_cols : r_cols;

	Matrix<T> result(add_rows,add_cols);
	for (unsigned int i = 0; i < add_rows; ++i)
	{	
		T* result_ptr = result.row(i);
		const T* left_ptr = row(i%t_rows);
		const T* right_ptr = m.row(i%r_rows);
		for (unsigned int j = 0; j < add_cols; ++j)
		{
			result_ptr[j] = left_ptr[j%t_cols] + right_ptr[j%r_cols];
		}
	}
	return result;
}

template<typename T>
Matrix<T>& Matrix<T>::operator+=(const Matrix<T>& m)
{
	unsigned int t_rows = m_r_range.e - m_r_range.s;
	unsigned int t_cols = m_c_range.e - m_c_range.s;

	unsigned int r_rows = m.rows();
	unsigned int r_cols = m.cols();

	for (unsigned int i = 0; i < t_rows; ++i)
	{
		T* left_ptr = row(i);
		const T* right_ptr = m.row(i%r_rows);
		for (unsigned int j = 0; j < t_cols; ++j)
		{
			left_ptr[j] += right_ptr[j%r_cols];
		}
	}

	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::operator-(const Matrix<T>& m) const
{
	unsigned int rows = m_r_range.e - m_r_range.s;
	unsigned int cols = m_c_range.e - m_c_range.s;

	unsigned int r_rows = m.rows();
	unsigned int r_cols = m.cols();

	unsigned int sub_rows = rows > r_rows ? rows : r_rows;
	unsigned int sub_cols = cols > r_cols ? cols : r_cols;

	Matrix<T> result(sub_rows,sub_cols);
	for (unsigned int i = 0; i < sub_rows; ++i)
	{
		const T* data_ptr = row(i%rows);
		const T* data_m_ptr = m.row(i%r_rows);
		T* data_result_ptr = result.row(i);
		for (unsigned int j = 0; j < sub_cols; ++j)
		{
			data_result_ptr[j] = data_ptr[j%cols] - data_m_ptr[j%r_cols];
		}
	}

	return result;
}

template<typename T>
Matrix<T>& Matrix<T>::operator -=(const Matrix<T>& m)
{
	unsigned int rows = m_r_range.e - m_r_range.s;
	unsigned int cols = m_c_range.e - m_c_range.s;

	unsigned int r_rows = m.rows();
	unsigned int r_cols = m.cols();

	for (unsigned int i = 0; i < rows; ++i)
	{
		T* data_ptr = row(i);
		const T* data_m_ptr = m.row(i%r_rows);

		for (unsigned int j = 0; j < cols; ++j)
		{
			data_ptr[j] -= data_m_ptr[j%r_cols];
		}
	}
	return *this;
}

template<typename T>
Matrix<T> Matrix<T>::operator*(const Matrix<T>& r)
{	
	unsigned int l_rows = rows();
	unsigned int l_cols = cols();

	unsigned int r_rows = r.rows();
	unsigned int r_cols = r.cols();
	assert(l_cols == r_rows);

	Matrix<T> result(l_rows,r_cols,T(0));
	T* data = result.dataptr();

	for (unsigned int left_i = 0; left_i < l_rows; ++left_i)
	{
		T* l_r_ptr = row(left_i);

		for (unsigned int right_j = 0; right_j < r_cols; ++right_j)
		{
			for (unsigned int left_j = 0; left_j < l_cols; ++left_j)
			{
				const T* r_r_ptr = r.row(left_j);
				data[left_i * r_cols + right_j] += l_r_ptr[left_j] * r_r_ptr[right_j];
			}

		}
	}

	return result;
}

template<typename T>
bool Matrix<T>::isempty() const
{
	if (m_rows == 0 || m_cols == 0)
		return true;
	else
		return false;
}

template<typename T>
unsigned int Matrix<T>::stride() const
{
	return m_cols;
}

template<typename T>
void Matrix<T>::offset(unsigned int& offset_r,unsigned int& offset_c) const
{
	offset_r = m_r_range.s;
	offset_c = m_c_range.s;
}

template<typename T>
void Matrix<T>::layout(unsigned int& offset_r, 
						unsigned int& offset_c, 
						unsigned int& h, 
						unsigned int& w) const
{
	h = m_rows;
	w = m_cols;

	offset_r = m_r_range.s;
	offset_c = m_c_range.s;
}


template<typename T>
void Matrix<T>::fliplr()
{
	int rs = rows();
	int cs = cols();
	
	int half_cs = cs / 2;

	for (int i = 0; i < rs; ++i)
	{
		T* data = row(i);
		for (int j = 0; j < half_cs; ++j)
		{
			T val = data[j];
			data[j] = data[cs - 1 - j];
			data[cs - 1 - j] = val;
		}
	}
}

template<typename T>
void Matrix<T>::flipud()
{
	int rs = rows();
	int cs = cols();

	int half_rs = rs / 2;
	for (int i = 0; i < half_rs; ++i)
	{
		T* up_data = row(i);
		T* down_data = row(rs - 1 - i);
		for (int j = 0; j < cs; ++j)
		{
			T val = up_data[j];
			up_data[j] = down_data[j];
			down_data[j] = val;
		}
	}
}

template<typename T>
Matrix<T> Matrix<T>::select(const std::vector<unsigned int>& index, unsigned int offset) const{
	unsigned int rows = index.size() - offset;
	unsigned int cols = this->cols();

	Matrix<T> ss(rows, cols);
	for(unsigned int r = offset; r < index.size(); ++r){
		T* ss_ptr = ss.row(r-offset);
		const T* data_ptr = this->row(index[r]);
		memcpy(ss_ptr, data_ptr, sizeof(T)*cols);
	}

	return ss;
}

template<typename T>
Matrix<T> Matrix<T>::gatherND(const Matrix<int>& indices, int group) const{
	int gather_rows = indices.rows();	
	if(indices.cols() == 1){
		// ignore group
		return this->gather(indices, 0);
	}
	else{
		Matrix<T> gather_mat(gather_rows, group);
		for(int i=0; i<gather_rows; ++i){
			const int* indices_ptr = indices.row(i);
			int row_i = indices_ptr[0];
			int col_i = indices_ptr[1];
			gather_mat(Range(i,i+1),Range(0,group)).copy((*this)(Range(row_i, row_i+1),Range(col_i*group, (col_i + 1)*group)));
		}

		return gather_mat;
	}
}

template<typename T>
Matrix<T> Matrix<T>::gather(const Matrix<int>& indices, int axis) const{
	assert(indices.rows() == 1 || indices.cols() == 1);
	
	if(axis == 0){
		// gather rows from indices
		int gather_rows = indices.rows()*indices.cols();
		int gather_cols = this->cols();
		Matrix<T> gather_data(gather_rows, gather_cols);
		for(int i=0; i<gather_rows; ++i){
			T* gather_data_ptr = gather_data.row(i);
			const T* data_ptr = row(indices.at(i));
			memcpy(gather_data_ptr, data_ptr, sizeof(T)*gather_cols);
		}

		return gather_data;
	}
	else{
		// gather cols from indices
		int gather_rows = this->rows();
		int gather_cols = indices.rows()*indices.cols();
		Matrix<T> gather_data(gather_rows, gather_cols);
		for(int i=0; i<gather_rows; ++i){
			const T* data_ptr = row(i);
			T* gather_data_ptr = gather_data.row(i);
			for(int j=0; j<gather_cols; ++j){
				gather_data_ptr[j] = data_ptr[indices.at(j)];
			}
		}

		return gather_data;
	}
}

template<typename T>
T Matrix<T>::determinant(){
	int me_rows = this->rows();
	int me_cols = this->cols();
	assert(me_rows == me_cols);
	assert(me_rows == 2 || me_rows == 3);

	T det = 0;
    switch(me_rows){
		case 2:{
			det = this->at(0,0) * this->at(1,1) - this->at(0,1) * this->at(1,0);
            return det;
        }
        break;
        case 3:{
            /***

            a b c

            d e f

            g h i


            a b c a b c

            d e f d e f

            g h i g h i

            // det (A) = aei + bfg + cdh - afh - bdi - ceg.
            ***/
			T a = this->at(0,0);
			T b = this->at(0,1);
			T c = this->at(0,2);
			T d = this->at(1,0);
			T e = this->at(1,1);
			T f = this->at(1,2);
			T g = this->at(2,0);
			T h = this->at(2,1);
			T i = this->at(2,2);
            T det = (a*e*i + b*f*g + c*d*h); // - a*f*h - b*d*i - c*e*g);

            det = det - a*f*h;
            det = det - b*d*i;
            det = det - c*e*g;
            return det;
        }
        break;
	}

	return T(det);
}

template<typename T>
Matrix<T> Matrix<T>::cofactor(){
	int me_rows = this->rows();
	int me_cols = this->cols();
	Matrix<T> cf = Matrix<T>(me_rows, me_cols);
    if(me_rows != me_cols)
		return cf;

    if(me_rows < 2)
		return cf;
    else if(me_rows == 2){
		cf.at(0,0) = this->at(1,1);
		cf.at(0,1) = -this->at(1,0);
		cf.at(1,0) = -this->at(0,1);
		cf.at(1,1) = this->at(0,0);
        return cf;
    }
	else if(me_rows >= 3){
		int DIM = me_rows;
		std::vector<std::vector<Matrix<T>>> temp;
		temp.resize(DIM);
		for(int i=0; i<DIM; i++){
			temp[i].resize(DIM);
		}

		for(int i = 0; i < DIM; i++)
			for(int j = 0; j < DIM; j++)
				temp[i][j] = Matrix<T>(DIM-1, DIM-1);

		for(int k1 = 0; k1 < DIM; k1++){  
			for(int k2 = 0; k2 < DIM; k2++){
				int i1 = 0;
				for(int i = 0; i < DIM; i++){
					int j1 = 0;
					for(int j = 0; j < DIM; j++){
						if(k1 == i || k2 == j)
							continue;
						temp[k1][k2].at(i1,j1++) = this->at(i,j);
					}
					if(k1 != i)
						i1++;
				}
			}
		}

		bool flagPositive = true;
		for(int k1 = 0; k1 < DIM; k1++){  
			flagPositive = ((k1 % 2) == 0);
			for(int k2 = 0; k2 < DIM; k2++){
				if(flagPositive == true){
					cf.at(k1,k2) = temp[k1][k2].determinant();
					flagPositive = false;
				}
				else{
					cf.at(k1,k2) = -temp[k1][k2].determinant();
					flagPositive = true;
				}
			}
		}
	}

	return cf;
}

template<typename T>
Matrix<T> Matrix<T>::inv(){
	int me_rows = this->rows();
	int me_cols = this->cols();
	Matrix<T> cf(me_rows, me_cols);
	Matrix<T> inv_mat(me_rows, me_cols);
    if(me_rows != me_cols)
		return inv_mat;

    // to find out Determinant
    T det = this->determinant();
	cf = this->cofactor();
	// inv = transpose of cofactor / Determinant
	for(int i = 0; i < me_rows; i++){
		for(int j = 0; j < me_cols; j++){
			inv_mat.at(j,i) = cf.at(i,j)/(det + eagleeye_eps);
		}

	}
	return inv_mat;
}
}
