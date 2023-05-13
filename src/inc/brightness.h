#include <linux/string.h>
#include "file_ops.h"

#define BACKLIGHT_PATH "/sys/class/backlight/intel_backlight/brightness"
#define MAX_BACKLIGHT_PATH "/sys/class/backlight/intel_backlight/max_brightness"
#define MAX_BUFFER_LEN 6
#define STEP      10


struct bright_data {
	int max; 
	int cur;
	int step;
}

static struct bright_data *data;
static char brightness_str[MAX_BUFFER_LEN];

int backlight_read_value(struct file *file);
void backlight_init(void);
void brightness_update(void);
void backlight_change(int flag);
void brightness_exit(void);