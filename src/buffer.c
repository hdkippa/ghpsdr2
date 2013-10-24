/******************************************************************************
 * Project: hpsdr
 *
 * File: buffer.c 
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

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#include "list.h"
#include "buffer.h"
#include "libusbio.h"

void buf_print_list(usbBufPool_t *pool, const char *name);

static int SERIAL=0;

//########### usbBufPool_t ##########//
usbBuf_t *buf_create_buffer(size_t size)
{
	usbBuf_t *entry = (usbBuf_t *)malloc(sizeof(usbBuf_t));
	if (!entry)
	{
		fprintf(stderr, "%s: malloc oxyBuf_t failed\n", __func__);
		return NULL;
	}
	memset(entry, 0, sizeof(*entry));
	INIT_LIST_HEAD(&entry->list); // point at nothing
	INIT_LIST_HEAD(&entry->inflight); 

	entry->buf = malloc(size);
	if (!entry->buf)
	{
		fprintf(stderr, "%s: malloc buf size %d fhttp://publib.boulder.ibm.com/infocenter/iseries/v5r4/index.jsp?topic=%2Fapis%2Fusers_76.htmailed\n", __func__, size);
		free(entry);
		return NULL;
	}
	memset(entry->buf, 0, size);

	entry->size = size;
	entry->id = SERIAL++;

	// and make a libusb_transfer
	entry->xfr = libusb_alloc_transfer(128);
	if (!entry->xfr)
	{
		fprintf(stderr, "%s: libusb_alloc_transfer failed!\n", __func__);
		free(entry->buf);
		free(entry);
		return NULL;
	}

	//fprintf(stderr, "%s:entry %p p %p n %p id %d buf %p \n", __func__, entry, entry->list.prev, entry->list.next, entry->id, entry->buf);

	return entry;
}

void buf_cancel_inflight(usbBufPool_t *pool)
{
	int rc=0;
	usbBuf_t *tmp;
	struct list_head *pos, *q;
	struct timeval tv = {0, 5000};

	if (pool)
	{
		// cancel the transactions on struct list_head inflight
		pthread_mutex_lock(&pool->lock);
		list_for_each_safe(pos, q, &pool->ob->inflight)
		{
			tmp = list_entry(pos, usbBuf_t, inflight);
			if (tmp)
			{
				if (tmp->xfr)
				{
					// what if it's not inflight?
					fprintf(stderr, "%s: 0 cancel bufid %d \n", __func__, tmp->id);
					rc = libusb_cancel_transfer(tmp->xfr);
					if (rc<0)
						fprintf(stderr, "%s: 1 libusb_cancel_transfer bufid %d error %s\n", __func__, tmp->id, libusb_error_name(rc));
					if (rc == LIBUSB_ERROR_NOT_FOUND) // it's not in flight but is on the inflight list, so fly it
					{
						continue;
						/*
						list_del_init(&tmp->inflight);
						list_add_tail(&tmp->list, &pool->ob->list);
						*/
					}

				}
			}
		}
		pthread_mutex_unlock(&pool->lock);
	}
}

void buf_destroy_buffer_pool(usbBufPool_t *pool)
{
	struct list_head *pos, *q;
	usbBuf_t *tmp;

	if (pool)
	{
		pthread_mutex_lock(&pool->lock);
		list_for_each_safe(pos, q, &pool->ob->list) 
		{
			tmp = list_entry(pos, usbBuf_t, list);
			fprintf(stderr, "%s: delete bufid %d\n", __func__, tmp->id);
			if (tmp)
			{
				if(tmp->buf)
					free(tmp->buf);
				if (tmp->xfr)
				{
					libusb_free_transfer(tmp->xfr);
				}
				list_del(pos);
				free(tmp);
			}
		}
		if (pool->ob)
			free(pool->ob);
		free(pool);
		pool=NULL;
		// no more list to unlock!
		//pthread_mutex_unlock(&tmp->lock);
	}
}

void buf_print_list(usbBufPool_t *pool, const char *name)
{
	struct list_head *pos, *q;
	usbBuf_t *tmp;

	if (!name)
		name=">";

	//pthread_mutex_lock(&pool->lock);
	fprintf(stderr, "%s: FREE_LIST (%s): (%d in list) list_empty %d\n", 
		__func__, name, buf_count_buffers(pool), list_empty(&pool->ob->list));
	list_for_each(pos, &pool->ob->list) 
	{
		tmp = list_entry(pos, usbBuf_t, list);
		if (tmp)
		{
			fprintf(stderr, "%s: %p p %p n %p id %d buf %p\n", __func__, pos, pos->prev, pos->next, tmp->id, tmp->buf);
		}
	}
	fprintf(stderr, "%s: BUSY_LIST (%s): (%d in list)\n", __func__, name, buf_count_inflight(pool));
	list_for_each(pos, &pool->ob->inflight) 
	{
		tmp = list_entry(pos, usbBuf_t, inflight);
		if (tmp)
		{
			fprintf(stderr, "%s: %p p %p n %p id %d buf %p\n", __func__, pos, pos->prev, pos->next, tmp->id, tmp->buf);
		}
	}
	//pthread_mutex_unlock(&list->lock);
}

	
usbBufPool_t *buf_create_pool(int num_bufs, size_t size, const char *name)
{
	int rc, i;
	usbBufPool_t *pool;
	if (!name) 
		name = ">";

	//fprintf(stderr, "%s: %s num_bufs %d size %d\n", __func__, name, num_bufs, size);

	pool = (usbBufPool_t *)malloc(sizeof(usbBufPool_t));
	if (!pool)
	{
		fprintf(stderr, "%s: malloc usbBufPool_t failed\n", __func__);
		return NULL;
	}
	memset (pool, 0, sizeof(usbBufPool_t));
	pool->name = name;

	rc = pthread_mutex_init(&pool->lock, NULL); // unlocked
	if (rc<0)
	{
		free(pool);
		fprintf(stderr, "%s: pthread_mutex_init usbBufPool_t failed rc=%d\n", __func__, rc);
		return NULL;
	}

	pool->ob = buf_create_buffer(size);
	if (!pool->ob)
	{
		free(pool);
		fprintf(stderr, "%s: base buffer create failed\n", __func__);
		return NULL;
	}

	pthread_mutex_lock(&pool->lock);
	for (i=0; i<num_bufs; i++)
	{
		char buf[32];
		usbBuf_t *entry = buf_create_buffer(size);
		if (entry)
		{
			list_add_tail(&entry->list, &pool->ob->list);
		}
	}

	//buf_print_list(pool, "CREATE");

	pthread_mutex_unlock(&pool->lock);

	return pool;
}

usbBuf_t *buf_get_buffer(usbBufPool_t *pool)
{
	struct list_head *pos;
	usbBuf_t *tmp;

	if (!pool)
	{
		fprintf(stderr, "%s: No pool!\n", __func__);
		return NULL;
	}

	tmp=NULL;

	pthread_mutex_lock(&pool->lock);
	list_for_each(pos, &pool->ob->list) 
	{
		tmp = list_entry(pos, usbBuf_t, list);
		break;
	}

	if (tmp)
	{
		list_del_init(&tmp->list);
		list_add_tail(&tmp->inflight, &pool->ob->inflight);
	}
	pthread_mutex_unlock(&pool->lock);

	return tmp;
}

void buf_put_buffer(usbBufPool_t *pool, usbBuf_t *entry)
{
	if (!pool || !entry)
		return;

	pthread_mutex_lock(&pool->lock);
	list_del_init(&entry->inflight);
	list_add_tail(&entry->list, &pool->ob->list);
	pthread_mutex_unlock(&pool->lock);
}

int buf_count_buffers(usbBufPool_t *pool)
{
	struct list_head *pos;
	int count;

	if (!pool)
		return -1;

	count=0;
	pthread_mutex_lock(&pool->lock);
	list_for_each(pos, &pool->ob->list) 
	{
		count++;
	}
	pthread_mutex_unlock(&pool->lock);

	return count;
}

int buf_count_inflight(usbBufPool_t *pool)
{
	struct list_head *pos;
	int count;

	if (!pool)
		return -1;

	count=0;
	pthread_mutex_lock(&pool->lock);
	list_for_each(pos, &pool->ob->inflight) 
	{
		count++;
	}
	pthread_mutex_unlock(&pool->lock);

	return count;
}
