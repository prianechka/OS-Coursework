obj-m := brightness_module.o

brightness_module-objs += brightness_control_ops.o
brightness_module-objs += brightness_keyboard_controller.o
brightness_module-objs += kernel_space_fops.o

all default:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean