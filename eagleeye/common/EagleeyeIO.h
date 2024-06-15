#ifndef _EAGLEEYE_IO_H_
#define _EAGLEEYE_IO_H_

#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/DynamicArray.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <fstream>

namespace eagleeye
{
/**
 *	@name IO mode
 *	@{
 */
enum EagleeyeIOMode
{
	READ_BINARY_MODE,
	WRITE_BINARY_MODE,
	READ_ASCII_MODE,
	WRITE_ASCII_MODE,
	UNDEFINED_IO
};
/** @}*/

/**
 *	@brief Matrix, DynamicArray and Array IO
 *	@par Example:
 *	@code
 *	createWriteHandle(file_path,WriteAsciiMode);
 *	write(..,..);
 *	destroyHandle();
 *	createReadHandle(file_path,ReadAsciiMode);
 *	read(..,..);
 *	destroyHandle();
 *	@encode
 */
class EagleeyeIO
{
public:
	EagleeyeIO(){
		// std::locale::global(std::locale(""));
	};
	virtual ~EagleeyeIO(){};

	/**
	 *	@brief create a "Write Handle"
	 *	@param file_path file path
	 *	@param isapp whether open file by app mode
	 *	@param iomode Write Ascii or Binary
	 *	@note createWritehandle and destroyHandle are one pair
	 */
	bool createWriteHandle(std::string file_path,bool isapp=true,EagleeyeIOMode iomode=WRITE_ASCII_MODE);

	/**
	 *	@brief create one "Read Handle"
	 *	@param file_path file path
	 *	@param iomode Read Ascii or Binary
	 *	@note createReadHandle and destroyHandle are one pair
	 */
	bool createReadHandle(std::string file_path,EagleeyeIOMode iomode=READ_ASCII_MODE);

	/**
	 *	@brief destroy "Handle"
	 */
	bool destroyHandle();

	/**
	 *	@brief write/read DynamicArray
	 */
	template<class T>
	bool write(const DynamicArray<T>& arr,std::string id);
	template<class T>
	bool read(DynamicArray<T>& arr,std::string& id);

	template<class T>
	bool write(const DynamicArray<T>& arr);
	template<class T>
	bool read(DynamicArray<T>& arr);

	template<class T>
	bool write(const Matrix<T>& mat,std::string id);
	template<class T>
	bool read(Matrix<T>& mat,std::string& id);
	template<class T>
	bool write(const Matrix<T>& mat);
	template<class T>
	bool read(Matrix<T>& mat);

	bool write(Tensor tensor);
	bool read(Tensor& tensor);

	/**
	 *	@brief write/read common type
	 */
	template<class T>
	bool write(const T& any_data);
	template<class T>
	bool read(T& any_data);

	/**
	 *	@brief write/read string
	 */
	bool write(std::string str);
	bool write(const char* str);
	bool read(std::string& str);

	/**
	 *	@brief only Write/Read in the binary way
	 */
	bool write(const void* info,int size);
	bool read(void*& info,int& size);

	/**
	 *	@brief write/read DynamicArray
	 */
	template<class T>
	static bool write(const DynamicArray<T>& arr,std::string id,std::string file_path,EagleeyeIOMode iomode);
	template<class T>
	static bool read(DynamicArray<T>& arr,std::string& id,std::string file_path,EagleeyeIOMode iomode);
	template<class T>
	static bool write(const DynamicArray<T>& arr,std::string file_path,EagleeyeIOMode iomode);
	template<class T>
	static bool read(DynamicArray<T>& arr,std::string file_path,EagleeyeIOMode iomode);

	/**
	 *	@brief write/read Matrix
	 */
	template<class T>
	static bool write(const Matrix<T>& ma,std::string id,std::string file_path,EagleeyeIOMode iomode);
	template<class T>
	static bool read(Matrix<T>& ma,std::string& id,std::string file_path,EagleeyeIOMode iomode);
	template<class T>
	static bool write(const Matrix<T>& ma,std::string file_path,EagleeyeIOMode iomode);
	template<class T>
	static bool read(Matrix<T>& ma,std::string file_path,EagleeyeIOMode iomode);

private:
	EagleeyeIOMode m_iomode;
	std::ofstream m_o_file_handle;
	std::ifstream m_i_file_handle;
};
}

#include "EagleeyeIO.hpp"
#endif