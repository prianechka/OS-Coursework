#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

static struct file *backlight_file;

struct file *file_open(const char *path, int flags, int rights);
ssize_t file_read(struct file *fp, unsigned long long offset, unsigned char *data, unsigned int size);
ssize_t file_write(struct file *fp, unsigned long long offset, unsigned char *data, unsigned int size);
void file_close(struct file *file);