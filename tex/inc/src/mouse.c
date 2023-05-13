struct mouse_t {
    unsigned char     *data;
    struct input_dev  *input_dev;
    struct usb_device *usb_dev;
    struct urb        *irq;
};

static struct usb_driver my_mouse_driver = {
    .name       = DRIVER_NAME,
    .probe      = my_mouse_probe,
    .disconnect = my_mouse_disconnect,
    .id_table   = mouse_table,
};

struct urb_workqueue {
    struct urb *urb;
    struct work_struct work;
};

typedef struct urb_workqueue urb_workqueue_t;