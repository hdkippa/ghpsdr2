// udp server for hpsdr gtk interface
// broadcasts I/Q buffers to UDT port
// meant to use to feed Gnuradio UDP source from hpsdr gtk+
// controls not implemented yet
// Copywrite Kipp A. Aldrich 05/05/2013

/* udp.h */

extern int udpFd;

// initialize the UDP I/Q server
int init_udp(int port);
// send a buffer to via the UDP I/Q server
int send_udp(float *buf, int len);
