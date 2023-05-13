#include "inc/controller.h"
#include "inc/brightness.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb/input.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define DRIVER_NAME    "Driver for brightness"
#define DRIVER_AUTHOR  "Alexander Pryanishnikov"
#define DRIVER_DESC    "Driver for my mouse"
#define DRIVER_LICENSE "JOPA"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

#define ID_VENDOR_MOUSE HUI
#define ID_PRODUCT_MOUSE PIZDA
#define USB_PACKET_LEN  10
#define WHEEL_THRESHOLD 4

struct my_mouse {
    unsigned char     *data;
    dma_addr_t         data_dma;
    struct input_dev  *input_dev;
    struct usb_device *usb_dev;
    struct urb        *irq;
    int                old_wheel_pos;
    char               phys[32];
};

typedef struct my_mouse mouse_t;

struct urb_workqueue {
    struct urb *urb;
    struct work_struct work;
};

typedef struct urb_workqueue urb_workqueue_t;

static int shift = 0;
static int bright_flag = 0;
static int pressed_key;
static struct workqueue_struct *workq;
static struct input_dev *virtual_mouse;

static void work_irq(struct work_struct *work) {
    urb_workqueue_t *container = container_of(work, urb_workqueue_t, work);
    struct urb *urb;
    int retval;
    mouse_t *mouse;
    unsigned char *data;

    if (container == NULL) {
        printk(KERN_ERR "%s: %s - container is NULL\n", DRIVER_NAME, __func__);
        return;
    }

    urb = container->urb;
    mouse = urb->context;
    data = mouse->data;

    if (urb->status != 0) {
        printk(KERN_ERR "%s: %s - urb status is %d\n", DRIVER_NAME, __func__, urb->status);
        kfree(container);
        return;
    }

    switch(data[1]) {
        case 1:
            if (shift) 
                bright_flag = 1;
            break;
        case 2:
            if (shift) 
                bright_flag = -1;
            break;
        default:
            break;
    }
    backlight_change(bright_flag);
    if (!bright_flag) {
        input_report_key(virtual_mouse, BTN_LEFT, data[1] & 0x0001);
        input_report_key(virtual_mouse, BTN_RIGHT, data[1] & 0x0002);
    } 
    bright_flag = 0;

	input_report_rel(virtual_mouse, REL_X,     data[2] - data[3]);
    input_report_rel(virtual_mouse, REL_Y,     data[4] - data[5]);
	input_report_rel(virtual_mouse, REL_WHEEL, data[6]);

    input_sync(virtual_mouse);

    retval = usb_submit_urb(urb, GFP_ATOMIC);
    if (retval)
        printk(KERN_ERR "%s: %s - usb_submit_urb failed with result %d\n", DRIVER_NAME, __func__, retval);

    kfree(container);
}

static void my_mouse_irq(struct urb *urb) {
    urb_workqueue_t *container = kzalloc(sizeof(urb_workqueue_t), GFP_KERNEL);
    container->urb = urb;
    INIT_WORK(&container->work, work_irq);
    queue_work(workq, &container->work);
}

static int my_mouse_open(struct input_dev *dev) {
    mouse_t *mouse = input_get_drvdata(dev);

    mouse->old_wheel_pos = -WHEEL_THRESHOLD - 1;
    mouse->irq->dev = mouse->usb_dev;
    if (usb_submit_urb(mouse->irq, GFP_KERNEL))
        return -EIO;

    return 0;
}

static void my_mouse_close(struct input_dev *dev) {
    mouse_t *mouse = input_get_drvdata(dev);
    usb_kill_urb(mouse->irq);
}

static int my_mouse_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_device *usb_device = interface_to_usbdev(interface);
    mouse_t *mouse;
    struct input_dev *input_dev;
    struct usb_endpoint_descriptor *endpoint;
    int error = -ENOMEM;

    printk(KERN_INFO "%s: probe checking my_mouse\n", DRIVER_NAME);

    mouse = kzalloc(sizeof(mouse_t), GFP_KERNEL);
    input_dev = input_allocate_device();
    if (!mouse || !input_dev) {
        input_free_device(input_dev);
        kfree(mouse);

        printk(KERN_ERR "%s: error when allocate device\n", DRIVER_NAME);
        return error;
    }

    mouse->data = (unsigned char *)usb_alloc_coherent(usb_device, USB_PACKET_LEN, GFP_KERNEL, &mouse->data_dma);
    if (!mouse->data) {
        input_free_device(input_dev);
        kfree(mouse);

        printk(KERN_ERR "%s: error when allocate coherent\n", DRIVER_NAME);
        return error;
    }

    mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
    if (!mouse->irq) {
        usb_free_coherent(usb_device, USB_PACKET_LEN, mouse->data, mouse->data_dma);
        input_free_device(input_dev);
        kfree(mouse);

        printk(KERN_ERR "%s: error when allocate urb\n", DRIVER_NAME);
        return error;
    }

    mouse->usb_dev = usb_device;
    mouse->input_dev = input_dev;
    usb_make_path(usb_device, mouse->phys, sizeof(mouse->phys));
    strlcat(mouse->phys, "/input0", sizeof(mouse->phys));
    input_dev->name = DRIVER_NAME;
    input_dev->phys = mouse->phys;
    usb_to_input_id(usb_device, &input_dev->id);
    input_dev->dev.parent = &interface->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
		BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] |= BIT_MASK(BTN_SIDE) |
		BIT_MASK(BTN_EXTRA);
	input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);
    input_set_drvdata(input_dev, mouse);
    input_dev->open = my_mouse_open;
    input_dev->close = my_mouse_close;
    endpoint = &interface->cur_altsetting->endpoint[0].desc;
    usb_fill_int_urb(
        mouse->irq, usb_device,
        usb_rcvintpipe(usb_device, endpoint->bEndpointAddress),
        mouse->data, USB_PACKET_LEN,
        my_mouse_irq, mouse, endpoint->bInterval
    );
    usb_submit_urb(mouse->irq, GFP_ATOMIC);
    mouse->irq->transfer_dma = mouse->data_dma;
    mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    error = input_register_device(mouse->input_dev);
    if (error) {
        usb_free_urb(mouse->irq);
        usb_free_coherent(usb_device, USB_PACKET_LEN, mouse->data, mouse->data_dma);
        input_free_device(input_dev);
        kfree(mouse);

        printk(KERN_ERR "%s: error when register device\n", DRIVER_NAME);
        return error;
    }
    usb_set_intfdata(interface, mouse);
    printk(KERN_INFO "%s: device is connected\n", DRIVER_NAME);
    return 0;
}

static void my_mouse_disconnect(struct usb_interface *interface) {
    mouse_t *mouse = usb_get_intfdata(interface);
    usb_set_intfdata(interface, NULL);

    if (mouse) {
        usb_kill_urb(mouse->irq);
        input_unregister_device(mouse->input_dev);
        usb_free_urb(mouse->irq);
        usb_free_coherent(interface_to_usbdev(interface), USB_PACKET_LEN, mouse->data, mouse->data_dma);
        kfree(mouse);

        printk(KERN_INFO "%s: device was disconected\n", DRIVER_NAME);
    }
}

static struct usb_device_id mouse_table [] = {
    { USB_DEVICE(ID_VENDOR_MOUSE, ID_PRODUCT_MOUSE) },
    { },
};

MODULE_DEVICE_TABLE(usb, mouse_table);

static struct usb_driver my_mouse_driver = {
    .name       = DRIVER_NAME,
    .probe      = my_mouse_probe,
    .disconnect = my_mouse_disconnect,
    .id_table   = mouse_table,
};

void workHandler(struct work_struct *work)
{
	scancode = inb(KBD_DATA_REG);
    switch (kb_data.scancode) {
		case SHIFT_PR: shift = 1; break;
        case SHIFT_REL: shift = 0; break;
		default: 
			break;	
    }
	return;
}

irqreturn_t irq_handler(int irq, void *dev_id) 
{
	if (irq == KEYB_IRQ)
	{
		queue_work(workqueue, &workHandler);
		return IRQ_HANDLED;
	}
	else
		return IRQ_NONE;
}

static void my_mouse_irq(struct urb *urb) {
    urb_workqueue_t *container = kzalloc(sizeof(urb_workqueue_t), GFP_KERNEL);
    container->urb = urb;
    INIT_WORK(&container->work, work_irq);
    queue_work(workq, &container->work);
}

static int __init mouse_bright_init(void) {
    int result = usb_register(&my_mouse_driver);
    int i, j, ret;

    int return_val = request_irq(KEYB_IRQ, (irq_handler_t)irq_handler, IRQF_SHARED, "interrupt", &tmp);
	if (return_val) 
	{
		printk (KERN_DEBUG "request irq failed\n");	
		return -1;
	}

    if (result < 0) {
        printk(KERN_ERR "%s: usb register error\n", DRIVER_NAME);
        return result;
    }
    workq = create_workqueue("workqueue_mouse");
    if (workq == NULL) {
        printk(KERN_ERR "%s: allocation workqueue error\n", DRIVER_NAME);
        return -1;
    }

    work_k = create_workqueue("workqueue_keyboard");
    if (workq_k == NULL) {
        printk(KERN_ERR "%s: allocation workqueue error\n", DRIVER_NAME);
        return -1;
    }
    virtual_mouse = input_allocate_device();
    if (virtual_mouse == NULL) {
        printk(KERN_ERR "%s: allocation device error\n", DRIVER_NAME);
        return -1;
    }

    INIT_WORK(&work_k, workHandler);
    virtual_mouse->name = "virtual mouse";
    set_bit(EV_REL, virtual_mouse->evbit);
	set_bit(REL_X, virtual_mouse->relbit);
	set_bit(REL_Y, virtual_mouse->relbit);
    set_bit(EV_KEY, virtual_mouse->evbit);
    set_bit(BTN_LEFT, virtual_mouse->keybit);
	set_bit(BTN_RIGHT, virtual_mouse->keybit);

    result = input_register_device(virtual_mouse);
    if (result != 0) {
        printk(KERN_ERR "%s: registration device error\n", DRIVER_NAME);
        return result;
    }
	
	backlight_init();
    printk(KERN_INFO "brightness_controller.c: initialization complete.");
    return ret;
}

static void __exit mouse_bright_exit(void) {
    flush_workqueue(workq);
    destroy_workqueue(workq);
    input_unregister_device(virtual_mouse);
    usb_deregister(&my_mouse_driver);
    free_irq(KB_IRQ, &data);
    printk(KERN_INFO "%s: module unloaded\n", DRIVER_NAME);
}

module_init(mouse_bright_init);
module_exit(mouse_bright_exit);



