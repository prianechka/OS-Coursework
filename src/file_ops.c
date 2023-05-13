#include "inc/file_ops.h"
#include <asm/uaccess.h>
#include <asm/segment.h>


struct file *file_open(const char *path, int flags, int rights) {
	struct file *fp = NULL;
	
	fp = filp_open(path, flags, rights);
	int error = 0;
    
	if(IS_ERR(fp)){
		error = PTR_ERR(fp);
		printk("tmp brightness: %d", error);
		return NULL;
	}
	
	return fp;
}


ssize_t file_read(struct file *fp, unsigned long long offset, unsigned char *data, unsigned int size) {
    return kernel_read(fp, data, size, &offset);
}


ssize_t file_write(struct file *fp, unsigned long long offset, unsigned char *data, unsigned int size) {
	return kernel_write(fp, data, size, &offset);
}


void file_close(struct file *file) {
    filp_close(file, NULL);
}   
