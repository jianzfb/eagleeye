#include "eagleeye/common/EagleeyeSerial.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye{
SerialStringReader::SerialStringReader(std::string prefix, std::string folder){
	this->m_prefix = prefix;
	this->m_folder = folder;
	this->m_count = 0;

	this->m_dir_parent = opendir(this->m_folder.c_str());
	if(!this->m_dir_parent){
		EAGLEEYE_LOGE("couldnt open folder");
	}
	struct dirent* entry = readdir(this->m_dir_parent);
	while(entry){
		std::string file_name = std::string(entry->d_name);
    	std::string prefix = this->formatName(this->m_prefix);
		if(file_name.find(prefix) == 0) {
	        std::string file_path = this->m_folder + "/" + file_name;
        	std::vector<std::string> file_name_keys = eagleeye::split(file_name, ".");
        	std::vector<std::string> file_name_index = eagleeye::split(file_name_keys[0], "_");

        	int file_index = atoi(file_name_index[1].c_str());
        	this->m_serials[file_index] = file_path;
        	this->m_count += 1;	
	    }

		entry = readdir(this->m_dir_parent);
	}

	if(this->m_serials.size() != this->m_count){
		EAGLEEYE_LOGD("file number not consistent with subfix");
	}
	this->m_current_index=0;
}

SerialStringReader::~SerialStringReader(){
	if(this->m_dir_parent){
		closedir(this->m_dir_parent);
	}
}

std::string SerialStringReader::formatName(const std::string input) {
  std::string res = input;
  for (size_t i = 0; i < input.size(); ++i) {
    if (!isalnum(res[i])) res[i] = '_';
  }
  return res;
}

std::string SerialStringReader::next(){
	if(!this->m_dir_parent){
		EAGLEEYE_LOGD("Read data failed");
		return std::string();
	}
	if(this->m_serials.find(this->m_current_index) == this->m_serials.end()){
		EAGLEEYE_LOGD("have no more data");
		return std::string();
	}
	std::string file_path = this->m_serials[this->m_current_index];
	this->m_current_index += 1;
    return file_path;
}

int64_t SerialStringReader::count(){
	return this->m_count;
}	
}
