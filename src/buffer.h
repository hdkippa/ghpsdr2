/******************************************************************************
 * Project: hpsdr
 *
 * File: buffer.h 
 *
 * Description: implement a simple buffer pool manager to avoid allocs/deallocs
 *				during operations. Alos used to minimize blocking IO time for USB
 *
 * Date: Mon 25 Feb 2013
 *
 * $Revision:$
 *
 * $Header:$
 *
 * Copyright (C) 2013 Kipp A. Aldrich <kippaldirch@gmail.com>
 ******************************************************************************
 *
 * $Log:$
 *
 ******************************************************************************
 */

#ifndef _OZYBUFIO_H
#define _OZYBUFIO_H

// stole the kernel's linked lists
#include <stddef.h> // size_t
#include <pthread.h>
#include "list.h"

#define OZY_BUFFER_SIZE 512
#define OZY_NUM_BUFS 32 // 1024 samples/OZY_BUFFER_SIZE (minimum samples/buffers to send to hpsdr at a time)  

typedef struct usbBuf
{
	// stolen kernel linked list for elegance and familiarity
	struct list_head list; // available buffers 
	struct list_head inflight; // in flight buffers 
	void *buf;
	int id;
	size_t size;
	struct libusb_transfer *xfr;
} usbBuf_t;

typedef struct usbBufPool 
{
	pthread_mutex_t lock;
	usbBuf_t *ob;
	const char *name;
} usbBufPool_t;

extern void  buf_destroy_buffer_pool(usbBufPool_t *list);
extern usbBufPool_t *buf_create_pool(int num_bufs, size_t size, const char *name);
extern int buf_init_buffer_pools(void);
extern usbBuf_t *buf_create_buffer(size_t size);
extern usbBuf_t *buf_get_buffer(usbBufPool_t *pool);
extern void buf_put_buffer(usbBufPool_t *pool, usbBuf_t *entry);
extern int buf_count_buffers(usbBufPool_t *pool);
extern int buf_count_inflight(usbBufPool_t *pool);
extern void buf_cancel_inflight(usbBufPool_t *pool);

#endif /* _OZYBUFIO_H */

