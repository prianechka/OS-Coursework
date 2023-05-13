#include "inc/brightness.h"

int read_light_value(struct file *file){
	
	int cur_brightness = 0;
	file_read((struct file *) file, 0, brightness_str, MAX_BUFFER_LEN);
	brightness_str[MAX_BUFFER_LEN-1] = '\0';
	kstrtoint(brightness_str, 10, &cur_brightness);
	memset(brightness_str, 0, sizeof(brightness_str));

	return cur_brightness;
}

void backlight_init(void) {
	data = kmalloc(sizeof(struct backlight_data), GFP_KERNEL);
	memset(brightness_str, 0, sizeof(brightness_str));
	
	backlight_file = file_open(BACKLIGHT_PATH, O_RDWR, 0);
	data->cur = backlight_read_value(backlight_file);
	printk(KERN_INFO "brightness.c: Tmp_brightness was initialized with %d!\n", data->tmp_brightness);
	
	struct file *max_backlight_file = file_open(MAX_BACKLIGHT_PATH, O_RDONLY, 0);
	data->max = backlight_read_value(max_backlight_file);
	file_close((struct file *) max_backlight_file);
	printk(KERN_INFO "brightness.c: Max_brightness was initialized with %d!\n", data->max_brightness);
	data->step = data->max / STEP;
	printk(KERN_INFO "brightness.c: Backlight structure was initialized!\n");
}

void brightness_update(void){	
	snprintf(brightness_str, MAX_BRINGHTNESS_CLASS, "%d", data->tmp_brightness);
	file_write(backlight_file, 0, brightness_str, MAX_BRINGHTNESS_CLASS);
	memset(brightness_str, 0, sizeof(brightness_str));
	printk(KERN_INFO "brightness.c: Backlight tmp_brightness was updated!\n");
}

void backlight_change(int flag) {
	
	if (flag == 0) 
		return;
	
	if (flag == 1) {
		data->cur += data->step;
		if (data->cur  > data->max) 
			data->cur = data->max;
		printk(KERN_INFO "brightness.c: Backlight tmp_brightness was increased!\n");
	} 
	else if (flag == -1) {
		data->cur -= data->step;
		if (data->cur <= 8) 
			data->cur = 8;
		printk(KERN_INFO "brightness.c: Backlight tmp_brightness was decreased!\n");
	}
	brightness_update();
}


void brightness_exit(void){
	kfree(data);	
	printk(KERN_INFO "brightness.c: Backlight structure was uninitialized!\n");
}