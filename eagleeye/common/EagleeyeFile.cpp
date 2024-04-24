#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/3rd/pnglib/png.h"
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

bool deletedir(const char *path) {
    DIR *dir = opendir(path);
    if (dir == nullptr) {
        // 打开目录失败
        return false;
    }

    dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            // 跳过当前目录和父目录
            continue;
        }

        // 构造文件/文件夹的完整路径
        std::string fullPath = std::string(path) + "/" + entry->d_name;

        if (entry->d_type == DT_DIR) {
            // 如果是文件夹，递归删除
            if (!deletedir(fullPath.c_str())) {
                closedir(dir);
                return false;
            }
        } else {
            // 如果是文件，直接删除
            if (remove(fullPath.c_str()) != 0) {
                closedir(dir);
                return false;
            }
        }
    }

    // 关闭目录流
    closedir(dir);

    // 删除当前目录
    if (rmdir(path) != 0) {
        return false;
    }

    return true;
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
  if(dirptr != NULL){
	  closedir(dirptr);
  }
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

bool traverseFiles(const char* folder){
	    DIR *dp = NULL;
        struct dirent *dirp;
        if ((dp = opendir(folder)) == NULL) {
			EAGLEEYE_LOGE("couldnt open dir %s", folder);
            return false;
        }

        while ((dirp = readdir(dp)) != NULL) {
            // linux DT_DIR文件夹；DT_REG文件
            if(dirp->d_type & DT_DIR){
                // 文件夹
                std::string dir_name = dirp->d_name;
				EAGLEEYE_LOGI("(folder) %s", dir_name.c_str());
            }
			else if(dirp->d_type & DT_REG){
				// 文件
				std::string file_name = dirp->d_name;
				EAGLEEYE_LOGI("(file) %s", file_name.c_str());
			}
        }

        if(dp != NULL){
            closedir(dp);
        }
        return true;
}

void savepng(const char* file_path, unsigned char* data, int height, int width, int stride, int channel){
    int y;
    FILE *fp = fopen(file_path, "wb");
    if(!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();
    
    if (setjmp(png_jmpbuf(png))) abort();
    
    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    int png_color = 0;
    switch (channel)
    {
    case 1:
        png_color = PNG_COLOR_TYPE_GRAY;
        break;
    case 3:
        png_color = PNG_COLOR_TYPE_RGB;
        break;
    default:
        png_color = PNG_COLOR_TYPE_RGBA;
        break;
    }
    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        png_color,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    //png_set_filler(png, 0, PNG_FILLER_AFTER);
    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int i=0; i<height; ++i){
        row_pointers[i] = data + stride*i*channel;
    }

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    free(row_pointers);
    fclose(fp);
    png_destroy_write_struct(&png, &info);
}

void loadpng(const char* file_path, unsigned char*& data, int& height, int& width, int& channel){
    FILE *fp = fopen(file_path, "rb");
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) abort();

    png_infop info = png_create_info_struct(png);
    if(!info) abort();

    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);
    png_read_info(png, info);

    width      = png_get_image_width(png, info);
    height     = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth  = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if(bit_depth == 16)
        png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }
    png_read_image(png, row_pointers);

    if(color_type == PNG_COLOR_TYPE_RGB){
        // rgb image
        channel = 3;
        data = (unsigned char*)malloc(height*width*channel);
        for(int y=0; y<height; ++y){
            unsigned char* rgb_img_ptr = data + y*width*channel;
            for(int x=0; x<width; ++x){
                rgb_img_ptr[x*3] = *(row_pointers[y] + x*4);
                rgb_img_ptr[x*3+1] = *(row_pointers[y] + x*4 + 1);
                rgb_img_ptr[x*3+2] = *(row_pointers[y] + x*4 + 2);
            }
        }
    }
    else if(color_type == PNG_COLOR_TYPE_RGBA){
        // rgba image
        channel = 4;
        data = (unsigned char*)malloc(height*width*channel);
        for(int y=0; y<height; ++y){
            unsigned char* rgba_img_ptr = data + y*width*channel;
            for(int x=0; x<width; ++x){
                rgba_img_ptr[x*4] = *(row_pointers[y] + x*4);
                rgba_img_ptr[x*4+1] = *(row_pointers[y] + x*4 + 1);
                rgba_img_ptr[x*4+2] = *(row_pointers[y] + x*4 + 2);
                rgba_img_ptr[x*4+3] = *(row_pointers[y] + x*4 + 3);
            }
        }
    }

    for(int y=0; y<height; ++y){
        free(row_pointers[y]);
    }
    free(row_pointers);

    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);
}
}