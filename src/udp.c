// udp server for hpsdr gtk interface
// broadcasts I/Q buffers to UDT port
// meant to use to feed Gnuradio UDP source from hpsdr gtk+
// controls not implemented yet
// Copywrite Kipp A. Aldrich 05/05/2013

/* udp.c */ 

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "udp.h"
#include <unistd.h>


// defines 
#define UDP_PORT 3456
#define NAME_BUF_SIZE 1024
#define SERV_BCST_ADDR "255.255.255.255"

int udpFd;

static struct hostent *this_host;
static struct sockaddr_in udpAddr, dstAddr;

static char this_hostname[NAME_BUF_SIZE]; 

static int sockaddr_size=sizeof(struct sockaddr_in);

int init_udp(int port)
{
	int fd;
	int ret;
	int broadcastEnable;

	fprintf(stderr, "%s: port %d sockaddr_size %d\n", __func__, port>0?port:UDP_PORT, sockaddr_size);

	if ((udpFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf(stderr, "%s: socket failed\n", __func__);
		return -1;
	}

	broadcastEnable=1;
	ret=setsockopt(udpFd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
	if (ret)
	{
		fprintf(stderr, "%s: setsockopt SO_BROADCAST failed\n", __func__);
		close(udpFd);
		udpFd=0;
		return -1;
	}

	ret=setsockopt(udpFd, SOL_SOCKET, SO_REUSEADDR, &broadcastEnable, sizeof(broadcastEnable));
	if (ret)
	{
		fprintf(stderr, "%s: setsockopt SO_REUSEADDR failed\n", __func__);
		close(udpFd);
		udpFd=0;
		return -1;
	}

	/* set up IP and port */
	memset(&udpAddr,0,sizeof(udpAddr));
	udpAddr.sin_family = AF_INET;
	udpAddr.sin_port=htons(port > 0 ? port : UDP_PORT);
	udpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//udpAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	//udpAddr.sin_addr.s_addr = inet_addr(SERV_BCST_ADDR);

	if (bind(udpFd, (struct sockaddr*)&udpAddr, sizeof(udpAddr))<0)
	{
		fprintf(stderr, "%s: bind socket failed for udpFd\n", __func__);
		return -1;
	}

	// also set up and maintain the client address, since it's broadcast we do it here
	/*
	if (gethostname(this_hostname, NAME_BUF_SIZE)<0)
	{
		fprintf(stderr, "%s: gethostname failed\n", __func__);
		return -1;
	}

	if (!this_hostname)
	{
		fprintf(stderr, "%s: this_hostname is NULL\n", __func__);
		return -1;
	}

	fprintf(stderr, "%s: this_hostname is %s\n", __func__, this_hostname);

	if ((this_host=gethostbyname(this_hostname))==NULL)
	{
		fprintf(stderr, "%s: gethostbyname failed for udpFd (%s)\n", __func__, strerror(errno));
		//return -1;
	}
	*/

	dstAddr.sin_family = AF_INET;
	dstAddr.sin_port=htons(port > 0 ? port : UDP_PORT);
	dstAddr.sin_addr.s_addr = inet_addr(SERV_BCST_ADDR); 
	//dstAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	return 0;
}

int send_udp(float *buf, int len)
{
	int x;
	int ret;

	// broadcast our data on the local network
	x=len;
	while (x)
	{
		ret = sendto(udpFd, buf, x, 0,(struct sockaddr *) &dstAddr, sockaddr_size);
		if (ret < 0)
		{
			// check for interrupts!
			perror("sendto");
			return -1;
		}
		x -= ret;
		//fprintf(stderr, "%s: sendto %d bytes\n", __func__, ret);
	}	
	

	// receive the data back from the broadcast so we don't fill our local queues???
	/*
	x=len;
	do
	{
		ret = recvfrom(udpFd, buf, x, 0, (struct sockaddr *)&udpAddr, &sockaddr_size);
		if (ret < 0)
		{
			// check for interrupts!
			perror("sendto");
			return -1;
		}
		x += ret;
		//fprintf(stderr, "%s: recvfrom %d bytes\n", __func__, ret);
	}while(x<len);
	*/

	return ret;
}
