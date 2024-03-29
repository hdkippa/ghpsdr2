/** 
* @file main.h
* @brief The main files headers files.
* @author John Melton, G0ORX/N6LYT, Doxygen Comments Dave Larsen, KV0S
* @version 0.1
* @date 2009-04-11
*/
// main.h
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
*/

#ifndef _MAIN_H_
#define _MAIN_H_

GdkColor background;
GdkColor buttonBackground;
GdkColor bandButtonBackground;
GdkColor buttonSelected;
GdkColor black;
GdkColor white;
GdkColor displayButton;
GdkColor green;
GdkColor red;
GdkColor meterYellow;
GdkColor grey;
GdkColor plotColor;
GdkColor filterColor;GtkWidget* mainFixed;

GtkWidget* buttonExit;
GtkWidget* buttonSetup;
GtkWidget* buttonTest;

GtkWidget* vfoWindow;
GtkWidget* bandWindow;
GtkWidget* modeWindow;
GtkWidget* displayWindow;
GtkWidget* filterWindow;
GtkWidget* audioWindow;
GtkWidget* meterWindow;
GtkWidget* bandscopeWindow;
GtkWidget* bandscope_controlWindow;
GtkWidget* agcWindow;
GtkWidget* preampWindow;
GtkWidget* receiverWindow;
GtkWidget* volumeWindow;
GtkWidget* transmitWindow;
GtkWidget* subrxWindow;

GdkColor subrxFilterColor;
GdkColor verticalColor;
GdkColor horizontalColor;
GdkColor spectrumTextColor;

GtkWidget* mainWindow;
GtkWidget* mainFixed;

GtkWidget* buttonExit;
GtkWidget* buttonSetup;
GtkWidget* buttonTest;

GtkWidget* vfoWindow;
GtkWidget* bandWindow;
GtkWidget* modeWindow;
GtkWidget* displayWindow;
GtkWidget* filterWindow;
GtkWidget* audioWindow;
GtkWidget* meterWindow;
GtkWidget* bandscopeWindow;
GtkWidget* bandscope_controlWindow;
GtkWidget* agcWindow;
GtkWidget* preampWindow;
GtkWidget* receiverWindow;
GtkWidget* volumeWindow;
GtkWidget* transmitWindow;
GtkWidget* subrxWindow;

#define SDR_MAXPATHLEN 1024

extern char propertiesPath[SDR_MAXPATHLEN]; // set to MAXPATHLEN

extern char *recFilePath;
extern FILE *recFp;

extern int cwPitch;

#endif // _MAIN_H_
