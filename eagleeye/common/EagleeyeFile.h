#ifndef _EAGLEEYEFILE_H_
#define _EAGLEEYEFILE_H_


namespace eagleeye{
bool copyfile(const char* src_file,const char* target_file);
bool deletefile(const char* file_path);
bool renamefile(const char* old_file_name,const char* new_file_name);

bool createdirectory(const char* path);
bool isdirexist(const char* path);
bool isfileexist(const char* path);


}

#endif