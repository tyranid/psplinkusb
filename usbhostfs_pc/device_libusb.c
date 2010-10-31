#include <stdio.h>
#include <usb.h>
#include <unistd.h>
#include <usbhostfs.h>
#include "usbhostfs_pc.h"
#include "device.h"

#ifdef __CYGWIN__
#define NO_UID_CHECK
/* Define out set* and get* calls for cygwin as they are unnecessary and can cause issues */
#define seteuid(x)
#define setegid(x)
#define getuid()
#define getgid()
#endif

void device_init(void)
{
	usb_init();
}

void device_close(device_handle device)
{
	seteuid(0);
	setegid(0);
	if(device)
	{
		struct usb_dev_handle* hDev = (struct usb_dev_handle*)device;
		usb_release_interface(hDev, 0);
		usb_reset(hDev);
		usb_close(hDev);
	}
	seteuid(getuid());
	setegid(getgid());
}

int device_write(device_handle dev, int ep, char *bytes, int size, int timeout)
{
	int ret;

	V_PRINTF(2, "Bulk Write dev %p, ep 0x%x, bytes %p, size %d, timeout %d\n", 
			dev, ep, bytes, size, timeout);

	seteuid(0);
	setegid(0);
	ret = usb_bulk_write((struct usb_dev_handle*)dev, ep, bytes, size, timeout);
	seteuid(getuid());
	setegid(getgid());

	V_PRINTF(2, "Bulk Write returned %d\n", ret);

	return ret;
}

int device_read(device_handle dev, int ep, char *bytes, int size, int timeout)
{
	int ret;

	V_PRINTF(2, "Bulk Read dev %p, ep 0x%x, bytes %p, size %d, timeout %d\n", 
			dev, ep, bytes, size, timeout);
	seteuid(0);
	setegid(0);
	ret = usb_bulk_read((struct usb_dev_handle*)dev, ep, bytes, size, timeout);
	seteuid(getuid());
	setegid(getgid());

	V_PRINTF(2, "Bulk Read returned %d\n", ret);

	return ret;
}

usb_dev_handle *open_device(struct usb_bus *busses)
{
	struct usb_bus *bus = NULL;
	struct usb_dev_handle *hDev = NULL;

	seteuid(0);
	setegid(0);

	for(bus = busses; bus; bus = bus->next) 
	{
		struct usb_device *dev;

		for(dev = bus->devices; dev; dev = dev->next)
		{
			if((dev->descriptor.idVendor == SONY_VID) 
				&& (dev->descriptor.idProduct == g_pid))
			{
				hDev = usb_open(dev);
				if(hDev != NULL)
				{
					int ret;
					ret = usb_set_configuration(hDev, 1);
#ifndef NO_UID_CHECK
					if((ret < 0) && (errno == EPERM) && geteuid()) {
						fprintf(stderr,
							"Permission error while opening the USB device.\n"
							"Fix device permissions or run as root.\n");
						usb_close(hDev);
						exit(1);
					}
#endif
					if(ret == 0)
					{
						ret = usb_claim_interface(hDev, 0);
						if(ret == 0)
						{
							seteuid(getuid());
							setegid(getgid());
							return hDev;
						}
						else
						{
							usb_close(hDev);
							hDev = NULL;
						}
					}
					else
					{
						usb_close(hDev);
						hDev = NULL;
					}
				}
			}
		}
	}
	
	if(hDev)
	{
		usb_close(hDev);
	}

	seteuid(getuid());
	setegid(getgid());

	return NULL;
}

device_handle device_open(void)
{
	usb_find_busses();
	usb_find_devices();

	return (device_handle) open_device(usb_get_busses());
}

