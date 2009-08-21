/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

  $Id$
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
#include "device.h"
#include "Logger.h"
#include "Parser.h"
#include "Dialogs.h"
#include "Utils.h"
#include "MapWindow.h"

bool InitLDRotary(ldrotary_s *buf) {
short i, bsize;
#ifdef DEBUG_ROTARY
char ventabuffer[200];
FILE *fp;
#endif

	switch (AverEffTime) {
		case ae15seconds:
			bsize=15;	// useless, LDinst already there
			break;
		case ae30seconds:
			bsize=30;	// limited useful
			break;
		case ae60seconds:
			bsize=60;	// starting to be valuable
			break;
		case ae90seconds:
			bsize=90;	// good interval
			break;
		case ae2minutes:
			bsize=120;	// other software's interval
			break;
		case ae3minutes:
			bsize=180;	// probably too long interval
			break;
		default:
			bsize=3; // make it evident
			break;
	}
	//if (bsize <3 || bsize>MAXLDROTARYSIZE) return false;
	for (i=0; i<MAXLDROTARYSIZE; i++) {
		buf->distance[i]=0;
		buf->altitude[i]=0;
	}
	buf->totaldistance=0;
	buf->start=-1;
	buf->size=bsize;
	buf->valid=false;
#ifdef DEBUG_ROTARY
	// John, DEBUGROTARY is used only on PC, no need to make it any better, it will be removed
	sprintf(ventabuffer,"InitLdRotary size=%d\r\n",buf->size);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
                    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
  return false; //RLD Paolo, I added this so it would compile correctly.  Not sure what the correct value is.
                //VNT Rob, it's an unused bool type so any value is ok, Ill make it void eventually
}

void InsertLDRotary(ldrotary_s *buf, int distance, int altitude) {
static short errs=0;
#ifdef DEBUG_ROTARY
char ventabuffer[200];
FILE *fp;
#endif

	if (CALCULATED_INFO.OnGround == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"OnGround, ignore LDrotary\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}
	if (CALCULATED_INFO.Circling == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Circling, ignore LDrotary\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}

	if (distance<3 || distance>150) { // just ignore, no need to reset rotary
		if (errs>2) {
#ifdef DEBUG_ROTARY
			sprintf(ventabuffer,"Rotary reset after exceeding errors\r\n");
			if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
				    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
			InitLDRotary(&rotaryLD);
			errs=0;
			return;

		}
		errs++;
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"(errs=%d) IGNORE INVALID distance=%d altitude=%d\r\n",errs,distance,altitude);
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}
	errs=0;
	if (++buf->start >=buf->size) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"*** rotary reset and VALID=TRUE ++bufstart=%d >=bufsize=%d\r\n",buf->start, buf->size);
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		buf->start=0;
		buf->valid=true; // flag for a full usable buffer
	}
	// need to fill up buffer before starting to empty it
	if ( buf->valid == true) buf->totaldistance-=buf->distance[buf->start];
	buf->totaldistance+=distance;
	buf->distance[buf->start]=distance;
	buf->altitude[buf->start]=altitude;
#ifdef DEBUG_ROTARY
	sprintf(ventabuffer,"insert buf[%d/%d], distance=%d totdist=%d\r\n",buf->start, buf->size-1, distance,buf->totaldistance);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
		    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
}

/*
 * returns 0 if invalid, 999 if too high

 */
int CalculateLDRotary(ldrotary_s *buf ) {

	int altdiff, eff;
	short bcold;
#ifdef DEBUG_ROTARY
	char ventabuffer[200];
	FILE *fp;
#endif

	if ( CALCULATED_INFO.Circling == TRUE || CALCULATED_INFO.OnGround == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Not Calculating, on ground or circling\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return(0);
	}

	if ( buf->start <0) {

#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Calculate: invalid buf start<0\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return(0);
	}

	ldrotary_s bc;
  	memcpy(&bc, buf, sizeof(ldrotary_s));

	if (bc.valid == false ) {
		if (bc.start==0) return(0); // unavailable
		bcold=0;
	} else {

		if (bc.start < (bc.size-1))
			bcold=bc.start+1;
		else
			bcold=0;
	}

	altdiff= bc.altitude[bcold] - bc.altitude[bc.start];
	if (altdiff == 0 ) return(INVALID_GR); // infinitum
	eff= bc.totaldistance / altdiff;

#ifdef DEBUG_ROTARY
	sprintf(ventabuffer,"bcstart=%d bcold=%d altnew=%d altold=%d altdiff=%d totaldistance=%d eff=%d\r\n",
		bc.start, bcold,
		bc.altitude[bc.start], bc.altitude[bcold], altdiff, bc.totaldistance, eff);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
                    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif

	if (eff>MAXEFFICIENCYSHOW) eff=INVALID_GR;

	return(eff);

}


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


