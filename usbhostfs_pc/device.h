#ifndef __DEVICE_H__
#define __DEVICE_H__

typedef void* device_handle;

void device_init(void);
device_handle device_open(void);
void device_close(device_handle device);
int device_write(device_handle device, int ep, char* bytes, int size, int timeout);
int device_read(device_handle device, int ep, char* bytes, int size, int timeout);

#endif
