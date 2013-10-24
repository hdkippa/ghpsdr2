/** 
* @file preamp.c
* @brief Preamp files for GHPSDR
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

#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>

#include "screensize.h"
#include "bandstack.h"
#include "command.h"
#include "preamp.h"
#include "main.h"
#include "property.h"
#include "ozy.h"
#include "vfo.h"

GtkWidget* preampFrame;
GtkWidget* preampTable;

GtkWidget* calibFrame;
GtkWidget* calibTable;

GtkWidget* buttonOn;
GtkWidget* calibButtonOn;

int calib=0;  // tune WWV, then turn on to clear, off to set to offset from nearest standard frequency
float preampOffset=0.0;
long long startOffset=0;

/* --------------------------------------------------------------------------*/
/** 
* @brief Preamp button Callback 
* 
* @param widget -- pointer to the parent widget, 
* @param data -- pointer to the data.
*/
void preampButtonCallback(GtkWidget* widget,gpointer data) {
    GtkWidget* label;
    char c[80];
    gboolean state;

    if(preamp) {
        state=0;
        preampOffset=0.0;
    } else {
        state=1;
        preampOffset=-20.0;
    }    
    setPreamp(state);


    label=gtk_bin_get_child((GtkBin*)widget);
    if(state) {
        //gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &buttonSelected);
        //gtk_widget_modify_fg(label, GTK_STATE_PRELIGHT, &buttonSelected);
    } else {
        //gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &white);
        //gtk_widget_modify_fg(label, GTK_STATE_PRELIGHT, &black);
    }
}

void setCalib(float cal) {
	calib=cal;
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Calib button Callback 
* 
* @param widget -- pointer to the parent widget, 
* @param data -- pointer to the data.
*/
void calibButtonCallback(GtkWidget* widget,gpointer data) {
	float offset;
    GtkWidget* label=gtk_bin_get_child((GtkBin*)widget);

    if(calib) {
        calib=0;
		offset=(float)frequencyA/(float)startOffset;
		calibOffset=offset-1.0f;
		if (frequencyA > startOffset)
			calibOffset=-calibOffset;
		printf("%s: calibOffset %8.8f\n", __func__, calibOffset);
		setAFrequency(startOffset);
        //gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &white);
        //gtk_widget_modify_fg(label, GTK_STATE_PRELIGHT, &black);
    } else {
        calib=1; // calibration active, set calibOffset to 0.0
		startOffset=frequencyA;
        calibOffset=0.0;
		setAFrequency(startOffset); // reset the dsp lib RXOsc to 0.0
        //gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &buttonSelected);
        //gtk_widget_modify_fg(label, GTK_STATE_PRELIGHT, &buttonSelected);
    }    
    setCalib(calib);
}

/* --------------------------------------------------------------------------*/
/** 
* @brief Build Preamp User Interface 
* 
* @return GtkWidget pointer 
*/
GtkWidget* buildPreampUI() {

    GtkWidget* label;


    preampFrame=gtk_frame_new("Preamp");
    ////gtk_widget_modify_bg(preampFrame,GTK_STATE_NORMAL,&background);
    ////gtk_widget_modify_fg(gtk_frame_get_label_widget(GTK_FRAME(preampFrame)),GTK_STATE_NORMAL,&white);

    preampTable=gtk_table_new(1,4,TRUE);

    // preamp settings
    buttonOn = gtk_button_new_with_label ("On");
    ////gtk_widget_modify_bg(buttonOn, GTK_STATE_NORMAL, &buttonBackground);
    //label=gtk_bin_get_child((GtkBin*)buttonOn);
    ////gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &white);
    gtk_widget_set_size_request(GTK_WIDGET(buttonOn),BUTTON_WIDTH,BUTTON_HEIGHT);
    g_signal_connect(G_OBJECT(buttonOn),"clicked",G_CALLBACK(preampButtonCallback),NULL);
    gtk_widget_show(buttonOn);
    gtk_table_attach_defaults(GTK_TABLE(preampTable),buttonOn,0,1,0,1);

	// calib settings
    calibButtonOn = gtk_button_new_with_label ("Cal");
    ////gtk_widget_modify_bg(calibButtonOn, GTK_STATE_NORMAL, &buttonBackground);
    //label=gtk_bin_get_child((GtkBin*)calibButtonOn);
    ////gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &white);
    gtk_widget_set_size_request(GTK_WIDGET(calibButtonOn),BUTTON_WIDTH,BUTTON_HEIGHT);
    g_signal_connect(G_OBJECT(calibButtonOn),"clicked",G_CALLBACK(calibButtonCallback),&calib);
    gtk_widget_show(calibButtonOn);
    gtk_table_attach_defaults(GTK_TABLE(preampTable),calibButtonOn,3,4,0,1);

    gtk_container_add(GTK_CONTAINER(preampFrame),preampTable);
    gtk_widget_show(preampTable);
    gtk_widget_show(preampFrame);

    return preampFrame;

}

void forcePreamp(int state) {
    GtkWidget* label;
    label=gtk_bin_get_child((GtkBin*)buttonOn);
    if(state) {
        //gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &buttonSelected);
        //gtk_widget_modify_fg(label, GTK_STATE_PRELIGHT, &buttonSelected);
        preampOffset=-20.0;
    } else {
        //gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &white);
        //gtk_widget_modify_fg(label, GTK_STATE_PRELIGHT, &black);
        preampOffset=0.0;
    }
}

void forceCalib(int state) {
    GtkWidget* label;
    label=gtk_bin_get_child((GtkBin*)buttonOn);
    if(state) {
        //gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &buttonSelected);
        //gtk_widget_modify_fg(label, GTK_STATE_PRELIGHT, &buttonSelected);
        calibOffset=0.0;
    } else {
        //gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &white);
        //gtk_widget_modify_fg(label, GTK_STATE_PRELIGHT, &black);
        calibOffset=0.0;
    }
}
