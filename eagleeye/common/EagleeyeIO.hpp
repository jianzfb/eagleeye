namespace eagleeye
{
template<class T>
bool EagleeyeIO::write(const DynamicArray<T>& arr,std::string id)
{
	switch(m_iomode)
	{
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				m_o_file_handle<<id;
				m_o_file_handle<<'\t';
				m_o_file_handle<<arr;
				m_o_file_handle<<'\n';
				return true;
			}
		}
	case WRITE_BINARY_MODE:
		{
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				int str_len=id.length();
				m_o_file_handle.write((char*)(&str_len),sizeof(int));
				m_o_file_handle.write(id.c_str(),sizeof(char)*str_len);

				//write array data
				int arr_size=arr.size();
				m_o_file_handle.write((char*)(&arr_size),sizeof(int));
				m_o_file_handle.write((char*)(arr.dataptr()),sizeof(T)*arr_size);

				return true;
			}
		}
	default:
		break;
	}
	return false;
}

template<class T>
bool EagleeyeIO::read(DynamicArray<T>& arr,std::string& id)
{
	switch(m_iomode)
	{
	case READ_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_i_file_handle||m_i_file_handle.eof())
			{
				return false;
			}
			else
			{
				m_i_file_handle>>id;
				m_i_file_handle>>arr;
				return true;
			}
		}
	case READ_BINARY_MODE:
		{
			if (!m_i_file_handle||m_i_file_handle.peek()==EOF)
			{
				return false;
			}
			else
			{
				int str_len;
				m_i_file_handle.read((char*)(&str_len),sizeof(int));
				char* id_data=new char[str_len+1];
				m_i_file_handle.read(id_data,sizeof(char)*str_len);
				id_data[str_len]='\0';

				id=std::string(id_data);

				delete[] id_data;

				//write array data
				int arr_size;			
				m_i_file_handle.read((char*)(&arr_size),sizeof(int));
				arr=DynamicArray<T>(arr_size);
				m_i_file_handle.read((char*)(arr.dataptr()),sizeof(T)*arr_size);

				return true;
			}
		}
	default:
		break;
	}

	return false;
}

template<class T>
bool EagleeyeIO::write(const DynamicArray<T>& arr)
{
	switch(m_iomode)
	{
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				m_o_file_handle<<arr;
				m_o_file_handle<<'\n';
				return true;
			}
		}
	case WRITE_BINARY_MODE:
		{
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				//write array data
				int arr_size=arr.size();
				m_o_file_handle.write((char*)(&arr_size),sizeof(int));
				m_o_file_handle.write((char*)(arr.dataptr()),sizeof(T)*arr_size);
				return true;
			}
		}
	default:
		break;
	}
	return false;
}

template<class T>
bool EagleeyeIO::read(DynamicArray<T>& arr)
{
	switch(m_iomode)
	{
	case READ_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_i_file_handle||m_i_file_handle.eof())
			{
				return false;
			}
			else
			{
				m_i_file_handle>>arr;
				return true;
			}
		}
	case READ_BINARY_MODE:
		{
			if (!m_i_file_handle||m_i_file_handle.peek()==EOF)
			{
				return false;
			}
			else
			{
				//write array data
				int arr_size;
				m_i_file_handle.read((char*)(&arr_size),sizeof(int));
				arr=DynamicArray<T>(arr_size);
				m_i_file_handle.read((char*)(arr.dataptr()),sizeof(T)*arr_size);

				return true;
			}
		}
	default:
		break;
	}

	return false;
}

template<class T>
bool EagleeyeIO::write(const Matrix<T>& mat,std::string id)
{
	switch(m_iomode)
	{
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				m_o_file_handle<<id;
				m_o_file_handle<<'\t';
				m_o_file_handle<<mat;
				m_o_file_handle<<'\n';
				return true;
			}
		}
	case WRITE_BINARY_MODE:
		{
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				int str_len = id.length();
				m_o_file_handle.write((char*)(&str_len),sizeof(int));
				m_o_file_handle.write(id.c_str(),sizeof(char) * str_len);

				int rows = mat.rows();
				int cols = mat.cols();

				m_o_file_handle.write((char*)(&rows),sizeof(int));
				m_o_file_handle.write((char*)(&cols),sizeof(int));

				for (int i = 0; i < rows; ++i)
				{
					const T* d = mat.row(i);
					m_o_file_handle.write((char*)(d),sizeof(T)*cols);
				}

				return true;
			}
		}
	default:
		break;
	}
	return false;
}
template<class T>
bool EagleeyeIO::read(Matrix<T>& mat,std::string& id)
{
	switch(m_iomode)
	{
	case READ_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_i_file_handle || m_i_file_handle.eof())
			{
				return false;
			}
			else
			{
				m_i_file_handle>>id;
				m_i_file_handle>>mat;
				return true;
			}
		}
	case READ_BINARY_MODE:
		{
			if (!m_i_file_handle || m_i_file_handle.peek() == EOF)
			{
				return false;
			}
			else
			{
				int str_len;
				m_i_file_handle.read((char*)(&str_len),sizeof(int));
				char* id_data = new char[str_len + 1];
				m_i_file_handle.read(id_data,sizeof(char) * str_len);
				id_data[str_len] = '\0';

				id = std::string(id_data);
				delete[] id_data;
				
				//read mat data
				int rows,cols;
				m_i_file_handle.read((char*)(&rows),sizeof(int));
				m_i_file_handle.read((char*)(&cols),sizeof(int));

				mat = Matrix<T>(rows,cols);
				m_i_file_handle.read((char*)(mat.dataptr()),sizeof(T) * rows * cols);

				return true;
			}
		}
	default:
		break;
	}

	return false;
}

template<class T>
bool EagleeyeIO::write(const Matrix<T>& mat)
{
	switch(m_iomode)
	{
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				m_o_file_handle<<mat;
				return true;
			}
		}
	case WRITE_BINARY_MODE:
		{
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				//write array data
				int rows=mat.rows();
				int cols=mat.cols();
				
				m_o_file_handle.write((char*)(&rows),sizeof(int));
				m_o_file_handle.write((char*)(&cols),sizeof(int));

				for (int i=0;i<rows;++i)
				{
					const T* d=mat.row(i);
					m_o_file_handle.write((char*)(d),sizeof(T)*cols);
				}

				return true;
			}
		}
	default:
		break;
	}
	return false;
}

template<class T>
bool EagleeyeIO::read(Matrix<T>& mat)
{
	switch(m_iomode)
	{
	case READ_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_i_file_handle||m_i_file_handle.eof())
			{
				return false;
			}
			else
			{
				m_i_file_handle>>mat;
				return true;
			}
		}
	case READ_BINARY_MODE:
		{
			if (!m_i_file_handle||m_i_file_handle.peek()==EOF)
			{
				return false;
			}
			else
			{
				//read mat data
				int rows,cols;
				m_i_file_handle.read((char*)(&rows),sizeof(int));
				m_i_file_handle.read((char*)(&cols),sizeof(int));

				mat=Matrix<T>(rows,cols);
				m_i_file_handle.read((char*)(mat.dataptr()),sizeof(T)*rows*cols);

				return true;
			}
		}
	default:
		break;
	}

	return false;
}

template<class T>
bool EagleeyeIO::write(const T& any_data)
{
	switch(m_iomode)
	{
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				m_o_file_handle<<any_data;
				return true;
			}
		}
	case WRITE_BINARY_MODE:
		{
			if (!m_o_file_handle)
			{
				return false;
			}
			else
			{
				m_o_file_handle.write((char*)&any_data,sizeof(T));
				return true;
			}
		}
	default:
		break;
	}

	return false;
}

template<class T>
bool EagleeyeIO::read(T& any_data)
{
	switch(m_iomode)
	{
	case READ_ASCII_MODE:
		{
			if (!m_i_file_handle||m_i_file_handle.eof())
			{
				return false;
			}
			else
			{
				m_i_file_handle>>any_data;
				return true;
			}
		}
	case READ_BINARY_MODE:
		{
			if (!m_i_file_handle||m_i_file_handle.peek()==EOF)
			{
				return false;
			}
			else
			{
				m_i_file_handle.read((char*)(&any_data),sizeof(T));
				return true;
			}
		}
	default:
		break;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
//some static functions
//write/read DynamicArray
template<class T>
bool EagleeyeIO::write(const DynamicArray<T>& arr,std::string id,std::string file_path,EagleeyeIOMode iomode)
{
	switch(iomode)
	{
	case WRITE_BINARY_MODE:
		{
			std::ofstream binary_file(file_path.c_str(),std::ios::binary);

			if (!binary_file)
			{
				return false;
			}
			else
			{
				//write id
				int str_len=id.length();
				binary_file.write((char*)(&str_len),sizeof(int));
				binary_file.write(id.c_str(),sizeof(char)*str_len);

				//write array data
				int arr_size=arr.size();
				binary_file.write((char*)(&arr_size),sizeof(int));
				binary_file.write((char*)arr.dataptr(),sizeof(T)*arr_size);

				binary_file.close();

				return true;
			}

		}
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			std::ofstream asc_file(file_path.c_str());

			if (asc_file)
			{
				asc_file<<id<<'\n';
				asc_file<<arr;
				
				asc_file.close();
				return true;
			}
			return false;
		}
	default:
		{
			return false;
		}
	}

	return false;
}

template<class T>
bool EagleeyeIO::read(DynamicArray<T>& arr,std::string& id,std::string file_path,EagleeyeIOMode iomode)
{
	switch(iomode)
	{
	case READ_BINARY_MODE:
		{
			std::ifstream binary_file(file_path.c_str(),std::ios::binary);

			if (!binary_file)
			{
				return false;
			}
			else
			{
				//write id						
				int str_len;
				binary_file.read((char*)(&str_len),sizeof(int));
				char* id_data=new char[str_len];
				binary_file.read(id_data,sizeof(char)*str_len);

				id=std::string(id_data);
				delete[] id_data;

				//write array data
				int arr_size=arr.size();				
				binary_file.read((char*)(&arr_size),sizeof(int));
				arr=DynamicArray<T>(arr_size);
				binary_file.read((char*)(arr.dataptr()),sizeof(T)*arr_size);

				binary_file.close();

				return true;
			}

		}
	case READ_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			std::ifstream ascii_file(file_path.c_str());
			ascii_file>>id;
			ascii_file>>arr;
			ascii_file.close();
			return false;
		}
	default:
		{
			return false;
		}
	}

	return false;
}

template<class T>
bool EagleeyeIO::write(const DynamicArray<T>& arr,std::string file_path,EagleeyeIOMode iomode)
{
	switch(iomode)
	{
	case WRITE_BINARY_MODE:
		{
			std::ofstream binary_file(file_path.c_str(),std::ios::binary);

			if (!binary_file)
			{
				return false;
			}
			else
			{
				//write array data
				int arr_size=arr.size();
				binary_file.write((char*)(&arr_size),sizeof(int));
				binary_file.write((char*)arr.dataptr(),sizeof(T)*arr_size);
				
				binary_file.close();

				return true;
			}

		}
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			std::ofstream asc_file(file_path.c_str());

			if (asc_file)
			{
				asc_file<<arr;
				asc_file.close();
				return true;
			}
			return false;
		}
	default:
		{
			return false;
		}
	}

	return false;
}

template<class T>
bool EagleeyeIO::read(DynamicArray<T>& arr,std::string file_path,EagleeyeIOMode iomode)
{
	switch(iomode)
	{
	case READ_BINARY_MODE:
		{
			std::ifstream binary_file(file_path.c_str(),std::ios::binary);

			if (!binary_file)
			{
				return false;
			}
			else
			{
				//write array data
				int arr_size;		
				binary_file.read((char*)(&arr_size),sizeof(int));
				arr=DynamicArray<T>(arr_size);
				binary_file.read((char*)(arr.dataptr()),sizeof(T)*arr_size);

				binary_file.close();

				return true;
			}
		}
	case READ_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			std::ifstream ascii_file(file_path.c_str());
			if (!ascii_file)
			{
				return false;
			}
			else
			{
				ascii_file>>arr;
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

template<class T>
bool EagleeyeIO::write(const Matrix<T>& mat,std::string id,std::string file_path,EagleeyeIOMode iomode)
{
	switch(iomode)
	{
	case WRITE_BINARY_MODE:
		{
			std::ofstream binary_file(file_path.c_str(),std::ios::binary);

			if (!binary_file)
			{
				return false;
			}
			else
			{
				//write id
				int str_len=id.length();
				binary_file.write((char*)(&str_len),sizeof(int));
				binary_file.write(id.c_str(),sizeof(char)*str_len);

				//write matrix data
				int rows=mat.rows();
				int cols=mat.cols();

				binary_file.write((char*)(&rows),sizeof(int));
				binary_file.write((char*)(&cols),sizeof(int));

				for (int i=0;i<rows;++i)
				{
					binary_file.write(mat.row(i),sizeof(T)*cols);
				}

				binary_file.close();

				return true;
			}

		}
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			std::ofstream asc_file(file_path.c_str());

			if (asc_file)
			{
				asc_file<<id<<'\n';
				asc_file<<mat;

				asc_file.close();
				return true;
			}
			return false;
		}
	default:
		break;
	}

	return false;
}

template<class T>
bool EagleeyeIO::read(Matrix<T>& mat,std::string& id,std::string file_path,EagleeyeIOMode iomode)
{
	switch(iomode)
	{
	case READ_BINARY_MODE:
		{
			std::ifstream binary_file(file_path.c_str(),std::ios::binary);

			if (!binary_file)
			{
				return false;
			}
			else
			{
				//write id						
				int str_len;
				binary_file.read((char*)(&str_len),sizeof(int));
				char* id_data=new char[str_len];
				binary_file.read(id_data,sizeof(char)*str_len);

				id=std::string(id_data);
				delete[] id_data;
				
				//read mat data
				int rows;
				int cols;
				binary_file.read((char*)(&rows),sizeof(int));
				binary_file.read((char*)(&cols),sizeof(int));

				mat=Matrix<T>(rows*cols);

				binary_file.read((char*)(mat.dataptr()),sizeof(T)*rows*cols);
				binary_file.close();

				return true;
			}
		}
	case READ_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			std::ifstream ascii_file(file_path.c_str());
			ascii_file>>id;
			ascii_file>>mat;

			ascii_file.close();
			return false;
		}
	default:
		break;
	}

	return false;
}

template<class T>
bool EagleeyeIO::write(const Matrix<T>& mat,std::string file_path,EagleeyeIOMode iomode)
{
	switch(iomode)
	{
	case WRITE_BINARY_MODE:
		{
			// std::locale::global(std::locale(""));
			std::ofstream binary_file(file_path.c_str(),std::ios::binary);

			if (!binary_file)
			{
				return false;
			}
			else
			{
				//write matrix data
				int rows=mat.rows();
				int cols=mat.cols();
				
				binary_file.write((char*)(&rows),sizeof(int));
				binary_file.write((char*)(&cols),sizeof(int));
				for (int i=0;i<rows;++i)
				{
					binary_file.write((char*)mat.row(i),sizeof(T)*cols);
				}

				binary_file.close();

				return true;
			}

		}
	case WRITE_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			std::ofstream asc_file(file_path.c_str());

			if (asc_file)
			{
				asc_file<<mat;

				asc_file.close();
				return true;
			}
			return false;
		}
	default:
		break;
	}

	return false;
}

template<class T>
bool EagleeyeIO::read(Matrix<T>& mat,std::string file_path,EagleeyeIOMode iomode)
{
	switch(iomode)
	{
	case READ_BINARY_MODE:
		{
			// std::locale::global(std::locale(""));
			std::ifstream binary_file(file_path.c_str(),std::ios::binary);
			if (!binary_file)
			{
				return false;
			}
			else
			{
				//read mat data
				int rows;
				int cols;
				binary_file.read((char*)(&rows),sizeof(int));
				binary_file.read((char*)(&cols),sizeof(int));
				mat=Matrix<T>(rows,cols);

				binary_file.read((char*)(mat.dataptr()),sizeof(T)*rows*cols);
				binary_file.close();

				return true;
			}
		}
	case READ_ASCII_MODE:
		{
			// std::locale::global( std::locale( "",std::locale::all^std::locale::numeric ) );
			std::ifstream ascii_file(file_path.c_str());
			ascii_file>>mat;

			ascii_file.close();
			return false;
		}
	default:
		break;
	}

	return false;
}

}
