#ifndef _EAGLEEYEFILE_H_
#define _EAGLEEYEFILE_H_


namespace eagleeye{
/**
 * @brief copyt file to 
 */ 
bool copyfile(const char* src_file,const char* target_file);

/**
 * @brief remove file
 */ 
bool deletefile(const char* file_path);

/**
 * @brief remove dir
 */
 bool deletedir(const char* dir_path);

/**
 * @brief rename file name
 */ 
bool renamefile(const char* old_file_name,const char* new_file_name);

/**
 * @brief create directory
 */ 
bool createdirectory(const char* path);

/**
 * @brief check whether folder exist
 */ 
bool isdirexist(const char* path);

/**
 * @brief check whether file exist
 */ 
bool isfileexist(const char* path);

/**
 * @brief traverse files in folder
 */ 
bool traverseFiles(const char* folder);

/**
* @brief save png file
*/
void savepng(const char* file_path, unsigned char* data, int height, int width, int stride, int channel);

}

#endif