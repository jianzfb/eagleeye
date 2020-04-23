#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeLog.h"
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

namespace eagleeye{
bool copyfile(const char* src_file,const char* target_file){
	FILE* from_fd = NULL;
	FILE* to_fd = NULL;
	from_fd = fopen(src_file,"rb");
	to_fd = fopen(target_file,"wb+");

	if (!from_fd){
		EAGLEEYE_LOGD("open %s error\n",src_file);
		return false;
	}
	if (!to_fd){
		EAGLEEYE_LOGD("open %s error\n",target_file);
		return false;
	}

	char buf[256];
	while (!feof(from_fd)){
		size_t bytes_read = fread(buf, 1, 256, from_fd);
		fwrite(buf, 1, bytes_read, to_fd);
	}

	fclose(from_fd);
	fclose(to_fd);

	return true;
}

bool deletefile(const char* file_path){
	if (remove(file_path))
		return true;
	else
		return false;
}

bool renamefile(const char* old_file_name,const char* new_file_name){
	if (rename(old_file_name,new_file_name))
		return true;
	else
		return false;
}

bool createdirectory(const char* path){
	char DirName[256];      
	strcpy(DirName,path);     
	int i,len = strlen(DirName);     
	if(DirName[len-1]!='/')      
		strcat(DirName,"/");   

	len = strlen(DirName);      
	for(i=1; i<len; i++){ 
		if(DirName[i]=='/'){      
		    DirName[i] = 0;      
		    int a = access(DirName, F_OK);
		    if(a ==-1){
		        mkdir(DirName,0755);
		    }    
		    DirName[i] = '/';      
		} 
    }        
  return true;      
}

bool isdirexist(const char* path){
  if(path == NULL){
    return false;
  }
  DIR *dirptr = opendir(path);
  bool isnotnull = (dirptr != NULL);
  closedir(dirptr);
  return isnotnull;
}

bool isfileexist(const char* path){
	if (FILE *file = fopen(path, "r")) {
		fclose(file);
		return true;
	} else {
		return false;
	}
}
}