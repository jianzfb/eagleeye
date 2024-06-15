#include "eagleeye/common/EagleeyeIO.h"

namespace eagleeye
{
bool EagleeyeIO::createWriteHandle(std::string file_path,bool isapp,EagleeyeIOMode iomode/* =WriteAsciiMode */)
{
	m_iomode=iomode;
	switch(m_iomode)
	{
	case WRITE_ASCII_MODE:
		{
			if (isapp)
			{
				m_o_file_handle.open(file_path.c_str(),std::ios::app);
			}
			else
			{
				m_o_file_handle.open(file_path.c_str());
			}

			if (!m_o_file_handle)
			{
				m_iomode=UNDEFINED_IO;
				return false;
			}
			else
				return true;
		}
	case WRITE_BINARY_MODE:
		{
			if (isapp)
			{
				m_o_file_handle.open(file_path.c_str(),std::ios::app|std::ios::binary|std::ios::out);
			}
			else
			{
				m_o_file_handle.open(file_path.c_str(),std::ios::binary);
			}
			
			if (!m_o_file_handle)
			{
				m_iomode=UNDEFINED_IO;
				return false;
			}
			else
			{
				return true;
			}
		}
	default:
		{
			return false;
		}
	}

	return false;
}

bool EagleeyeIO::createReadHandle(std::string file_path,EagleeyeIOMode iomode/* =ReadAsciiMode */)
{
	m_iomode=iomode;
	switch(m_iomode)
	{
	case READ_ASCII_MODE:
		{
			m_i_file_handle.open(file_path.c_str());
			if (!m_i_file_handle)
			{
				m_iomode=UNDEFINED_IO;
				return false;
			}
			else
				return true;
		}
	case READ_BINARY_MODE:
		{
			m_i_file_handle.open(file_path.c_str(),std::ios::binary);
			if (!m_i_file_handle)
			{
				m_iomode=UNDEFINED_IO;
				return false;
			}
			else
			{
				return true;
			}
		}
	default:
		{
			return false;
		}
	}

	return false;
}

bool EagleeyeIO::destroyHandle()
{
	switch(m_iomode)
	{
	case READ_ASCII_MODE:
	case READ_BINARY_MODE:
		{
			m_i_file_handle.close();
			return true;
		}
	case WRITE_ASCII_MODE:
	case WRITE_BINARY_MODE:
		{
			m_o_file_handle.close();
			return true;
		}
	default:
		break;
	}
	return false;
}

bool EagleeyeIO::write(std::string str)
{
	switch(m_iomode)
	{
	case WRITE_ASCII_MODE:
		{
			m_o_file_handle<<str<<'\n';
			return true;
		}
	case WRITE_BINARY_MODE:
		{
			int str_len=str.length();
			m_o_file_handle.write((char*)(&str_len),sizeof(int));
			m_o_file_handle.write(str.c_str(),sizeof(char)*str_len);
			return true;
		}
	default:
		break;
	}

	return false;
}

bool EagleeyeIO::write(const char* str)
{
	const char* temp_str=str;
	switch(m_iomode)
	{
	case WRITE_ASCII_MODE:
		{
			while((*temp_str)!='\0')
			{
				m_o_file_handle<<*temp_str;
				temp_str=temp_str+1;
			}

			m_o_file_handle<<'\n';
			return true;
		}
	case WRITE_BINARY_MODE:
		{
			int str_len=0;
			while((*temp_str)!='\0')
			{
				str_len++;
				temp_str=temp_str+1;
			}

			m_o_file_handle.write((char*)(&str_len),sizeof(int));
			m_o_file_handle.write(str,sizeof(char)*str_len);

			break;
		}
	default:
		break;
	}
	return false;
}

bool EagleeyeIO::read(std::string& str)
{
	switch(m_iomode)
	{
	case READ_ASCII_MODE:
		{
			m_i_file_handle>>str;
			return true;
		}
	case READ_BINARY_MODE:
		{
			int str_len;
			m_i_file_handle.read((char*)(&str_len),sizeof(int));
			
			char* data=new char[str_len+1];
			m_i_file_handle.read(data,sizeof(char)*str_len);
			data[str_len]='\0';
			str=std::string(data);
			delete[] data;

			return true;
		}
	default:
		break;
	}

	return false;
}

bool EagleeyeIO::write(const void* info,int size)
{
	switch(m_iomode)
	{
	case WRITE_BINARY_MODE:
		{
			m_o_file_handle.write((const char*)(&size),sizeof(int));
			m_o_file_handle.write((const char*)(info),sizeof(char)*size);
			return true;
		}
	default:
		break;
	}
	return false;
}

bool EagleeyeIO::read(void*& info,int& size)
{
	switch(m_iomode)
	{
	case READ_BINARY_MODE:
		{
			int mm = 0;
			m_i_file_handle.read((char*)(&mm),sizeof(int));
			size = mm;
			m_i_file_handle.read((char*)info, sizeof(char)*size);
			return true;
		}
	default:
		break;
	}

	return false;
}

bool EagleeyeIO::write(Tensor tensor){
	// 形状
	int64_t dim_num = tensor.dims().size();
	this->write(&dim_num, sizeof(int64_t));

	std::vector<int64_t> tensor_shape = tensor.dims().data();
	this->write(tensor_shape.data(), tensor_shape.size()*sizeof(int64_t));

	// 类型
	int64_t tensor_type = (int64_t)(tensor.type());
	this->write(&tensor_type, sizeof(int64_t));

	// 数据
	this->write(tensor.cpu(), tensor.blobsize());
	return true;
}

bool EagleeyeIO::read(Tensor& tensor){
	// 形状
	int64_t dim_num_array[1];
	void* dim_num_array_ptr = (void*)dim_num_array;
	int dim_num_size = 0;
	this->read(dim_num_array_ptr, dim_num_size);

	// 
	std::vector<int64_t> tensor_shape(dim_num_array[0]);
	void* tensor_shape_ptr = (void*)(tensor_shape.data());
	int tensor_shape_size = 0;
	this->read(tensor_shape_ptr, tensor_shape_size);

	// 类型
	int64_t tensor_type_array[1];
	void* tensor_type_array_ptr = (void*)tensor_type_array;
	int tensor_type_size = 0;
	this->read(tensor_type_array_ptr, tensor_type_size);

	// 数据
	tensor = Tensor(
		tensor_shape,
		EagleeyeType(tensor_type_array[0]),
		DataFormat::AUTO,
		CPU_BUFFER
	);

	int tensor_data_size = 0;
	void* tensor_ptr = tensor.cpu();
	this->read(tensor_ptr, tensor_data_size);
	return true;
}
}