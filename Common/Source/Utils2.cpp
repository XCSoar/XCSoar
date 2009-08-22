/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Utils2.h"
#include "StdAfx.h"
#include <stdio.h>
#ifndef __MINGW32__
#if defined(CECORE)
#include "winbase.h"
#endif
#if (WINDOWSPC<1)
#include "projects.h"
#endif
#endif
#include "options.h"
#include "externs.h"
#include "XCSoar.h"
#include "InfoBoxLayout.h"
#include "Device/device.h"
#include "Logger.h"
#include "Dialogs.h"
#include "Utils.h"
#include "MapWindow.h"

#ifdef ENABLE_UNUSED_CODE

typedef struct {
        int     array[RASIZE]; // rotary array with a predefined max capacity
        short   start;          // pointer to current first item in rotarybuf if used
        short   size;           // real size of rotary buffer (0-size)
} ifilter_s;

bool InitFilterBuffer(ifilter_s *buf, short bsize) {
short i;
	if (bsize <3 || bsize>RASIZE) return false;
	for (i=0; i<RASIZE; i++) buf->array[i]=0;
	buf->start=-1;
	buf->size=bsize;
}

void InsertRotaryBuffer(ifilter_s *buf, int value) {
	if (++buf->start >=buf->size) {
		buf->start=0;
	}
	buf->array[buf->start]=value;
}

int FilterFast(ifilter_s *buf, int minvalue, int maxvalue) {

  ifilter_s bc;
  memcpy(&bc, buf, sizeof(ifilter_s));

  short i,curs,nc,iter;
  int s, *val;
  float aver=0.0, oldaver, low=minvalue, high=maxvalue, cutoff;

  for (iter=0; iter<MAXITERFILTER; iter++) {
 	 for (i=0, nc=0, s=0; i<bc.size; i++) {
		val=&bc.array[i];
		if (*val >=low && *val <=high) { s+=*val; nc++; }
	  }
	  if (nc==0) { aver=0.0; break; }
	  oldaver=aver; aver=((float)s/nc);
 	  //printf("Sum=%d count=%d Aver=%0.3f (old=%0.3f)\n",s,nc,aver,oldaver);
	  if (oldaver==aver) break;
	  cutoff=aver/50; // 2%
 	  low=aver-cutoff;
 	  high=aver+cutoff;
  }
  //printf("Found: aver=%d (%0.3f) after %d iterations\n",(int)aver, aver, iter);
  return ((int)aver);

}

int FilterRotary(ifilter_s *buf, int minvalue, int maxvalue) {

  ifilter_s bc;
  memcpy(&bc, buf, sizeof(ifilter_s));

  short i,curs,nc,iter;
  int s, val;
  float aver, low, high, cutoff;

  low  = minvalue;
  high = maxvalue;

  for (iter=0; iter<MAXITERFILTER; iter++) {
 	 for (i=0, nc=0, s=0,curs=bc.start; i<bc.size; i++) {

		val=bc.array[curs];
		if (val >=low && val <=high) {
			s+=val;
			nc++;
		}
		if (++curs >= bc.size ) curs=0;
	  }
	  if (nc==0) {
		aver=0.0;
		break;
	  }
	  aver=((float)s/nc);
 	  //printf("Sum=%d count=%d Aver=%0.3f\n",s,nc,aver);

	  cutoff=aver/50; // 2%
 	  low=aver-cutoff;
 	  high=aver+cutoff;
  }

  //printf("final: aver=%d\n",(int)aver);
  return ((int)aver);

}
/*
main(int argc, char *argv[])
{
  short i;
  ifilter_s buf;
  InitFilterBuffer(&buf,20);
  int values[20] = { 140,121,134,119,116,118,121,122,120,124,119,117,116,130,122,119,110,118,120,121 };
  for (i=0; i<20; i++) InsertRotaryBuffer(&buf, values[i]);
  FilterFast(&buf, 70,200 );
  buf.start=10;
  for (i=0; i<20; i++) InsertRotaryBuffer(&buf, values[i]);
  FilterFast(&buf, 70,200 );
}
*/

#endif /* ENABLE_UNUSED_CODE */

/*
	Virtual Key Manager by Paolo Ventafridda

	Returns 0 if invalid virtual scan code, otherwise a valid transcoded keycode.

 */
int ProcessVirtualKey(int X, int Y, long keytime, short vkmode) {

// 0 is always thermal mode, and does not account
#define MAXBOTTOMMODES 5
#define VKTIMELONG 1500

	#ifdef DEBUG_PROCVK
	TCHAR buf[100];
	wsprintf(buf,_T("R=%d,%d,%d,%d, X=%d Y=%d kt=%ld"),MapWindow::MapRect.left, MapWindow::MapRect.top,
	MapWindow::MapRect.right, MapWindow::MapRect.bottom,X,Y,keytime);
	DoStatusMessage(buf);
	#endif

	short sizeup=MapWindow::MapRect.bottom-MapWindow::MapRect.top;
	short sizeright=MapWindow::MapRect.right-MapWindow::MapRect.left;
	short yup=(sizeup/3)+MapWindow::MapRect.top;
	short ydown=MapWindow::MapRect.bottom-(sizeup/3);
	short xleft=sizeright/3; // TODO FIX
	short xright=sizeright-xleft;

	if (Y<yup) {
		#ifndef DISABLEAUDIO
	        if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
		#endif
		if (keytime>=VKTIMELONG)
			return 0xc1;
		else
			return 38;
	}
	if (Y>ydown) {
		#ifndef DISABLEAUDIO
	        if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
		#endif
		if (keytime>=VKTIMELONG)
			return 0xc2;
		else

			return 40;
	}

		/*
		 * FIX ready: do not pass virtual ENTER while in Panmode.
		 * Currently it is allowed, should be better tested.  VNT 090702
		if ( !MapWindow::EnablePan ) {
	             DoStatusMessage(_T("Virtual ENTER"));
		     return 13;
		}
		return 0; // ignore it
		*/
	        return 13;
//	}
	DoStatusMessage(_T("VirtualKey Error"));
	return 0;
}


