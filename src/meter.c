/** 
* @file meter.c
* @brief Meter functions 
* @author John Melton, G0ORX/N6LYT, Doxygen Comments Dave Larsen, KV0S
* @version 0.1
* @date 2009-04-11
*/

/* Copyright (C) 
* 2009 - John Melton, G0ORX/N6LYT, Doxygen Comments Dave Larsen, KV0S
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
* 
*/

// meter.c

#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "screensize.h"
#include "main.h"
#include "meter.h"
#include "meter_update.h"
#include "property.h"
#include "soundcard.h"
#include "ozy.h"
#include "preamp.h"
#include "filter.h"

GtkWidget* meterFixed;
GtkWidget* meter;
// from a QPixMap use:
// GdkPixmap* gdkPix = gdk_pixmap_foreign_new(pixmap.handle());
GdkPixmap* meterPixmap;

GtkWidget* dbm;
GdkPixmap* dbmPixmap;

GtkWidget* buttonSIGNAL;
GtkWidget* buttonSAV;

int meterDbm;

int meterMode=meterSIGNAL;

int meterFastPeak=0;
int meterSlowPeak=0;
int meterFastPeakCount=0;
int meterSlowPeakCount=0;
static const int meterFastPeakSamples=15;
static const int meterSlowPeakSamples=45;

int meterX=0;

void plotSignal(float* samples);
void drawSignal();
void updateOff();

// S-meter angles and consts
static const double vurange = (M_PI/180.0)*90.0;
static const double angle1 = 225.0 * (M_PI/180); // angles are in radians
static const double angle2 = 280.0 * (M_PI/180); // angles are in radians
static const double angle3 = 315.0 * (M_PI/180); // angles are in radians
static const int meter_width = 160;
static const int meter_height = 80;

static int nt_x = 0;
static int nt_y = 0;

static int pb1_x = 0;
static int pb1_y = 0;
static int pt1_x = 0;
static int pt1_y = 0;

static int pb2_x = 0;
static int pb2_y = 0;
static int pt2_x = 0;
static int pt2_y = 0;

// s-meter units are not linear so make a table to map dBm to S-units

/* --------------------------------------------------------------------------*/
/** 
* @brief Calculate filter calibration size offset 
* 
* @return 
*/
float getFilterSizeCalibrationOffset() {
    //int size=buffer_size;
    int size=filterHigh - filterLow;
    float i=log10((float)size);
    return 3.0f*(11.0f-i);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief  Callback when meter is created
* 
* @param widget
* @param event
* 
* @return 
*/
gboolean meter_configure_event(GtkWidget* widget,GdkEventConfigure* event) {
    GdkGC* gc;

    if(meterPixmap) g_object_unref(meterPixmap);

    meterPixmap=gdk_pixmap_new(widget->window,widget->allocation.width,widget->allocation.height,-1);

    gc=gdk_gc_new(widget->window);
    gdk_gc_set_rgb_fg_color(gc,&black);
    //gdk_gc_set_rgb_fg_color(gc,&meterYellow);
    gdk_draw_rectangle(meterPixmap,
                       gc,
                       TRUE,
                       0,0,
                       widget->allocation.width,
                       widget->allocation.height);

    g_object_unref(gc);

    return TRUE;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Callback when meter is exposed - paint it from the pixmap
* 
* @param widget
* @param event
* 
* @return 
*/

#define NUM_SuTICKS 9
#define NUM_DbTICKS 6

gboolean meter_expose_event(GtkWidget* widget,GdkEventExpose* event) 
{
	int tick_len;
	cairo_t *cr;
	cairo_pattern_t *p;
	double xb, yb, xt, yt;
	double angle, start_angle, tick_span;
	int w, h, i;
	char buf[32];

    gdk_draw_drawable(widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                    meterPixmap,
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);

	// draw the meter dial and pointer
	//cr = gdk_cairo_create(meterPixmap);
	cr = gdk_cairo_create(gtk_widget_get_window(meter));

	// meterYellow radial base to simulate lamp behind meter
	cairo_translate(cr, 0, 0);
	p = cairo_pattern_create_radial(meter_width/2+5, meter_height-5, 0, meter_width/2, meter_height, meter_height*2);
	cairo_pattern_add_color_stop_rgb(p, 0.3, 0.75, .70, .36);
	cairo_pattern_add_color_stop_rgb(p, 1, 0.3, 0.25, 0.15);
	cairo_set_source(cr, p);
	cairo_pattern_destroy(p);

	cairo_arc(cr, meter_width/2, meter_height/2, meter_height, 0, M_PI * 2);
	cairo_fill(cr);     
	cairo_stroke(cr);

	// black S1-S9
	cairo_new_sub_path(cr);
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, .0, .0, .0, 1.0);
	cairo_arc(cr, meter_width/2, meter_height, meter_height-15, angle1, angle2);  
	cairo_stroke(cr);

	// red +10db - +60db
	cairo_new_sub_path(cr);
	cairo_set_line_width(cr, 2.0);
	cairo_set_source_rgba(cr, .6, .3, .3, 1.0);
	cairo_arc(cr, meter_width/2, meter_height, meter_height-15, angle2, angle3);  
	cairo_stroke(cr);

	// draw the tick marks on the scale
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, 0, 0, 0, 1.0);

	w = event->area.width/2;
	h = meter_height;
	tick_len = meter_height-10;

	tick_span = (angle2-angle1)/(NUM_SuTICKS-1);
	start_angle = (M_PI/180)*45.0;

	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 10.0);

	// Ugh. handle drawing a meter face, sort of by hand here...
	cairo_new_sub_path(cr);
	for (i=0; i<NUM_SuTICKS; i++)
	{
		angle=start_angle+(tick_span*i);

		xt = w-cos(angle)*(tick_len-(i%2?2:0));
		yt = h-sin(angle)*(tick_len-(i%2?2:0));

		xb = w-cos(angle)*(tick_len-5);
		yb = h-sin(angle)*(tick_len-5);

		cairo_move_to(cr, xb, yb);
		cairo_line_to(cr, xt, yt);

		// construct S-meter labelling by hand. do this by struct?
		if (i == 0)
		{
			cairo_move_to(cr, xt-9, yt-2);
			sprintf(buf, "S1");
			cairo_show_text(cr, buf); 
		}
		else if (i == 2)		// top of tick
		{
			cairo_move_to(cr, xt-5, yt-2);
			sprintf(buf, "3");
			cairo_show_text(cr, buf); 
		}
		else if (i == 4)
		{
			cairo_move_to(cr, xt-3, yt-2);
			sprintf(buf, "5");
			cairo_show_text(cr, buf); 
		}
		else if (i == 6)
		{
			cairo_move_to(cr, xt-1, yt-2);
			sprintf(buf, "7");
			cairo_show_text(cr, buf); 
		}
		else if (i == 8)
		{
			cairo_move_to(cr, xt-0, yt-2);
			sprintf(buf, "9");
			cairo_show_text(cr, buf); 
		}

	}	
	
	start_angle=start_angle+(tick_span*i);
	tick_span = (angle3-angle2)/NUM_DbTICKS;

	//cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	//cairo_set_font_size(cr, 8.0);

	for (i=0; i<NUM_DbTICKS; i++)
	{
		angle=start_angle+(tick_span*i);

		// top of tick
		xt = w-cos(angle)*(tick_len-(i%2?0:2));
		yt = h-sin(angle)*(tick_len-(i%2?0:2));

		xb = w-cos(angle)*(tick_len-5);
		yb = h-sin(angle)*(tick_len-5);

		cairo_move_to(cr, xb, yb);
		cairo_line_to(cr, xt, yt);

		if (i == 01)
		{
			cairo_move_to(cr, xt-2, yt-2);
			sprintf(buf, "+20");
			cairo_show_text(cr, buf); 
		}
		else if (i == 03)
		{
			cairo_move_to(cr, xt-2, yt-2);
			sprintf(buf, "+40");
			cairo_show_text(cr, buf); 
		}		
		else if (i == 05)
		{
			cairo_move_to(cr, xt-1, yt-2);
			sprintf(buf, "+60");
			cairo_show_text(cr, buf); 
		}
	}

	cairo_move_to(cr, meter_width/2-22, meter_height-34);
	sprintf(buf, "%3.3d dBm", meterDbm);
	cairo_show_text(cr, buf); 
	cairo_stroke(cr);

	cairo_set_font_size(cr, 9.0);
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_BOLD);
	cairo_move_to(cr, meter_width/2-29, meter_height-46);
	sprintf(buf, "S-unit", meterDbm);
	cairo_show_text(cr, buf); 
	cairo_stroke(cr);

	cairo_set_source_rgba(cr, 0.6, 0.3, 0.3, 1.0);
	cairo_move_to(cr, meter_width/2+1, meter_height-46);
	sprintf(buf, "dB/S9", meterDbm);
	cairo_show_text(cr, buf); 
	cairo_stroke(cr);

	// draw the fast peak bar 
	cairo_set_line_width(cr, 3.0);
	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
	cairo_move_to(cr, pb1_x, pb1_y);
	cairo_line_to(cr, pt1_x, pt1_y);
	cairo_stroke(cr);

	// draw the slow peak bar 
	cairo_set_line_width(cr, 3.0);
	cairo_set_operator(cr, CAIRO_OPERATOR_XOR);
	//cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_move_to(cr, pb2_x, pb2_y);
	cairo_line_to(cr, pt2_x, pt2_y);
	cairo_stroke(cr);

	// draw the needle over the background
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
	cairo_move_to(cr, meter_width/2, meter_height);
	cairo_line_to(cr, nt_x, nt_y);
	cairo_stroke(cr);

	cairo_destroy(cr);

    return FALSE;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Draw the meter signal 
*/
const double full_scale=108.0; // S1 @ -121dbM, +60db @ -13 dbM, range in dBm is 108 
const double s_full_scale=48.0; // @ -73 dbm
const double db_full_scale=60.0;

void meterDrawSignal() 
{
	if(meter->window) 
	{
		int i;
		double angle;
		double needle_len;
		int x1, y1;
		double draw_level, tick_len;

		draw_level = meterX;


		if (draw_level<=s_full_scale)
		{
			angle=((angle2-angle1)/s_full_scale)*draw_level;
		}
		else 
		{
			angle=((angle3-angle2)/db_full_scale)*(draw_level-s_full_scale);
			angle+=angle2-angle1;
		}

		angle+=((M_PI/180.0)*45.0);

		needle_len = meter->allocation.height-10;

		// bottome center for start of needle
		x1 = meter->allocation.width/2;
		y1 = meter->allocation.height;

		// top of needle, polar to rectangular
		nt_x = x1-cos(angle)*needle_len;
		nt_y = y1-sin(angle)*needle_len;

		// calculate the fast peak meter position
		// update the fast peak
		if(meterX>meterFastPeak) {
			meterFastPeak=meterX;
			meterFastPeakCount=0;
		}
		if(meterFastPeakCount++ >= meterFastPeakSamples) {
			meterFastPeak=meterX;
			meterFastPeakCount=0;
		}

		draw_level = meterFastPeak;
		if (draw_level<=s_full_scale)
		{
			angle=((angle2-angle1)/s_full_scale)*draw_level;
		}
		else 
		{
			angle=((angle3-angle2)/db_full_scale)*(draw_level-s_full_scale);
			angle+=angle2-angle1;
		}

		angle+=((M_PI/180.0)*45.0);

		tick_len = meter->allocation.height-10;
		pb1_x = x1-cos(angle)*(tick_len-5);
		pb1_y = y1-sin(angle)*(tick_len-5);

		pt1_x = x1-cos(angle)*(tick_len);
		pt1_y = y1-sin(angle)*(tick_len);

		// calculate the slow peak meter position
		// update the fast peak
		if(meterX>meterSlowPeak) {
			meterSlowPeak=meterX;
			meterSlowPeakCount=0;
		}
		if(meterSlowPeakCount++ >= meterSlowPeakSamples) {
			meterSlowPeak=meterX;
			meterSlowPeakCount=0;
			meterFastPeak=meterX;
			meterFastPeakCount=0;
		}

		draw_level = meterSlowPeak;
		if (draw_level<=s_full_scale)
		{
			angle=((angle2-angle1)/s_full_scale)*draw_level;
		}
		else 
		{
			angle=((angle3-angle2)/db_full_scale)*(draw_level-s_full_scale);
			angle+=angle2-angle1;
		}

		angle+=((M_PI/180.0)*45.0);

		tick_len = meter->allocation.height-10;
		pb2_x = x1-cos(angle)*(tick_len-5);
		pb2_y = y1-sin(angle)*(tick_len-5);

		pt2_x = x1-cos(angle)*(tick_len);
		pt2_y = y1-sin(angle)*(tick_len);

		gtk_widget_queue_draw(meter);
	}
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Plot the meter signal 
* 
* @param sample
*/
void meterPlotSignal(float sample) {

    // plot the meter
    float val;//=sample-30; // convert to dBm from dB 1W
    //val+=multimeterCalibrationOffset + getFilterSizeCalibrationOffset()+preampOffset;
	//val+=preampOffset;
	// convert to dBm from dB 1V and offset for filter width
	val+=preampOffset + (sample-getFilterSizeCalibrationOffset());
	//val+=preampOffset + sample;

    meterDbm=(int)val;
    meterX=121+meterDbm;
    if(meterX<=0) meterX=1;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Turn off the meter update
*/
void meterUpdateOff() {

    // get the meter context - just copy the window GC and modify
    GdkGC* gc;
    if(meter->window) {
        gc=gdk_gc_new(meter->window);
        gdk_gc_copy(gc,meter->style->black_gc);
        gdk_draw_rectangle(meterPixmap,gc,TRUE,0,0,meter->allocation.width,meter->allocation.height);

        // update the meter

        gtk_widget_queue_draw_area(meter,0,0,meter->allocation.width,meter->allocation.height);
    }
}

void updateMeter(float sample) {
    switch(meterMode) {
        case meterSIGNAL:
            meterPlotSignal(sample);
            meterDrawSignal();
            break;
        case meterOFF:
            meterUpdateOff();
            break;
    }
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Set the meter mode
* 
* @param mode
*/
void setMeterMode(int mode) {
    char command[80];
    meterMode=mode;

    switch(meterMode) {
        case meterSIGNAL:
            //sprintf(command,"setMeterType 0");
            //writeCommand(command);
            break;
        case meterOFF:
            break;
    }
}


/* --------------------------------------------------------------------------*/
/** 
* @brief Build the meter user interface
* 
* @return 
*/

GtkWidget* buildMeterUI() {
	cairo_t *cr;

   meterFixed=gtk_fixed_new();
    //gtk_widget_modify_bg(meterFixed,GTK_STATE_NORMAL,&background);

    // meter
    meter=gtk_drawing_area_new();
    gtk_widget_set_size_request(GTK_WIDGET(meter), meter_width, meter_height);
    g_signal_connect(G_OBJECT (meter),"configure_event",G_CALLBACK(meter_configure_event),NULL);
    g_signal_connect(G_OBJECT (meter),"expose_event",G_CALLBACK(meter_expose_event),NULL);
    gtk_widget_show(meter);
    gtk_fixed_put((GtkFixed*)meterFixed,meter,0,0);

    return meterFixed;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Save the meter state
*/
void meterSaveState() {
    char string[128];
    sprintf(string,"%d",meterMode);
    setProperty("meter.mode",string);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Restore the meter state
*/
void meterRestoreState() {
    char* value;
    value=getProperty("meter.mode");
    if(value) meterMode=atoi(value);
}


