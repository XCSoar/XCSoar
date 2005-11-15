/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"

#include "RasterTerrain.h"
#include "XCSoar.h"

// JMW added cacheing of results of terrain lookup to reduce file IO

extern TCHAR szRegistryTerrainFile[];

// static variables shared between rasterterrains because can only
// have file opened by one reader

TERRAIN_INFO RasterTerrain::TerrainInfo;
FILE *RasterTerrain::fpTerrain;

CRITICAL_SECTION  CritSec_TerrainFile;


void RasterTerrain::SetCacheTime() {
  terraincachehits = 1;
  terraincachemisses = 0;
  cachetime++;
}


void RasterTerrain::ClearTerrainCache() {
  int i;
  for (i=0; i<MAXTERRAINCACHE; i++) {
    TerrainCache[i].index= -1;
    TerrainCache[i].recency= 0;
    TerrainCache[i].h= 0;
  }
  SortThresold = MAXTERRAINCACHE-1;
}

static int _cdecl TerrainCacheCompare(const void *elem1, const void *elem2 ){
#ifdef PARANOID
  if (!elem1 && !elem2) {
    return(0);
  }
  if (elem1 && !elem2) {
    return(-1);
  }
  if (!elem1 && elem2) {
    return(1);
  }
#endif
  if (((TERRAIN_CACHE *)elem1)->recency > ((TERRAIN_CACHE *)elem2)->recency)
    return (-1);
  if (((TERRAIN_CACHE *)elem1)->recency < ((TERRAIN_CACHE *)elem2)->recency)
    return (+1);
  if (((TERRAIN_CACHE *)elem1)->index > ((TERRAIN_CACHE *)elem2)->index)
    return (-1);
  if (((TERRAIN_CACHE *)elem1)->index < ((TERRAIN_CACHE *)elem2)->index)
    return (+1);
  return (0);
}

void RasterTerrain::OptimizeCash(void){

  qsort(&TerrainCache, MAXTERRAINCACHE,
        sizeof(_TERRAIN_CACHE), TerrainCacheCompare);
  SortThresold = MAXTERRAINCACHE-1;

}

short RasterTerrain::LookupTerrainCacheFile(long SeekPos) {
  // put new value in slot tcpmin

  __int16 NewAlt = 0;
  DWORD SeekRes;
  short Alt;

  if(!isTerrainLoaded())
    return -1;

  EnterCriticalSection(&CritSec_TerrainFile);

  //SeekRes = SetFilePointer(hTerrain,SeekPos,NULL,FILE_BEGIN);
  SeekRes = fseek(fpTerrain, SeekPos, SEEK_SET);
  //if(SeekRes == 0xFFFFFFFF && (dwError = GetLastError()) != NO_ERROR ) {
  if(SeekRes != 0) {
    // error, not found!
    Alt = -1;
  } else {
    if (fread(&NewAlt, 1, sizeof(__int16), fpTerrain) != sizeof(__int16))
      Alt = -1;
    else {
      // ReadFile(hTerrain,&NewAlt,sizeof(__int16),&dwBytesRead,NULL);
      Alt = NewAlt;
      if(Alt<0) Alt = -1;
    }
  }
  LeaveCriticalSection(&CritSec_TerrainFile);

  return Alt;

}

int TerrainCacheSearch(const void *key, const void *elem2 ){
#ifdef PARANOID
  if (!elem2) return (0);
#endif
  if ((long)key > ((TERRAIN_CACHE *)elem2)->index)
    return (-1);
  if ((long)key < ((TERRAIN_CACHE *)elem2)->index)
    return (+1);
  return (0);
}

short RasterTerrain::LookupTerrainCache(long SeekPos) {
  int ifound= -1;
  unsigned int recencymin = 0;
  int i;
  _TERRAIN_CACHE* tcp, *tcpmin;

  if(fpTerrain == NULL || TerrainInfo.StepSize == 0)
    return -1;

  terraincacheefficiency = (100*terraincachehits)/(terraincachehits+terraincachemisses);

  // search to see if it is found in the cache
  tcp = (_TERRAIN_CACHE *)bsearch((void *)SeekPos, &TerrainCache,
                                  SortThresold, sizeof(_TERRAIN_CACHE),
                                  TerrainCacheSearch);

  if (tcp != NULL){
    tcp->recency = cachetime;
    terraincachehits++;
    return(tcp->h);
  }

  // bsearch failed, so try exhaustive search

  tcp = &TerrainCache[SortThresold];
  for (i=SortThresold; i<MAXTERRAINCACHE; i++) {
    if (tcp->index == SeekPos) {
      tcp->recency = cachetime;
      terraincachehits++;
      return (tcp->h);
    }
    tcp++;
  }

  // if not found..
  terraincachemisses++;

  if (SortThresold>= MAXTERRAINCACHE) {
    SortThresold= MAXTERRAINCACHE-1;
  }
  if (SortThresold<0) {
    SortThresold = 0;
  }

  tcpmin = &TerrainCache[SortThresold];

  short Alt = LookupTerrainCacheFile(SeekPos);

  tcpmin->recency = cachetime;
  tcpmin->h = Alt;
  tcpmin->index = SeekPos;

  SortThresold--;
  if (SortThresold<0) {
    SortThresold = 0;
  }

  return (Alt);
}


float RasterTerrain::GetTerrainSlopeStep() {
  if(fpTerrain == NULL || TerrainInfo.StepSize == 0)
    return 0;
  float kpixel = (float)256.0/(
			  GetTerrainStepSize()
                          * (float)rounding
                          );
    return kpixel;
}


float RasterTerrain::GetTerrainStepSize() {
  if(fpTerrain == NULL || TerrainInfo.StepSize == 0)
    return 0;
  // this is approximate of course..
  float fstepsize = (float)(250.0/0.0025*TerrainInfo.StepSize);
  return fstepsize;
}


void RasterTerrain::SetTerrainRounding(double dist) {
  if(fpTerrain == NULL || TerrainInfo.StepSize == 0)
    return;
  rounding = iround(dist/(GetTerrainStepSize()/1000.0));
  if (rounding<1) {
    rounding = 1;
  }
}

int RasterTerrain::GetEffectivePixelSize(double dist) {
  int grounding;
  grounding = iround(2.0*(GetTerrainStepSize()/1000.0)/dist);
  if (grounding<1) {
    grounding = 1;
  }
  return grounding;
}

// JMW rounding further reduces data as required to speed up terrain
// display on low zoom levels


short RasterTerrain::GetTerrainHeight(double Lattitude,
				      double Longditude)
{
  long SeekPos;
  double X,Y;
  long lx, ly;

  //if(hTerrain == NULL)
  if(fpTerrain == NULL || TerrainInfo.StepSize == 0)
    return -1;

  if ((Lattitude > TerrainInfo.Top )||
      (Lattitude < TerrainInfo.Bottom )||
      (Longditude < TerrainInfo.Left )||
      (Longditude > TerrainInfo.Right )) {
    return -1;
  }

  X =  Longditude -TerrainInfo.Left;
  X = X / TerrainInfo.StepSize ;

  lx = lround(X/rounding)*rounding;

  Y = TerrainInfo.Top  - Lattitude ;
  Y = Y / TerrainInfo.StepSize ;

  if ((Y<0)||(X<0)) {
    return 0;
  }

  ly = lround(Y/rounding)*rounding;

  ly *= TerrainInfo.Columns;
  ly +=  lx;

  SeekPos = ly;
  SeekPos *= 2;
  SeekPos += sizeof(TERRAIN_INFO);

  ////// JMW added terrain cache lookup
  short h = LookupTerrainCache(SeekPos);
  return h;
}





void RasterTerrain::OpenTerrain(void)
{
  DWORD dwBytesRead;
  static TCHAR  szFile[MAX_PATH] = TEXT("\0");

  GetRegistryString(szRegistryTerrainFile, szFile, MAX_PATH);
  SetRegistryString(szRegistryTerrainFile, TEXT("\0"));

  fpTerrain = _tfopen(szFile, TEXT("rb"));
  //if( hTerrain == NULL)
  if( fpTerrain == NULL)
    {
      return;
    }
  //ReadFile(hTerrain,&TerrainInfo,sizeof(TERRAIN_INFO),&dwBytesRead,NULL);
//  setvbuf(fpTerrain, NULL, 0x00 /*_IOFBF*/, 4096*8);
  dwBytesRead = fread(&TerrainInfo, 1, sizeof(TERRAIN_INFO), fpTerrain);

  // TODO: sanity check of terrain info, to check validity of file
  // this can be done by checking the bounds compute correctly and
  // that we can seek to the end of the file
  SetRegistryString(szRegistryTerrainFile, szFile);

  InitializeCriticalSection(&CritSec_TerrainFile);

}



void RasterTerrain::CloseTerrain(void)
{
  //if( hTerrain == NULL)
  if( fpTerrain == NULL)
    {
      return;
    }
  else
    {
      //CloseHandle(hTerrain);
      fclose(fpTerrain);
      DeleteCriticalSection(&CritSec_TerrainFile);
      //hTerrain = NULL;
      fpTerrain = NULL;
    }
}
