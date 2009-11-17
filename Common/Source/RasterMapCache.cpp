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

#include "RasterMapCache.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"

// static variables shared between rasterterrains because can only
// have file opened by one reader

#ifdef __MINGW32__
#define int_fast8_t jas_int_fast8_t
#endif

// JMW experimental
#include "jasper/jasper.h"
#include "jasper/jpc_rtc.h"
#include "wcecompat/ts_string.h"


short RasterMapCache::_GetFieldAtXY(unsigned int lx,
                                    unsigned int ly) {

  //  unsigned int ix = CombinedDivAndMod(lx);
  //  unsigned int iy = CombinedDivAndMod(ly);

  if ((ly>=(unsigned int)TerrainInfo.Rows)
      ||(lx>=(unsigned int)TerrainInfo.Columns)) {
    return TERRAIN_INVALID;
  }
  return LookupTerrainCache((ly*TerrainInfo.Columns+lx)*2
                            +sizeof(TERRAIN_INFO));
}

void RasterMapCache::LockRead()
{
  // all lookups for this class potentially change the
  // object so must have write (exclusive) lock even
  // when doing terrain queries.
  lock.writeLock();
};


// Cached load on demand
// Cache map
int RasterMapCache::ref_count = 0;

ZZIP_FILE *RasterMapCache::fpTerrain;

void RasterMapCache::ServiceCache(void) {
  Poco::ScopedRWLock protect(lock, true);

  if (terraincachemisses > 0){
    OptimiseCache();
  }
  SetCacheTime();
}


void RasterMapCache::SetCacheTime() {
  terraincachehits = 1;
  terraincachemisses = 0;
  cachetime++;
}


void RasterMapCache::ClearTerrainCache() {
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

void RasterMapCache::OptimiseCache(void){
  qsort(&TerrainCache, MAXTERRAINCACHE,
        sizeof(_TERRAIN_CACHE), TerrainCacheCompare);
  SortThresold = MAXTERRAINCACHE-1;
}


short RasterMapCache::LookupTerrainCacheFile(const long &SeekPos) {
  // put new value in slot tcpmin

  __int16 NewAlt = 0;
  long SeekRes;
  short Alt;

  if(!isMapLoaded())
    return TERRAIN_INVALID;

  SeekRes = zzip_seek(fpTerrain, SeekPos, SEEK_SET);
  if(SeekRes != SeekPos) {
    // error, not found!
    Alt = TERRAIN_INVALID;
  } else {
    if (zzip_fread(&NewAlt, 1, sizeof(__int16), fpTerrain) != sizeof(__int16))
      Alt = TERRAIN_INVALID;
    else {
      Alt = max(0,NewAlt);
    }
  }

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

short RasterMapCache::LookupTerrainCache(const long &SeekPos) {
  _TERRAIN_CACHE* tcp, *tcpmin, *tcplim;

  if(fpTerrain == NULL || TerrainInfo.StepSize == 0)
    return TERRAIN_INVALID;

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
  tcplim = tcp+MAXTERRAINCACHE-SortThresold;
  while (tcp< tcplim) {
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
  } else if (SortThresold<0) {
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

// Specialised open/close routines
bool RasterMapCache::Open(char* zfilename) {
  Poco::ScopedRWLock protect(lock, true);
  terrain_valid = false;
  if (strlen(zfilename)<=0) {
    return false;
  }
  if (!fpTerrain) {
    fpTerrain = zzip_fopen(zfilename, "rb");
    if (!fpTerrain) {
      return false;
    }
    if (!zzip_file_real(fpTerrain)) {
      // don't allow cache mode on files in zip, because way too slow
      zzip_fclose(fpTerrain);
      fpTerrain = NULL;   // was false
      return false;
    };
  }

  DWORD dwBytesRead;
  dwBytesRead = zzip_fread(&TerrainInfo, 1, sizeof(TERRAIN_INFO),
                           fpTerrain);

  if (dwBytesRead != sizeof(TERRAIN_INFO)) {
    _Close();
    return false;
  }

  if (!TerrainInfo.StepSize) {
    _Close();
    return false;
  }
  terrain_valid = true;
  ClearTerrainCache();
  return terrain_valid;
}

// Close routines
void RasterMapCache::Close(void) {
  Poco::ScopedRWLock protect(lock, true);
  _Close();
}

void RasterMapCache::_Close(void) {
  terrain_valid = false;
  if(fpTerrain) {
    if (ref_count==1) {
      zzip_fclose(fpTerrain);
      fpTerrain = NULL;
    }
  }
}


