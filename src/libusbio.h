/** 
* @file libusbio.h
* @brief Header file for the USB I/O functions, interface to libusb1.0
* @author 
* @version 0.1
* @date 2009-05-18
*/

#ifndef _LIBUSBIO_H
#define _LIBUSBIO_H

#include <libusb-1.0/libusb.h>

// the hpsdr ozy usb endpoints, from ozy specification manual
#define EP_TX_DATA		(2 | LIBUSB_ENDPOINT_OUT)
#define EP_RX_BSDATA	(4 | LIBUSB_ENDPOINT_IN)
#define EP_RX_DATA		(6 | LIBUSB_ENDPOINT_IN)

int libusb_open_ozy(void);
void libusb_close_ozy(void);
int libusb_get_ozy_firmware_string(char* buffer,int buffersize);
int libusb_xfer_ozy(int ep,void* buffer,int buffersize, void *user_data);

extern int usb_initialized;
extern libusb_context *usb_context;
extern struct libusb_transfer *usb_ctl;

#endif /* _LIBUSBIO_H */

