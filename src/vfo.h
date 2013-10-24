/** 
* @file vfo.h
* @brief Header files for the vfo functions
* @author John Melton, G0ORX/N6LYT, Doxygen Comments Dave Larsen, KV0S
* @version 0.1
* @date 2009-04-12
*/
// vfo.h

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
#ifndef _VFO_H
#define _VFO_H

extern long long frequencyA;
extern long long frequencyB;
extern int frequencyAChanged;
extern int frequencyBChanged;

extern long long frequencyALO;
extern long long frequencyBLO;
extern long long ddsAFrequency;
extern long long ddsBFrequency;

extern long frequencyIncrement;

int bSubRx;
extern int bSplit;
extern int splitChanged;

void vfoSaveState();
void vfoRestoreState();

GtkWidget* buildVfoUI();
void vfoIncrementFrequency(long increment);
void setAFrequency(long long frequency);
void setBFrequency(long long frequency);
void setLOFrequency(long long frequency);

int vfoTransmit(gpointer data);
int vfoStepFrequency(gpointer data);

void vfoRX2(int state);
void vfoSplit(int state);

extern float calibOffset;

#endif // _VFO_H
