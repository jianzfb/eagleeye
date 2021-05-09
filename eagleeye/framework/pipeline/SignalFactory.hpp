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
	this->m_meta.rows = 0;
	this->m_meta.cols = 0;
	this->m_meta.needed_rows = 0;
	this->m_meta.needed_cols = 0;	
	this->m_meta.allocate_mode = 0;

	this->m_meta.timestamp = 0;
	this->m_sig_category = SIGNAL_CATEGORY_IMAGE;
	this->m_timestamp = 0;
}

template<class T>
void ImageSignal<T>::copyInfo(AnySignal* sig){
	if(sig == NULL){
        return;
    }

	//call the base class
	BaseImageSignal::copyInfo(sig);
	if(SIGNAL_CATEGORY_IMAGE == (sig->getSignalCategory() & SIGNAL_CATEGORY_IMAGE)){
		this->m_meta = sig->meta();
	}
}

template<class T>
void ImageSignal<T>::copy(AnySignal* sig){
	if((SIGNAL_CATEGORY_IMAGE != (sig->getSignalCategory() & SIGNAL_CATEGORY_IMAGE)) || 
			(this->getSignalValueType() != sig->getSignalValueType())){
		return;
	}
	ImageSignal<T>* from_sig = (ImageSignal<T>*)(sig);	

	MetaData from_data_meta;
	Matrix<T> from_data = from_sig->getData(from_data_meta);
	this->setData(from_data, from_data_meta);
}

template<class T>
void ImageSignal<T>::printUnit()
{
	Superclass::printUnit();

	EAGLEEYE_LOGD("image name %s \n",this->m_meta.name.c_str());
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
	this->m_meta.rows = 0;
	this->m_meta.cols = 0;
	this->m_meta.timestamp = 0;
	this->m_timestamp = 0;
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
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_IMAGE){
		// SIGNAL_CATEGORY_IMAGE
		return img;
	}
	else{
		// SIGNAL_CATEGORY_IMAGE_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		while(this->m_queue.size() == 0){
            this->m_cond.wait(locker);

			if(this->m_queue.size() > 0 || this->m_signal_exit){
				break;
			}
        }

		if(this->m_signal_exit){
			return Matrix<T>();
		}

		Matrix<T> data = this->m_queue.front();
        this->m_queue.pop();
		this->m_meta_queue.pop();
        locker.unlock();
		return data;
	}
}

template<class T>
void ImageSignal<T>::setData(ImageSignal<T>::DataType data){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_IMAGE){
		// SIGNAL_CATEGORY_IMAGE
		this->img = data;
		this->m_meta.rows = this->img.rows();
		this->m_meta.cols = this->img.cols();
	}
	else{
		// SIGNAL_CATEGORY_IMAGE_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		this->m_queue.push(data); 
		MetaData mm;
		mm.rows = data.rows();
		mm.cols = data.cols();
		this->m_meta_queue.push(mm);
		locker.unlock();

		// notify
		this->m_cond.notify_all();
	}

	modified();
}

template<class T>
typename ImageSignal<T>::DataType ImageSignal<T>::getData(MetaData& mm){
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_IMAGE){
		// SIGNAL_CATEGORY_IMAGE
		mm = this->m_meta;
		return img;
	}
	else{
		// SIGNAL_CATEGORY_IMAGE_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		while(this->m_queue.size() == 0){
            this->m_cond.wait(locker);

			if(this->m_queue.size() > 0 || this->m_signal_exit){
				break;
			}
        }
		if(this->m_signal_exit){
			return Matrix<T>();
		}

		Matrix<T> data = this->m_queue.front();
		mm = this->m_meta_queue.front();
        this->m_queue.pop();
		this->m_meta_queue.pop();
        locker.unlock();
		return data;
	}
}

template<class T>
void ImageSignal<T>::setData(ImageSignal<T>::DataType data, MetaData mm){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_IMAGE){
		// SIGNAL_CATEGORY_IMAGE
		this->img = data;
		// ignore needed_rows, needed_cols and allocate_mode
		int needed_rows = this->m_meta.needed_rows;
		int needed_cols = this->m_meta.needed_cols;
		int allocate_mode = this->m_meta.allocate_mode;

		this->m_meta = mm;

		this->m_meta.needed_rows = needed_rows;
		this->m_meta.needed_cols = needed_cols;
		this->m_meta.allocate_mode = allocate_mode;
	}
	else{
		// SIGNAL_CATEGORY_IMAGE_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		this->m_queue.push(data); 
		this->m_meta_queue.push(mm);
		locker.unlock();

		// notify
		// this->m_cond.notify_one();
		this->m_cond.notify_all();
	}

	modified();
}

template<class T>
void ImageSignal<T>::setData(void* data, MetaData meta){
	// const int* data_size, const int data_dims, int rotation
	if(this->getSignalType() == EAGLEEYE_SIGNAL_TEXTURE){
		unsigned int texture_id = *((unsigned int*)(data));
		EAGLEEYE_LOGD("TEXTURE ID %d", texture_id);
		this->img = Matrix<T>(texture_id);
		this->m_timestamp = 0;
		this->m_meta = meta;
		return;
	}

	// MetaData meta_data = this->meta();

	Matrix<T> signal_content;
	if(meta.allocate_mode == 1){
		// InPlace Mode
		signal_content = Matrix<T>(meta.rows, meta.cols, data, false);
	}
	else if(meta.allocate_mode == 2){
		// Largest Mode
		signal_content = Matrix<T>(meta.rows, meta.cols, this->getNeededMem(), false);
		memcpy(signal_content.dataptr(), data, sizeof(T)*meta.rows*meta.cols);
	}
	else{
		// Copy Mode
		signal_content = Matrix<T>(meta.rows, meta.cols, data, true);
	}
	
	meta.timestamp = this->m_timestamp;
	this->m_timestamp += 1;
	this->setData(signal_content, meta);
}

template<class T>
void ImageSignal<T>::getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type){
	this->m_tmp = this->getData();
	data = (void*)this->m_tmp.dataptr();
	data_dims = 3;
	data_size[0] = this->m_tmp.rows();
	data_size[1] = this->m_tmp.cols();
	data_size[2] = TypeTrait<T>::size;
	data_type = TypeTrait<T>::type;
}

template<class T>
void ImageSignal<T>::setMeta(MetaData meta){
	if(meta.allocate_mode == 3 && meta.needed_rows > 0 && meta.needed_cols > 0){
		// allocate allowed largest space
		this->setNeededMem(sizeof(T)*meta.needed_rows*meta.needed_cols);
	}

	this->m_meta = meta;
}

}