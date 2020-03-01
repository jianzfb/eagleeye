#ifndef _EAGLEEYE_SERIALSTRINGREADER_H_
#define _EAGLEEYE_SERIALSTRINGREADER_H_
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include <map>

namespace eagleeye{
class SerialStringReader{
public:
	SerialStringReader(std::string prefix, std::string folder);
	virtual ~SerialStringReader();
	
	/**
	 * @brief get next 
	 * 
	 * @param data 
	 * @param size 
	 * @return std::string 
	 */
	virtual std::string next();
	
	/**
	 * @brief serial length
	 * 
	 * @return int64_t 
	 */
	virtual int64_t count();

protected:
	std::string formatName(const std::string input);

private:
	std::map<int64_t, std::string> m_serials;
	int64_t m_current_index;

	std::string m_prefix;
	std::string m_folder;
	int64_t m_count; 
    DIR* m_dir_parent;
};
}

#endif