/* 
 * File:   libusbio.c
 * Author: jm57878
 *
 * Created on 18 February 2009, 21:16
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>

#include "ozy.h"
#include "libusbio.h"
#include "spectrum_buffers.h"
#include "list.h" // the rip of the kernel's linked list

/*
 *  interface to libusb1.0
 */

#define OZY_PID (0x0007)
#define OZY_VID (0xfffe)

#define VRQ_SDR1K_CTL 0x0d
#define SDR1KCTRL_READ_VERSION  0x7
#define VRT_VENDOR_IN 0xC0

#define OZY_IO_TIMEOUT	500

#define ISOS 1

// externs to others
int usb_initialized=0;
libusb_context *usb_context=NULL;
libusb_device_handle* usb_handle=NULL;

static int do_exit=0;

static pthread_t usb_event_reaper_thread_id;
static pthread_cond_t exit_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t exit_cond_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_mutex_t exit_cond_lock = PTHREAD_MUTEX_INITIALIZER;

struct libusb_transfer *usb_ctl=NULL;
struct libusb_transfer *usb_xfr=NULL;

static void request_exit(int code)
{
	//fprintf(stderr,"%s:exit requested code %d\n", __func__, code);
	pthread_mutex_lock(&exit_cond_lock);
	do_exit = code;
	pthread_cond_signal(&exit_cond);
	pthread_mutex_unlock(&exit_cond_lock);
}

void *usb_event_reaper(void *arg)
{
	while(!do_exit)
	{
		int rc;
		struct timeval tv = {0, 1000};
		rc=libusb_handle_events_timeout(usb_context, &tv);
		if (rc<0)
		{
			printf("%s: FAILRE %s do_exit %d\n", __func__, libusb_error_name(rc), do_exit);
			request_exit(rc);
			break;
		}
	}
	return NULL;
}

static int find_ozy(void)
{
    usb_handle=libusb_open_device_with_vid_pid(usb_context, OZY_VID, OZY_PID);
	return usb_handle ? 0 : -EIO;
}

void libusb_shutdown_ozy() 
{
	if (usb_ctl)
		libusb_free_transfer(usb_ctl);

	if (usb_handle)
		libusb_close(usb_handle);
		// all other transfers are freed in ozy_destroy_buffer_pool

	if (usb_context)
		libusb_exit(usb_context);

/*
	request_exit(1);
	pthread_mutex_lock(&exit_cond_lock);
	while(!do_exit)
	{
		pthread_cond_wait(&exit_cond, &exit_cond_lock);
	}
	pthread_mutex_unlock(&exit_cond_lock);
	pthread_join(usb_event_reaper_thread_id, NULL);
	*/
}

int libusb_open_ozy(void) 
{
	int rc;

	if(!usb_initialized) 
	{
		if ((rc = libusb_init(&usb_context))<0)
		{
			fprintf(stderr,"libusb_init failed: %d\n", rc);
			return rc;
		}
	}

	if((rc = find_ozy())<0)
	{
		fprintf(stderr, "Could not find Hpsdr Ozy USB interface\n");
		libusb_close(usb_handle);
		return rc;
	}

	if ((rc = libusb_claim_interface(usb_handle,0))<0)
	{
		fprintf(stderr,"libusb_claim_interface failed: %d\n",rc);
		libusb_close(usb_handle);
		return rc;
	}

	if (!(usb_ctl = libusb_alloc_transfer(0)))
	{
		fprintf(stderr, "%s: libusb_alloc_transfer usb_ctl failed!\n", __func__);
		libusb_shutdown_ozy();
		return -1;
	}

	if (!(usb_xfr = libusb_alloc_transfer(ISOS)))
	{
		fprintf(stderr, "%s: libusb_alloc_transfer usb_xfr failed!\n", __func__);
		libusb_shutdown_ozy();
		return -1;
	}

	rc = pthread_cond_init(&exit_cond, NULL); // default value
	if (rc)
	{
		fprintf(stderr, "%s: pthread_cond_init exit_cond failed rc=%dn", __func__, rc);
		libusb_shutdown_ozy();
		return -1;
	}

	usb_initialized=1;

	rc=pthread_create(&usb_event_reaper_thread_id,NULL,usb_event_reaper,NULL);
	if(rc != 0) 
	{
		fprintf(stderr,"pthread_create failed on usb_event_reaper: rc=%d\n", rc);
	}

	return 0;
}

void libusb_handle_cb(struct libusb_transfer *ux)
{
/*
	if (ux->user_data)
	fprintf(stderr, "%s: proc %d ID %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d actual length %d\n", 
			__func__, getpid(), ((usbBuf_t *)ux->user_data)->id, ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length, ux->actual_length);
	else
	fprintf(stderr, "%s: proc %d NO ID flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
			__func__,  getpid(), ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);
*/

	if (!ux->user_data)
	{
		fprintf(stderr, "%s: NO USER DATA pid %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
			__func__,  getpid(), ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);
		return;
	}

	usbBuf_t *ob = (usbBuf_t *)ux->user_data;

	switch(ux->status)
	{
		case LIBUSB_TRANSFER_COMPLETED:
			{
				switch(ux->endpoint)
				{
					case 0x00: /* control endpoint completed */
					{
						//usbBuf_t *ob = (usbBuf_t *)ux->user_data;
						//ozy_put_buffer(ozyRxBufs, ob);
						break;
					}
					case 0x02: /* write to ozy endpoint completed */
					{
						//fprintf(stderr, "%s: oxyTxBuf id %d\n", __func__, ob->id);
						//ozy_put_buffer(ozyTxBufs, ob);
						break;
					}
					case 0x86: /* 512 byte data read completed */
					{
						//fprintf(stderr, "%s: oxyRxBuf id %d\n", __func__, ob->id);
						process_ozy_input_buffer((char *)ux->buffer);
						//ozy_put_buffer(ozyRxBufs, ob);
						break;
					}
					case 0x84: /* band scope 8192 bytes read */
					{
						//fprintf(stderr, "%s: oxyBxBuf id %d\n", __func__, ob->id);
						process_bandscope_buffer(ob->buf);
						//ozy_put_buffer(ozyBxBufs, ob);
						break;
					}
					default:
					{
						fprintf(stderr, "%s: Unknown ep 0x%x\n", __func__, ux->endpoint);
						break;

					}
				} // end switch(endpoint)
				break;
			}
		case LIBUSB_TRANSFER_ERROR:
			{
				fprintf(stderr, "%s: LIBUSB_TRANSFER_ERROR pid %d ID %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
					__func__,  getpid(), ((usbBuf_t *)ux->user_data)->id, ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);
				break;
			}
		case LIBUSB_TRANSFER_TIMED_OUT:
			{
				//fprintf(stderr, "%s: LIBUSB_TRANSFER_TIMED_OUT pid %d ID %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
				//__func__,  getpid(), ((usbBuf_t *)ux->user_data)->id, ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);
				break;
			}
		case LIBUSB_TRANSFER_CANCELLED:
			{
				fprintf(stderr, "%s: LIBUSB_TRANSFER_CANCELLED pid %d ID %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
						__func__,  getpid(), ((usbBuf_t *)ux->user_data)->id, ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);

				break;
			}
		case LIBUSB_TRANSFER_STALL:
			{
				fprintf(stderr, "%s: LIBUSB_TRANSFER_STALL pid %d ID %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
					__func__,  getpid(), ((usbBuf_t *)ux->user_data)->id, ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);
				break;
			}
		case LIBUSB_TRANSFER_NO_DEVICE:
			{
				fprintf(stderr, "%s: LIBUSB_TRANSFER_NO_DEVICE pid %d ID %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
					__func__,  getpid(), ((usbBuf_t *)ux->user_data)->id, ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);
				break;
			}
		case LIBUSB_TRANSFER_OVERFLOW:
			{
				fprintf(stderr, "%s: LIBUSB_TRANSFER_OVERFLOW pid %d ID %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
					__func__,  getpid(), ((usbBuf_t *)ux->user_data)->id, ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);
				break;
			}
		default:
			{
				fprintf(stderr, "%s: UNKNOWN STATUS pid %d ID %d flags 0x%2.2X ep 0x%2.2x type 0x%2.2x timeout %d status 0x%2.2x length %d\n", 
					__func__,  getpid(), ((usbBuf_t *)ux->user_data)->id, ux->flags, ux->endpoint, ux->type, ux->timeout, ux->status, ux->length);
			}
	}

	// put the buffer back into the correct buffer pool
	switch(ux->endpoint)
	{
		case 0x00: /* control endpoint completed */
		{
			//ozy_put_buffer(ozyRxBufs, ob);
			break;
		}
		case 0x02: /* write to ozy endpoint completed */
		{
			buf_put_buffer(ozyTxBufs, ob);
			break;
		}
		case 0x86: /* 512 byte data read completed */
		{
			buf_put_buffer(ozyRxBufs, ob);
			break;
		}
		case 0x84: /* band scope 8192 bytes read */
		{
			buf_put_buffer(ozyBxBufs, ob);
			break;
		}
		default:
		{
			fprintf(stderr, "%s: Unknown ep 0x%x\n", __func__, ux->endpoint);
			break;
		}
	} 

}

int libusb_get_ozy_firmware_string(char* buffer,int buffersize) {
	int rc;
	unsigned char setup_pkt[8];

	libusb_fill_control_setup(setup_pkt, VRT_VENDOR_IN, VRQ_SDR1K_CTL, SDR1KCTRL_READ_VERSION, 0, buffersize);
	libusb_fill_control_transfer(usb_ctl, usb_handle, setup_pkt, libusb_handle_cb, NULL, OZY_IO_TIMEOUT);
	rc=libusb_submit_transfer(usb_ctl);
	if(rc<0) 
	{
		fprintf(stderr,"libusb_fill_control_setup failed: %d\n",rc);
	}
	libusb_handle_events(usb_context);
    buffer[rc]=0x00;

	libusb_get_max_iso_packet_size(libusb_get_device(usb_handle), 0x02);
	libusb_get_max_iso_packet_size(libusb_get_device(usb_handle), 0x84);
	libusb_get_max_iso_packet_size(libusb_get_device(usb_handle), 0x86);

    return rc;
}

int  libusb_xfer_buf(int ep,void* buffer,int buffersize, void *user_data)
{
	int rc=0;

	if (!user_data)
	{
		fprintf(stderr,"%s: No user_data!\n", __func__);
		return -1;
	}

	usbBuf_t *ob = (usbBuf_t *)user_data;

	libusb_fill_bulk_transfer(ob->xfr, 
		usb_handle, 
		(unsigned char)ep, 
		(unsigned char *)buffer, 
		buffersize, 
		libusb_handle_cb, 
		user_data,
		1200);

	//libusb_set_iso_packet_lengths(ob->xfr, 4);

tryagain:

	rc=libusb_submit_transfer(ob->xfr);
    if(rc<0) 
	{
        fprintf(stderr,"%s: libusb_submit_transfer failed: buf %d rc=%d (%s)\n", __func__, ob->id, rc, libusb_error_name(rc));
		if (rc==-6) // busy, try again
		{
			struct timeval tv = {0, 5000};
			fprintf(stderr,"%s: Calling libusb_handle_events_timeout buf %d\n", __func__, ob->id);
			rc = libusb_handle_events_timeout(usb_context, &tv);
			fprintf(stderr,"%s: buf %d rc=%d (%s)\n", __func__, ob->id, rc, libusb_error_name(rc));
			goto tryagain;
		}
    }

	return rc;
}
