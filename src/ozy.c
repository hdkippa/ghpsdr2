/* 
 * File:   ozy.c
 * Author: jm57878
 *
 * Created on 10 March 2009, 20:26
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include <sys/timeb.h>
#include <math.h>

#include <gtk/gtk.h>

#include "ozy.h"
#include "buffer.h"
#include "property.h"
#include "spectrum_buffers.h"
#include "dttsp.h"
#include "libusbio.h"
#include "filter.h"
#include "volume.h"
#include "mode.h"
#include "audiostream.h"
#include "transmit.h"
#include "main.h"
#include "sinewave.h"
#include "vfo.h"
#include "metis.h"
#include "cw.h"
#include "udp.h"

/*
 *   ozy interface
 */

#define USB_TIMEOUT -7
//static struct OzyHandle* ozy;

#define LIBUSB_TIMEOUT_SECS 0
#define LIBUSB_TIMEOUT_USECS 5000

static char ozy_firmware_version[9];
int mercury_software_version=0;
int penelope_software_version=0;
int ozy_software_version=0;

int penelopeForwardPower=0;
int alexForwardPower=0;
int alexReversePower=0;
int AIN3=0;
int AIN4=0;
int AIN6=0;
int IO1=1; // 1 is inactive
int IO2=1;
int IO3=1;


static int ep6_ep2_io_thread_exit_code=0;
static pthread_t ep6_ep2_io_thread_id;
static pthread_cond_t ep6_ep2_io_thread_exit_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t ep6_ep2_io_thread_exit_cond_lock = PTHREAD_MUTEX_INITIALIZER;

static int ep4_io_thread_exit_code=0;
static pthread_t ep4_io_thread_id;
static pthread_cond_t ep4_io_thread_exit_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t ep4_io_thread_exit_cond_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_t ozy_input_buffer_thread_id;
static pthread_t ozy_spectrum_buffer_thread_id;

static long rxFrequency=7056000;
static int rxFrequency_changed=1;
static long txFrequency=7056000;
static int txFrequency_changed=1;

int output_buffer_index=8;

unsigned char bandscope_buffer[8192];
int bandscope_buffer_index=0;

static int force_write=0;

static unsigned char control_in[5]={0x00,0x00,0x00,0x00,0x00};

unsigned char control_out[5]={0x00,0x00,0x00,0x00,0x00};

// the main startup sampleRate TODO FIXME
int sampleRate=96000;  // default 48k
static int output_sample_increment=2; // 1=48000 2=96000 4=192000

const int buffer_size=BUFFER_SIZE;

float left_input_buffer[BUFFER_SIZE];
float right_input_buffer[BUFFER_SIZE];

float record_buffer[BUFFER_SIZE*2];
float udp_buffer[BUFFER_SIZE*2];

float mic_left_buffer[BUFFER_SIZE];
float mic_right_buffer[BUFFER_SIZE];

float left_output_buffer[BUFFER_SIZE];
float right_output_buffer[BUFFER_SIZE];

float left_tx_buffer[BUFFER_SIZE];
float right_tx_buffer[BUFFER_SIZE];

int start_samples, udp_samples=0, rec_samples=0, samples=0;

int left_sample;
int right_sample;
int mic_sample;


float left_sample_float;
float right_sample_float;
float mic_sample_float;

short left_rx_sample;
short right_rx_sample;
short left_tx_sample;
short right_tx_sample;

int usb_output_buffers=0;

int show_software_serial_numbers=1;

unsigned char spectrum_samples[SPECTRUM_BUFFER_SIZE];

int lt2208ADCOverflow=0;

int speed=1;           // default 96K
int class=0;           // default other
int lt2208Dither=1;    // default dither on
int lt2208Random=1;    // default random 0n
int alexAttenuation=0; // default alex attenuation 0Db
int micSource=1;       // default mic source Penelope
int clock10MHz=2;      // default 10 MHz clock source Mercury
int clock122_88MHz=1;  // default 122.88 MHz clock source Mercury
int preamp=0;          // default preamp off

int mox=0;
int ptt=0;
int dot=0;
int dash=0;

int xmit=0;  // not transmitting

int pennyLane=0;       // default is Penelope
int driveLevel=255;    // default is max
int driveLevelChanged=0; // force drive level to be sent

int timing=0;
static struct timeb start_time;
static struct timeb end_time;
struct timeval endtime;
struct timeval starttime;

static int sample_count=0;

static struct timeval st_time, en_time, diff_time;
static long lowt, hit;

static int metis=0;
static char interface[128];

int alexRxAntenna=0;
int alexTxAntenna=0;
int alexRxOnlyAntenna=0;

float vswr=0.0;

int debug_control1=0;

// available buffers
usbBufPool_t *ozyRxBufs, *ozyTxBufs, *ozyBxBufs;

void ozy_set_metis() {
    metis=1;
}

void ozy_set_interface(char* iface) {
    strcpy(interface,iface);
}

void setPennyLane(int state) {
    pennyLane=state;
}

void setDriveLevelChanged(int level) {
    driveLevel=level;
    driveLevelChanged=1;
}

char* ozy_get_interface() {
    return interface;
}

void process_bandscope_buffer(char* buffer) 
{
	int i;

	for(i=0;i<512;i++) {
		bandscope_buffer[bandscope_buffer_index++]=buffer[i];
	}

	if(bandscope_buffer_index>=SPECTRUM_BUFFER_SIZE) {
		memcpy(spectrum_samples,bandscope_buffer,SPECTRUM_BUFFER_SIZE);
		bandscope_buffer_index=0;
	}
	memcpy(spectrum_samples,buffer,SPECTRUM_BUFFER_SIZE);
}

static long long
timeval_diff(struct timeval *difference,
             struct timeval *end_time,
             struct timeval *start_time
            )
{
	struct timeval temp_diff;

	if(difference==NULL)
	{
		difference=&temp_diff;
	}

	difference->tv_sec =end_time->tv_sec -start_time->tv_sec ;
	difference->tv_usec=end_time->tv_usec-start_time->tv_usec;

	/* Using while instead of if below makes the code slightly more robust. */

	while(difference->tv_usec<0)
	{
		difference->tv_usec+=1000000;
		difference->tv_sec -=1;
	}

	return 1000000LL*difference->tv_sec+difference->tv_usec;

} /* timeval_diff() */

/* --------------------------------------------------------------------------*/
/** 
* @brief Process the ozy input buffer
* 
* @param buffer
*/
static usbBuf_t *ob=NULL;

void process_ozy_input_buffer(char* buffer) 
{
	int i,j;
	int b=0;
	unsigned char ozy_samples[8*8];
	int bytes;

	int last_ptt;
	int last_dot;
	int last_dash;
	int last_xmit;

	double gain;

	char *buf;

	if(buffer[b++]==SYNC && buffer[b++]==SYNC && buffer[b++]==SYNC) 
	{
		//fprintf(stderr, "%s: SYNC SYNC SYNC\n", __func__);
		// extract control bytes
		control_in[0]=buffer[b++];
		control_in[1]=buffer[b++];
		control_in[2]=buffer[b++];
		control_in[3]=buffer[b++];
		control_in[4]=buffer[b++];

		last_ptt=ptt;
		last_dot=dot;
		last_dash=dash;
		ptt=(control_in[0]&0x01)==0x01;
		dash=(control_in[0]&0x02)==0x02;
		dot=(control_in[0]&0x04)==0x04;

		last_xmit=xmit;
		xmit=mox|ptt|dot|dash;
		if (xmit)
		{
			fprintf(stderr, "mox %d ptt %d dot %d dash %d\n", mox, ptt, dot, dash);

			//int *vfoState=malloc(sizeof(int));
			//*vfoState=xmit;
			//g_idle_add(vfoTransmit,(gpointer)vfoState);
		}
		switch((control_in[0]>>3)&0x1F) 
		{

			case 0:
				lt2208ADCOverflow=control_in[1]&0x01;
				IO1=(control_in[1]&0x02)?0:1;
				IO2=(control_in[1]&0x04)?0:1;
				IO3=(control_in[1]&0x08)?0:1;
				if(mercury_software_version!=control_in[2]) 
				{
					mercury_software_version=control_in[2];
					fprintf(stderr,"  Mercury Software version: %d (0x%0X)\n",mercury_software_version,mercury_software_version);
				}
				if(penelope_software_version!=control_in[3])
				{
					penelope_software_version=control_in[3];
					fprintf(stderr,"  Penelope Software version: %d (0x%0X)\n",penelope_software_version,penelope_software_version);
				}
				if(ozy_software_version!=control_in[4]) 
				{
					ozy_software_version=control_in[4];
					fprintf(stderr,"  Ozy Software version: %d (0x%0X)\n",ozy_software_version,ozy_software_version);
				}
				break;
			case 1:
				penelopeForwardPower=(control_in[1]<<8)+control_in[2]; // from Penelope or Hermes

				alexForwardPower=(control_in[3]<<8)+control_in[4]; // from Alex or Apollo
				break;
			case 2:
				alexReversePower=(control_in[1]<<8)+control_in[2]; // from Alex or Apollo
				AIN3=(control_in[3]<<8)+control_in[4]; // from Pennelope or Hermes
				break;
			case 3:
				AIN4=(control_in[1]<<8)+control_in[2]; // from Pennelope or Hermes
				AIN6=(control_in[3]<<8)+control_in[4]; // from Pennelope or Hermes
				break;
		}

		if(xmit) 
		{
			float fwd=(float)alexForwardPower/100.0F;
			float rev=(float)alexReversePower/100.0F;

			float gamma=sqrt(rev/fwd);
			vswr=(1.0F+gamma)/(1.0F-gamma);
			//fprintf(stderr,"fwd=%f rev=%f vswr=%f\n",fwd,rev,vswr);
		}
		// extract the 63 samples
		start_samples = samples;
		for(i=0;i<63;i++) 
		{

			left_sample   = (int)((signed char) buffer[b++]) << 16;
			left_sample  += (int)((unsigned char)buffer[b++]) << 8;
			left_sample  += (int)((unsigned char)buffer[b++]);

			right_sample  = (int)((signed char) buffer[b++]) << 16;
			right_sample += (int)((unsigned char)buffer[b++]) << 8;
			right_sample += (int)((unsigned char)buffer[b++]);

			mic_sample    = (int)((signed char) buffer[b++]) << 8;
			mic_sample   += (int)((unsigned char)buffer[b++]);

			left_sample_float  = (float)left_sample/8388607.0; // 24 bit sample
			right_sample_float = (float)right_sample/8388607.0; // 24 bit sample
			mic_sample_float   = ((float)mic_sample/32767.0f)*micGain; // 16 bit sample

			// add to buffer
			if(xmit || mute) 
			{ // mute the input
				left_input_buffer[samples]=0.0;
				right_input_buffer[samples]=0.0;
			} 
			else 
			{
				left_input_buffer[samples]=left_sample_float;
				right_input_buffer[samples]=right_sample_float;
			}

			if(testing) 
			{
				mic_left_buffer[samples]=0.0f;
				mic_right_buffer[samples]=0.0f;
			} 
			else 
			{
				mic_left_buffer[samples]=mic_sample_float;
				mic_right_buffer[samples]=0.0f;
			}

			// write to a file the I/Q float buffers 
			if (recFilePath)
			{
				// did we open a filep using a name? or did we use stdout
				record_buffer[rec_samples++]=left_sample_float;
				record_buffer[rec_samples++]=right_sample_float;
				if (rec_samples==(buffer_size*2))
				{
					//int num;
					if (recFp)
						//num=fwrite(record_buffer, sizeof(float), rec_samples, recFp);
						fwrite(record_buffer, sizeof(float), rec_samples, recFp);
					else
						//num=fwrite(record_buffer, sizeof(float), rec_samples, stdout);
						fwrite(record_buffer, sizeof(float), rec_samples, stdout);

					rec_samples=0;
				}
			}

			// if UDP server is running feed it
			if (udpFd)
			{				
				udp_buffer[udp_samples++]=left_sample_float;
				udp_buffer[udp_samples++]=right_sample_float;
				if (udp_samples==(buffer_size*2))
				{
					send_udp(udp_buffer, udp_samples * sizeof(float));
					udp_samples=0;
				}
			}

			samples++;

			key_thread_process(1.0, dash, dot, FALSE);
			//key_thread_process(1.0, dash, dot, TRUE);

			// when we have enough samples give them to DttSP and get the results
			if(samples==buffer_size) 
			{
				// process the input
				Audio_Callback (
						left_input_buffer, right_input_buffer,
						left_output_buffer, right_output_buffer, 
						buffer_size, 0);

				// transmit
				if(xmit) 
				{
					if(tuning) 
					{
						sineWave(mic_left_buffer,buffer_size,tuningPhase,(double)cwPitch);
						tuningPhase=sineWave(mic_right_buffer,buffer_size,tuningPhase,(double)cwPitch);
					} 
					else if(testing) 
					{
						// leave alone
					} 
					else if(mode==modeCWU || mode==modeCWL) 
					{
						CWtoneExchange(mic_left_buffer, mic_right_buffer, buffer_size);
					}
				}

				// process the output
				Audio_Callback (
						mic_left_buffer, mic_right_buffer,
						left_tx_buffer, right_tx_buffer, 
						buffer_size, 1);

				if(pennyLane) 
				{
					gain=1.0;
				} 
				else 
				{
					//gain=rfGain/255.0;
					gain=rfGain;
				}

				for(j=0;j<buffer_size;j+=output_sample_increment) 
				{
					if (!ob)
					{
						ob = buf_get_buffer(ozyTxBufs);
						if (!ob)
						{
							fprintf(stderr, "%s: No ozyTxBufs available\n", __func__);
							break;
						}
					}
					buf = ob->buf;

					left_rx_sample=(short)(left_output_buffer[j]*32767.0);
					right_rx_sample=(short)(right_output_buffer[j]*32767.0);

					if(xmit || mute) 
					{
						left_tx_sample=(short)(left_tx_buffer[j]*32767.0*gain);
						right_tx_sample=(short)(right_tx_buffer[j]*32767.0*gain);
					} 
					else 
					{
						left_tx_sample=0;
						right_tx_sample=0;
					}

					buf[output_buffer_index++]=left_rx_sample>>8;
					buf[output_buffer_index++]=left_rx_sample;
					buf[output_buffer_index++]=right_rx_sample>>8;
					buf[output_buffer_index++]=right_rx_sample;
					buf[output_buffer_index++]=left_tx_sample>>8;
					buf[output_buffer_index++]=left_tx_sample;
					buf[output_buffer_index++]=right_tx_sample>>8;
					buf[output_buffer_index++]=right_tx_sample;

					if(output_buffer_index>=OZY_BUFFER_SIZE) 
					{
						// send it!
						buf[0]=SYNC;
						buf[1]=SYNC;
						buf[2]=SYNC;

						// set mox
						control_out[0]=control_out[0]&0xFE;
						control_out[0]=control_out[0]|(xmit&0x01);
						if(control_out[1]!=debug_control1) 
						{
							fprintf(stderr,"control_out[1]=%02X\n",control_out[1]);
							debug_control1=control_out[1];
						}

						if(splitChanged) 
						{
							buf[3]=control_out[0];
							buf[4]=control_out[1];
							buf[5]=control_out[2];
							buf[6]=control_out[3];
							if(bSplit) 
							{
								buf[7]=control_out[4]|0x04;
							} 
							else 
							{
								buf[7]=control_out[4];
							}
							splitChanged=0;
						} 
						else if(frequencyAChanged) 
						{
							if(bSplit)
							{
								buf[3]=control_out[0]|0x04; // Mercury (1)
							} 
							else 
							{
								buf[3]=control_out[0]|0x02; // Mercury and Penelope
							}
							buf[4]=ddsAFrequency>>24;
							buf[5]=ddsAFrequency>>16;
							buf[6]=ddsAFrequency>>8;
							buf[7]=ddsAFrequency;
							frequencyAChanged=0;
						} 
						else if(frequencyBChanged) 
						{
							if(bSplit) 
							{
								buf[3]=control_out[0]|0x02; // Penelope
								buf[4]=ddsBFrequency>>24;
								buf[5]=ddsBFrequency>>16;
								buf[6]=ddsBFrequency>>8;
								buf[7]=ddsBFrequency;
							}
							frequencyBChanged=0;
						} 
						else if(driveLevelChanged && pennyLane) 
						{ 
							buf[3]=0x12|(xmit&0x01);
							buf[4]=driveLevel;
							buf[5]=0;
							buf[6]=0;
							buf[7]=0;
							driveLevelChanged=0;
						} 
						else 
						{
							buf[3]=control_out[0];
							buf[4]=control_out[1];
							buf[5]=control_out[2];
							buf[6]=control_out[3];
							if(bSplit) 
							{
								buf[7]=control_out[4]|0x04;
							} 
							else 
							{
								buf[7]=control_out[4];
							}
						}

						if(metis) 
							metis_write(0x02, ob->buf, OZY_BUFFER_SIZE);
						else
							libusb_xfer_buf(0x02, ob->buf, OZY_BUFFER_SIZE, ob);

						ob = NULL;
						// save space for sync up and control
						output_buffer_index=8;
					}
				}

				samples=0; // of samples to trigger next DttSp action
			}

			if(timing) 
			{
				sample_count++;
				if(!(sample_count%sampleRate)) 
				{
					unsigned long long diff;
					gettimeofday(&endtime, NULL);
					diff=timeval_diff(NULL, &endtime, &starttime);
					gettimeofday(&starttime, NULL);
					//fprintf(stderr,"samples %d delta_t %7ld us rxbufs free %d in flight %d\n", sample_count, diff, buf_count_buffers(ozyRxBufs), buf_count_inflight(ozyRxBufs));
					sample_count=0;
					if (diff > 1010000LL)
					{
						fprintf(stderr,"%s: %d samples took %7ld usec. too long. Skipping buffer to catch up (%d samples)\n",
								__func__, sampleRate, diff-1000000LL, samples);

						samples=start_samples; // we're throwing away an input buffer's worth of data, fix up number of samples processed to trigger next dsp action
						return;
					}
				}
			}
		}
	} 
	else 
	{
		time_t t;
		struct tm* gmt;
		time(&t);
		gmt=gmtime(&t);

		fprintf(stderr,"%s: process_ozy_input_buffer: did not find sync\n", asctime(gmt));
		//dump_ozy_buffer("input buffer",buffer);
		//exit(1);
	}

}

void ozy_prime() 
{
	int i;
	int bytes;

	if(control_out[1]!=debug_control1) 
	{
		fprintf(stderr,"%s: control_out[1]=%02X\n", __func__, control_out[1]);
		debug_control1=control_out[1];
	}

	// prime the ozy pump w/ 2 packets of zeroes
	/*
	for(i=0; i<2; i++) 
	{
		fprintf(stderr, "%s: Calling buf_get_buffer\n", __func__);
		usbBuf_t *ob = buf_get_buffer(ozyTxBufs);
		if (!ob)
		{
			fprintf(stderr, "%s: Can't get ozyTxBuf, quitting\n", __func__);
			raise(SIGQUIT);
		}

		char *buf = ob->buf;
		buf[0]=SYNC;
		buf[1]=SYNC;
		buf[2]=SYNC;
		buf[3]=control_out[0];
		buf[4]=control_out[1];
		buf[5]=control_out[2];
		buf[6]=control_out[3];
		buf[7]=control_out[4];

		for(i=8;i<OZY_BUFFER_SIZE;i++) 
		{
			buf[i]=0;
		}

		if(metis) 
		{
			metis_write(0x02,buf,OZY_BUFFER_SIZE);
		} 
		else 
		{
			libusb_xfer_buf(0x02,(void*)(buf),OZY_BUFFER_SIZE, ob);
		}
	}
	*/
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Thread for reading the ep6 and ep2 I2C IO 
* 
* @param arg
* 
* @return 
*/

static void ozy_ep6_ep2_io_thread_request_exit(int code)
{
	//fprintf(stderr,"%s:exit requested code %d\n", __func__, code);
	pthread_mutex_lock(&ep6_ep2_io_thread_exit_cond_lock);
	ep6_ep2_io_thread_exit_code = code;
	pthread_cond_signal(&ep6_ep2_io_thread_exit_cond);
	pthread_mutex_unlock(&ep6_ep2_io_thread_exit_cond_lock);
}


static void ozy_ep4_io_thread_request_exit(int code)
{
	//fprintf(stderr,"%s:exit requested code %d\n", __func__, code);
	pthread_mutex_lock(&ep4_io_thread_exit_cond_lock);
	ep4_io_thread_exit_code = code;
	pthread_cond_signal(&ep4_io_thread_exit_cond);
	pthread_mutex_unlock(&ep4_io_thread_exit_cond_lock);
}

void* ozy_ep6_ep2_io_thread(void* arg) 
{
	int rc=0;

	struct timeval tv = {0, LIBUSB_TIMEOUT_USECS};

	while(!ep6_ep2_io_thread_exit_code) 
	{
		usbBuf_t *ob = buf_get_buffer(ozyRxBufs);
		if(ob)
		{
			rc = libusb_xfer_buf(EP_RX_DATA, ob->buf, OZY_BUFFER_SIZE, ob);
			if (rc<0)
			{
				ozy_ep6_ep2_io_thread_request_exit(rc);
				break;
			}
		}
		else
		{
			// no buffer available, rest awhile
			while((rc=libusb_handle_events_timeout(usb_context, &tv))==LIBUSB_ERROR_INTERRUPTED);	
			if (rc<0)
			{
				fprintf(stderr, "%s: libusb_handle_events_timeout() failed (%s)\n", __func__, libusb_error_name(rc));
				//ozy_ep6_ep2_io_thread_request_exit(rc);
				//break;
			}
		}
	}
}

void* ozy_ep4_io_thread(void* arg) 
{
	int rc=0;

	while(!ep4_io_thread_exit_code) 
	{
		struct timeval tv = {0, LIBUSB_TIMEOUT_USECS};

		usbBuf_t *ob = buf_get_buffer(ozyBxBufs);
		if(ob)
		{
			rc = libusb_xfer_buf(EP_RX_BSDATA, ob->buf, SPECTRUM_BUFFER_SIZE, ob);
		}
		while((rc=libusb_handle_events_timeout(usb_context, &tv))==LIBUSB_ERROR_INTERRUPTED);	
		if (rc<0)
		{
			fprintf(stderr, "%s: libusb_handle_events_timeout() failed (%s)\n", __func__, libusb_error_name(rc));
			//ozy_ep4_io_thread_request_exit(rc);
			//kbreak;
		}
	}
}

/* --------------------------------------------------------------------------*/
/** 

* @brief Get the spectrum samples
* 
* @param samples
*/
void getSpectrumSamples(char *samples) {
    memcpy(samples,spectrum_samples,SPECTRUM_BUFFER_SIZE);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the MOX
* 
* @param state
*/
void setMOX(int state) {
    mox=state;
}

void ozy_cancel_all(usbBufPool_t *pool)
{
	int rc = 0;
	int total = 0;
	usbBuf_t *tmp;
	struct list_head *pos, *q;
	struct timeval tv = {0, LIBUSB_TIMEOUT_USECS};

	if (pool)
	{
		int cnt = buf_count_inflight(pool);
		total = 0;
		//fprintf(stderr, "%s: I  cnt %d total %d\n", __func__, cnt, total);
		// cancel the transactions on struct list_head inflight
		buf_cancel_inflight(pool);

		cnt = buf_count_inflight(pool);
		//fprintf(stderr, "%s: II  cnt %d total %d\n", __func__, cnt, total);
		while(cnt && total<100)
		{
			//fprintf(stderr, "%s: III cnt %d total %d\n", __func__, cnt, total);
			while((rc=libusb_handle_events_timeout(usb_context, &tv))==LIBUSB_ERROR_INTERRUPTED);	
			if (rc<0)
			{
				fprintf(stderr, "%s: libusb_handle_events_timeout() failed (%s)\n", __func__, libusb_error_name(rc));
			}
			cnt = buf_count_inflight(pool);
			total++;
		}
	}
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the speed
* 
* @param speed
*/
void setSpeed(int s) {
	float cal;
	int newSampleRate;
    speed=s;
    control_out[1]=control_out[1]&0xFC;
    control_out[1]=control_out[1]|s;


    if(s==SPEED_48KHZ) {
        output_sample_increment=1;
        newSampleRate=48000;
    } else if(s==SPEED_96KHZ) {
        output_sample_increment=2;
        newSampleRate=96000;
    } else if(s==SPEED_192KHZ) {
        output_sample_increment=4;
        newSampleRate=192000;
    }

	if (newSampleRate == sampleRate)
		return;

	mute = 1;
	ozy_cancel_all(ozyTxBufs);
	ozy_cancel_all(ozyRxBufs);

	sample_count=0;
	sampleRate = newSampleRate;
    SetSampleRate((double)sampleRate);
    SetKeyerSampleRate((float)sampleRate);

    SetRXOutputGain(0,0,volume/100.0);

	cal=(float)frequencyA*calibOffset;
	SetRXOsc(0,0,cal);
    SetTXOsc(1,0,cal);

    setFilter(filter);

    setMode(mode);
	mute = 0;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the 10 mHz source
* 
* @param source
*/
void set10MHzSource(int source) {
    clock10MHz=source;
    control_out[1]=control_out[1]&0xF3;
    control_out[1]=control_out[1]|(clock10MHz<<2);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the 122 mHz source
* 
* @param source
*/
void set122MHzSource(int source) {
    clock122_88MHz=source;
    control_out[1]=control_out[1]&0xEF;
    control_out[1]=control_out[1]|(source<<4);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the configuration
* 
* @param config
*/
void setConfig(int config) {
    control_out[1]=control_out[1]&0x9F;
    control_out[1]=control_out[1]|(config<<5);
}


/* --------------------------------------------------------------------------*/
/** 
* @brief Set the mic source
* 
* @param source
*/
void setMicSource(int source) {
    micSource=source;
    control_out[1]=control_out[1]&0x7F;
    control_out[1]=control_out[1]|(source<<7);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the class
* 
* @param class
*/
void setClass(int c) {
    class=c;
    control_out[2]=control_out[2]&0xFE;
    control_out[2]=control_out[2]|c;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the OC outputs
* 
* @param outputs
*/
void setOCOutputs(int outputs) {
    control_out[2]=control_out[2]&0x01;
    control_out[2]=control_out[2]|(outputs<<1);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the Alex attenuation
* 
* @param attenuation
*/
void setAlexAttenuation(int attenuation) {
    alexAttenuation=attenuation;
    control_out[3]=control_out[3]&0xFC;
    control_out[3]=control_out[3]|attenuation;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the preamplifer gain
* 
* @param gain
*/
void setPreamp(int p) {
    preamp=p;
    control_out[3]=control_out[3]&0xFB;
    control_out[3]=control_out[3]|(p<<2);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the LT2208 dither
* 
* @param dither
*/
void setLT2208Dither(int dither) {
    lt2208Dither=dither;
    control_out[3]=control_out[3]&0xF7;
    control_out[3]=control_out[3]|(dither<<3);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the LT2208 random
* 
* @param random
*/
void setLT2208Random(int random) {
    lt2208Random=random;
    control_out[3]=control_out[3]&0xEF;
    control_out[3]=control_out[3]|(random<<4);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief close down 
* 
* @param none
*/
void ozy_close(void)
{
	//pthread_cancel(ep6_ep2_io_thread_id);
	//pthread_join(ep6_ep2_io_thread_id, NULL);

	mute = 1;
	ozy_ep6_ep2_io_thread_request_exit(2);
	pthread_mutex_lock(&ep6_ep2_io_thread_exit_cond_lock);
	while(!ep6_ep2_io_thread_exit_code)
		pthread_cond_wait(&ep6_ep2_io_thread_exit_cond, &ep6_ep2_io_thread_exit_cond_lock);
	pthread_mutex_unlock(&ep6_ep2_io_thread_exit_cond_lock);
	pthread_join(ep6_ep2_io_thread_id, NULL);

	//pthread_cancel(ep4_io_thread_id);
	//pthread_join(ep4_io_thread_id, NULL);

	ozy_ep4_io_thread_request_exit(2);
	pthread_mutex_lock(&ep4_io_thread_exit_cond_lock);
	while(!ep4_io_thread_exit_code)
		pthread_cond_wait(&ep4_io_thread_exit_cond, &ep4_io_thread_exit_cond_lock);
	pthread_mutex_unlock(&ep4_io_thread_exit_cond_lock);
	pthread_join(ep4_io_thread_id, NULL);

	ozy_cancel_all(ozyRxBufs);
	ozy_cancel_all(ozyTxBufs);
	ozy_cancel_all(ozyBxBufs);

	buf_destroy_buffer_pool(ozyRxBufs);
	buf_destroy_buffer_pool(ozyTxBufs);
	buf_destroy_buffer_pool(ozyBxBufs);

	libusb_shutdown_ozy();
}

int ozy_create_buffer_pools(void)
{
	ozyRxBufs = buf_create_pool(OZY_NUM_BUFS, OZY_BUFFER_SIZE, "ozyRxBufs");
	if (!ozyRxBufs)
	{
		fprintf(stderr, "%s: buf_create_pool ozyRxBufs failed\n", __func__);
		// gotta have 'em
		return -1;
	}
	ozyTxBufs = buf_create_pool(OZY_NUM_BUFS, OZY_BUFFER_SIZE, "ozyTxBufs");
	if (!ozyTxBufs)
	{
		buf_destroy_buffer_pool(ozyRxBufs);
		fprintf(stderr, "%s: buf_create_pool ozyTxBufs failed\n", __func__);
		// gotta have 'em
		return -1;
	}
	ozyBxBufs = buf_create_pool(OZY_NUM_BUFS, SPECTRUM_BUFFER_SIZE, "ozyBxBufs");
	if (!ozyBxBufs)
	{
		buf_destroy_buffer_pool(ozyTxBufs);
		buf_destroy_buffer_pool(ozyRxBufs);
		fprintf(stderr, "%s: buf_create_pool ozyBxBufs failed\n", __func__);
		// gotta have 'em
		return -1;
	}


	return 0;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Initialize Ozy
* 
* @param sample_rate
* 
* @return 
*/
int ozy_init() {
    int rc;

	rc=ozy_create_buffer_pools();
	if (rc<0)
	{
		fprintf(stderr, "%s: ozy_create_buffer_pool failed, can't continue\n",__func__);
		return -1;
	}

    //
    setSpeed(speed);

    // setup defaults
    control_out[0] = MOX_DISABLED;
    control_out[1] = CONFIG_BOTH
            | MERCURY_122_88MHZ_SOURCE
            | MERCURY_10MHZ_SOURCE
            | speed
            | MIC_SOURCE_PENELOPE;
    control_out[2] = MODE_OTHERS;
    control_out[3] = ALEX_ATTENUATION_0DB
            | LT2208_GAIN_OFF
            | LT2208_DITHER_ON
            | LT2208_RANDOM_ON;
    control_out[4] = 0;

    ozyRestoreState();

    tuningPhase=0.0;

    if(metis) {
        int found;
        long i;
        metis_discover(interface);
        for(i=0;i<30000;i++) {
            if(metis_found()>0) {
				fprintf(stderr,"Metis discovered after %ld\n",i);
                break;
            }
        }
        if(metis_found()<=0) {
            return (-3);
        }
    } else {
        // open ozy
        rc = libusb_open_ozy();
        if (rc != 0) {
            fprintf(stderr,"Cannot locate Ozy\n");
            return (-1);
        }

        rc=libusb_get_ozy_firmware_string(ozy_firmware_version,8);

        if(rc!=0) {
            fprintf(stderr,"Failed to get Ozy Firmware Version - Have you run initozy yet?\n");
            libusb_shutdown_ozy();
            return (-2);
        }

        fprintf(stderr,"Ozy FX2 version: %s\n",ozy_firmware_version);
    }
    
    force_write=0;

    create_spectrum_buffers(8);
    
	while (!usb_initialized)
	{
		fprintf(stderr, "usb not init\n");
		usleep(1000);
	}


    if(metis) 
	{ 
        metis_start_receive_thread();
    } 
	else 
	{
		// prime ozy by sending 2 packets to start
		usbBuf_t *ob = buf_get_buffer(ozyTxBufs);
		char *buf = ob->buf;
		buf[0]=SYNC;
		buf[1]=SYNC;
		buf[2]=SYNC;
		buf[3]=control_out[0];
		buf[4]=control_out[1];
		buf[5]=control_out[2];
		buf[6]=control_out[3];
		buf[7]=control_out[4];
		usbBuf_t *ob2 = buf_get_buffer(ozyTxBufs);
		buf = ob2->buf;
		buf[0]=SYNC;
		buf[1]=SYNC;
		buf[2]=SYNC;
		buf[3]=control_out[0];
		buf[4]=control_out[1];
		buf[5]=control_out[2];
		buf[6]=control_out[3];
		buf[7]=control_out[4];

		libusb_xfer_buf(0x02, buf, OZY_BUFFER_SIZE, ob);
		libusb_xfer_buf(0x02, buf, OZY_BUFFER_SIZE, ob2);

		// create a thread to read/write to EP6/EP2
		rc=pthread_create(&ep6_ep2_io_thread_id,NULL,ozy_ep6_ep2_io_thread,NULL);
		if(rc != 0) {
			fprintf(stderr,"pthread_create failed on ozy_ep6_io_thread: rc=%d\n", rc);
		}

		// create a thread to read from EP4
		rc=pthread_create(&ep4_io_thread_id,NULL,ozy_ep4_io_thread,NULL);
		if(rc != 0) {
			fprintf(stderr,"pthread_create failed on ozy_ep4_io_thread: rc=%d\n", rc);
		}
	}

    return rc;
}


/* --------------------------------------------------------------------------*/
/** 
* @brief Get the ADC Overflow 
* 
* @return 
*/
int getADCOverflow() {
    int result=lt2208ADCOverflow;
    lt2208ADCOverflow=0;
    return result;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Get Ozy FX2 firmware version
* 
* @return 
*/
char* get_ozy_firmware_version() {
    return ozy_firmware_version;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Get Mercury software version
* 
* @return 
*/
int get_mercury_software_version() {
    return mercury_software_version;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Get Penelope software version
* 
* @return 
*/
int get_penelope_software_version() {
    return penelope_software_version;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Get Ozy software version
* 
* @return 
*/
int get_ozy_software_version() {
    return ozy_software_version;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief save Ozy state
* 
* @return 
*/
void ozySaveState() {
    char string[128];
    sprintf(string,"%d",clock10MHz);
    setProperty("10MHzClock",string);
    sprintf(string,"%d",clock122_88MHz);
    setProperty("122_88MHzClock",string);
    sprintf(string,"%d",micSource);
    setProperty("micSource",string);
    sprintf(string,"%d",class);
    setProperty("class",string);
    sprintf(string,"%d",lt2208Dither);
    setProperty("lt2208Dither",string);
    sprintf(string,"%d",lt2208Random);
    setProperty("lt2208Random",string);
    sprintf(string,"%d",alexAttenuation);
    setProperty("alexAttenuation",string);
    sprintf(string,"%d",preamp);
    setProperty("preamp",string);
    sprintf(string,"%d",speed);
    setProperty("speed",string);
    sprintf(string,"%d",sampleRate);
    setProperty("sampleRate",string);
    sprintf(string,"%d",pennyLane);
    setProperty("pennyLane",string);

}

/* --------------------------------------------------------------------------*/
/** 
* @brief resore Ozy state
* 
* @return 
*/
void ozyRestoreState() {
    char *value;

    value=getProperty("10MHzClock");
    if(value) {
        set10MHzSource(atoi(value));
    }
    value=getProperty("122_88MHzClock");
    if(value) {
        set122MHzSource(atoi(value));
    }
    value=getProperty("micSource");
    if(value) {
        setMicSource(atoi(value));
    }
    value=getProperty("class");
    if(value) {
        //setMode(atoi(value));
        setClass(atoi(value));
    }
    value=getProperty("lt2208Dither");
    if(value) {
        setLT2208Dither(atoi(value));
    }
    value=getProperty("lt2208Random");
    if(value) {
        setLT2208Random(atoi(value));
    }
    value=getProperty("alexAttenuation");
    if(value) {
        setAlexAttenuation(atoi(value));
    }
    value=getProperty("preamp");
    if(value) {
        setPreamp(atoi(value));
    }
    value=getProperty("speed");
    if(value) {
        setSpeed(atoi(value));
    }
    value=getProperty("sampleRate");
    if(value) {
        sampleRate=atoi(value);
    }
    value=getProperty("pennyLane");
    if(value) {
        pennyLane=atoi(value);
    }
}

void setAlexRxAntenna(int a) {
    alexRxAntenna=a;
    if(!xmit) {
        control_out[4]=control_out[4]&0xFC;
        control_out[4]=control_out[4]|a;
    }
}

void setAlexTxAntenna(int a) {
    alexTxAntenna=a;
    if(xmit) {
        control_out[4]=control_out[4]&0xFC;
        control_out[4]=control_out[4]|a;
    }
}

void setAlexRxOnlyAntenna(int a) {
    alexRxOnlyAntenna=a;
    if(!xmit) {
        control_out[3]=control_out[3]&0x9F;
        control_out[3]=control_out[3]|(a<<5);

        if(a!=0) {
            control_out[3]=control_out[3]|0x80;
        } else {
            control_out[3]=control_out[3]&0x7F;
        }
    }
}
