namespace eagleeye
{
/* Image Signal */
template<class T>
ImageSignal<T>::ImageSignal(Matrix<T> data,char* name,char* info)
				:BaseImageSignal(TypeTrait<T>::type,TypeTrait<T>::size,info),
				img(data)
{
	this->m_meta.name = name;
	this->m_meta.info = info;
	this->m_meta.fps = 0.0;
	this->m_meta.nb_frames = 0;
	this->m_meta.frame = 0;
	this->m_meta.is_end_frame = false;
	this->m_meta.is_start_frame = false;
	this->m_meta.rotation = 0;
	this->m_meta.needed_rows = 0;
	this->m_meta.needed_cols = 0;	
	m_sig_category = SIGNAL_CATEGORY_IMAGE;
}

template<class T>
void ImageSignal<T>::copyInfo(AnySignal* sig){
	//call the base class
	BaseImageSignal::copyInfo(sig);

	//receive some info from the upper signal
	if(SIGNAL_CATEGORY_IMAGE == (sig->getSignalCategory() & SIGNAL_CATEGORY_IMAGE)){
		ImageSignal<T>* from_sig = (ImageSignal<T>*)(sig);	
		if (from_sig){
			this->m_meta = *(from_sig->meta());
		}
	}
}

template<class T>
void ImageSignal<T>::copy(AnySignal* sig){
	ImageSignal<T>* from_sig = (ImageSignal<T>*)(sig);	
	this->setData(from_sig->getData());
}

template<class T>
void ImageSignal<T>::printUnit()
{
	Superclass::printUnit();

	EAGLEEYE_LOGD("image name %s \n",this->meta()->name.c_str());
	EAGLEEYE_LOGD("image type %d channels %d rows %d cols %d \n",pixel_type,channels,img.rows(),img.cols());
}

template<class T>
void ImageSignal<T>::makeempty(bool auto_empty)
{
	if(auto_empty){
		if(this->m_release_count % this->getOutDegree() != 0){
			this->m_release_count += 1;
			return;
		}
	}

	img = Matrix<T>();
	this->m_meta.name = "";
	this->m_meta.info = "";
	this->m_meta.fps = 0.0;
	this->m_meta.nb_frames = 0;
	this->m_meta.frame = 0;
	this->m_meta.is_end_frame = false;
	this->m_meta.is_start_frame = false;
	this->m_meta.rotation = 0;
	this->m_meta.needed_rows = 0;
	this->m_meta.needed_cols = 0;	

	if(auto_empty){
		// reset count
		this->m_release_count = 1;
	}

	//force time update
	modified();
}

template<class T>
bool ImageSignal<T>::isempty(){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_IMAGE){
		if(img.rows() == 0 || img.cols() == 0){
			return true;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
}

template<class T>
typename ImageSignal<T>::DataType ImageSignal<T>::getData(){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_IMAGE){
		// SIGNAL_CATEGORY_IMAGE
		// refresh data
		if(this->m_delay_time > 0){
			this->m_link_node->refresh();
		}
		// return img
		return img;
	}
	else{
		// SIGNAL_CATEGORY_IMAGE_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		while(this->m_queue.size() == 0){
            this->m_cond.wait(locker);

			if(this->m_queue.size() > 0){
				break;
			}
        }
		this->img = this->m_queue.front();
        this->m_queue.pop();
        locker.unlock();
		return this->img;
	}
}

template<class T>
void ImageSignal<T>::setData(ImageSignal<T>::DataType data){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_IMAGE){
		// SIGNAL_CATEGORY_IMAGE
		this->img = data;
	}
	else{
		// SIGNAL_CATEGORY_IMAGE_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		this->m_queue.push(data);
		locker.unlock();

		// notify
		// this->m_cond.notify_one();
		this->m_cond.notify_all();
	}
}

template<class T>
void ImageSignal<T>::setSignalContent(void* data, const int* data_size, const int data_dims){
	Matrix<T> signal_content(data_size[0], data_size[1], data, true);
	this->setData(signal_content);
}

template<class T>
void ImageSignal<T>::getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type){
	Matrix<T> matrix_data = this->getData();
	data = (void*)matrix_data.dataptr();
	data_dims = 3;
	data_size[0] = matrix_data.rows();
	data_size[1] = matrix_data.cols();
	data_size[2] = TypeTrait<T>::size;
	data_type = TypeTrait<T>::type;
}

/* Tensor Signal */
template<class T>
TensorSignal<T>::TensorSignal(Tensor<T> m,char* n,char* info){
	this->m_data = m;
	this->m_release_count = 1;
}

template<class T>
TensorSignal<T>::~TensorSignal(){

}

template<class T>
void TensorSignal<T>::copyInfo(AnySignal* sig){
	//call the base class
	AnySignal::copyInfo(sig);
}

template<class T>
void TensorSignal<T>::copy(AnySignal* sig){
	TensorSignal<T>* from_sig = (TensorSignal<T>*)(sig);	
	this->setData(from_sig->getData());
}

template<class T>
void TensorSignal<T>::printUnit(){
	Superclass::printUnit();
}

template<class T>
void TensorSignal<T>::makeempty(bool auto_empty){
	if(auto_empty){
		if(this->m_release_count % this->getOutDegree() != 0){
			this->m_release_count += 1;
			return;
		}
	}
	this->m_data = Tensor<T>();
	if(auto_empty){
		// reset count
		this->m_release_count = 1;
	}

	//force time update
	modified();
}

template<class T>
bool TensorSignal<T>::isempty(){
	if(this->m_data.size() == 0){
		return true;
	}
	else{
		return false;
	}
}

template<class T>
typename TensorSignal<T>::DataType TensorSignal<T>::getData(){
	// refresh data
	if(this->m_delay_time > 0){
		this->m_link_node->refresh();
	}
	// return img
	return this->m_data;
}

template<class T>
void TensorSignal<T>::setData(TensorSignal<T>::DataType data){
	this->m_data = data;
}

template<class T>
void TensorSignal<T>::setSignalContent(void* data, const int* data_size, const int data_dims){
	Tensor<T> signal_content(std::vector<int64_t>{data_size[0],data_size[1], data_size[2], data_size[3]}, 
							 EagleeyeRuntime(EAGLEEYE_CPU),
							data, 
							true);
	this->setData(signal_content);
}


template<class T>
void TensorSignal<T>::getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type){
	Tensor<T> tensor_data = this->getData();
	data = (void*)tensor_data.dataptr();
	data_dims = 4;
	std::vector<int64_t> shape = tensor_data.shape();
	data_size[0] = shape[0];
	data_size[1] = shape[1];
	data_size[2] = shape[2];
	data_size[3] = shape[3];
	data_type = TypeTrait<T>::type;
}

/* Content Signal */
template<class T>
void ContentSignal<T>::copyInfo(AnySignal* sig)
{
	//call the base class
	Superclass::copyInfo(sig);
}

template<class T>
void ContentSignal<T>::printUnit()
{
	Superclass::printUnit();
}

template<class T>
void ContentSignal<T>::makeempty(bool auto_empty)
{
	info=T();

	//force time update
	modified();
}

template<class T>
bool ContentSignal<T>::isempty()
{
	return false;
}

template<class T>
typename ContentSignal<T>::DataType ContentSignal<T>::getData(){
	return this->info;
}

template<class T>
void ContentSignal<T>::setData(ContentSignal<T>::DataType data){
	this->info = data;
}

template<class T>
void ContentSignal<T>::getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type){
	data = &info;
	data_size[0] = 1;
	data_size[1] = 1;
	data_size[2] = 1;
	data_dims = 0;		
	data_type = -1;		// string type
}

template<class T>
void ContentSignal<T>::setSignalContent(void* data, const int* data_size, const int data_dims){
	//
	this->info = *((T*)data);
	// ignore data_size, data_dims
}

}