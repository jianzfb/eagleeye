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
	this->m_meta.allocate_mode = 1;
	this->m_meta.color_format = -1;

	this->m_meta.timestamp = 0;
	this->m_sig_category = SIGNAL_CATEGORY_IMAGE;
	this->m_max_queue_size = 5;
	this->m_get_then_auto_remove = true;
	this->setSignalType(EAGLEEYE_SIGNAL_IMAGE);
}

template<class T>
void ImageSignal<T>::copy(AnySignal* sig){
	if((SIGNAL_CATEGORY_IMAGE != (sig->getSignalCategory() & SIGNAL_CATEGORY_IMAGE)) || 
			(this->getValueType() != sig->getValueType())){
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
	//ignore auto_empty
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

	//force time update
	if(!m_disable_data_timestamp){
		modified();
	}
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
			if(this->m_queue.size() > 0 || m_disable){
				break;
			}
        }
		if(m_disable){
			// 由于失活，产生空数据返回
			locker.unlock();
			return Matrix<T>();
		}

		std::pair<Matrix<T>, int> data_info = this->m_queue.front();
		Matrix<T> data = data_info.first;
		if(this->m_get_then_auto_remove){
			this->m_queue.front().second -= 1;
			if(this->m_queue.front().second == 0){
				this->m_queue.pop();
				this->m_meta_queue.pop();
			}
		}
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
		if(!m_disable_data_timestamp){
			modified();
		}
	}
	else{
		// SIGNAL_CATEGORY_IMAGE_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		if(this->m_queue.size() > this->m_max_queue_size){
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}

		this->m_queue.push(std::pair<Matrix<T>, int>{data, int(this->getOutDegree())}); 
		MetaData mm;
		mm.rows = data.rows();
		mm.cols = data.cols();
		this->m_meta_queue.push(std::pair<MetaData, int>{mm, int(this->getOutDegree())});
		locker.unlock();

		if(!m_disable_data_timestamp){
			modified();
		}
		// notify
		this->m_cond.notify_all();
	}
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
			if(this->m_queue.size() > 0 || m_disable){
				break;
			}
        }
		if(m_disable){
			// 由于失活，产生空数据返回
			locker.unlock();
			mm = MetaData();
			mm.disable = true;
			return Matrix<T>();
		}
	
		std::pair<Matrix<T>, int> data_info = this->m_queue.front();
		Matrix<T> data = data_info.first;
		std::pair<MetaData, int> meta_info = this->m_meta_queue.front();
		mm = meta_info.first;
	
		if(this->m_get_then_auto_remove){
			this->m_queue.front().second -= 1;
			if(this->m_queue.front().second == 0){
				this->m_queue.pop();
				this->m_meta_queue.pop();
			}
		}
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
		if(!m_disable_data_timestamp){
			modified();
		}
	}
	else{
		// SIGNAL_CATEGORY_IMAGE_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		if(this->m_queue.size() > this->m_max_queue_size){
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}
		this->m_queue.push(std::pair<Matrix<T>, int>{data, this->getOutDegree()}); 
		this->m_meta_queue.push(std::pair<MetaData, int>{mm, this->getOutDegree()});
		locker.unlock();

		if(!m_disable_data_timestamp){
			modified();
		}
		// notify
		this->m_cond.notify_all();
	}
}

template<class T>
void ImageSignal<T>::setData(void* data, MetaData meta){
	// const int* data_size, const int data_dims, int rotation
	if(this->getSignalType() == EAGLEEYE_SIGNAL_TEXTURE){
		unsigned int texture_id = *((unsigned int*)(data));
		EAGLEEYE_LOGD("TEXTURE ID %d", texture_id);
		this->img = Matrix<T>(texture_id);
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
	this->setData(signal_content, meta);
}

template<class T>
void ImageSignal<T>::getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type){
	this->m_tmp = this->getData();
	data = (void*)this->m_tmp.dataptr();
	this->m_data_size[0] = this->m_tmp.rows();
	this->m_data_size[1] = this->m_tmp.cols();
	this->m_data_size[2] = TypeTrait<T>::size;

	data_size = this->m_data_size;
	data_dims = 3;
	data_type = TypeTrait<T>::type;
}

template<class T>
void ImageSignal<T>::getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type, MetaData& data_meta){
	this->m_tmp = this->getData(data_meta);
	data = (void*)this->m_tmp.dataptr();
	this->m_data_size[0] = this->m_tmp.rows();
	this->m_data_size[1] = this->m_tmp.cols();
	this->m_data_size[2] = TypeTrait<T>::size;
	data_size = this->m_data_size;
	
	data_dims = 3;
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

template<class T>
void ImageSignal<T>::wake(){
	this->m_cond.notify_all();
}
}