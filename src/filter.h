/** 
* @file filter.h
* @brief Header files to define the filters.
* @author John Melton, G0ORX/N6LYT, Doxygen Comments Dave Larsen, KV0S
* @version 0.1
* @date 2009-04-11
*/
// filter.h
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

#define FILTERS 12

#define CW_PITCH 600

struct _FILTER {
    int low;
    int high;
    char* title;
};

typedef struct _FILTER FILTER;

#define filterF0 0
#define filterF1 1
#define filterF2 2
#define filterF3 3
#define filterF4 4
#define filterF5 5
#define filterF6 6
#define filterF7 7
#define filterF8 8
#define filterF9 9
#define filterVar1 10
#define filterVar2 11

extern int filter;

extern int filterLow;
extern int filterHigh;

extern int txFilterLowCut;
extern int txFilterHighCut;

extern int filterVar1Low;
extern int filterVar1High;
extern int filterVar2Low;
extern int filterVar2High;


GtkWidget* buildFilterUI();
void filterSaveState();
void filterRestoreState();
void setFilterValues(int mode);
void setFilter(int filter);
void setTxFilters();
