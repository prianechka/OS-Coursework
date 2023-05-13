#include <linux/module.h>     // для работы с модулем
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>  // для прерываний и тасклета
#include <asm/io.h>           // для работы с портом ввода-вывода
#include <linux/string.h>     // функции memset, memccpy и т.д.

#define KB_IRQ              1
#define SHIFT_PR            54 
#define SHIFT_REL           182 

struct keyboard_data{
    unsigned char scancode;
} kb_data;

irq_handler_t kb_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
void tasklet_brightness_controller(unsigned long info);



