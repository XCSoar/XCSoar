/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "XCSoar.h"
#ifdef NEWFLARMDB
#include "FlarmIdFile.h"
#include "LogFile.hpp"
#include "LocalPath.hpp"

/**
 * Constructor of the FlarmIdFile class
 *
 * Reads the FLARMnet.org file and fills the flarmIds map
 */
FlarmIdFile::FlarmIdFile(void)
{
  //HANDLE hFile;
  TCHAR path[MAX_PATH];

  TCHAR flarmIdFileName[MAX_PATH] = TEXT("\0");

  LocalPath(path);

  wsprintf(flarmIdFileName,
	   TEXT("%s\\%s"),
	   path,
	   TEXT("data.fln"));

  //hFile = CreateFile(flarmIdFileName, GENERIC_READ,
  //	FILE_SHARE_READ, NULL, OPEN_EXISTING,
  //	     FILE_ATTRIBUTE_NORMAL, 0);
  FILE*	hFile = _wfopen(flarmIdFileName, TEXT("rt"));

  TCHAR res[100];
  TCHAR text[50];

  DWORD fileLength;

  //GetFileSize(hFile, &fileLength);
  //SetFilePointer(hFile, 7, NULL, FILE_BEGIN) ;
  fseek (hFile , 0 , SEEK_END);
  fileLength = ftell (hFile);
  fseek (hFile , 7 , SEEK_SET);

  int itemCount = 0;
  while(fileLength - ftell(hFile) > 87)
    {
      FlarmId *flarmId = new FlarmId;

      GetItem(hFile, flarmId);

      flarmIds[flarmId->GetId()] = flarmId;

      itemCount++;
    };

  wsprintf(text,TEXT("%d FlarmNet ids found\n"), itemCount);
  StartupStore(text);

  fclose(hFile);
}

/**
 * Destructor of the FlarmIdFile class
 */
FlarmIdFile::~FlarmIdFile(void)
{
}

/**
 * Reads next FLARMnet.org file entry and saves it
 * into the given flarmId
 * @param hFile File handle
 * @param flarmId Pointer to the FlarmId to be filled
 */
void FlarmIdFile::GetItem(HANDLE hFile, FlarmId *flarmId)
{
  GetAsString(hFile, 6, flarmId->id);
  GetAsString(hFile, 21, flarmId->name);
  GetAsString(hFile, 21, flarmId->airfield);
  GetAsString(hFile, 21, flarmId->type);
  GetAsString(hFile, 7, flarmId->reg);
  GetAsString(hFile, 3, flarmId->cn);
  GetAsString(hFile, 7, flarmId->freq);
  //SetFilePointer(hFile, 1, NULL, FILE_CURRENT) ;

  int i = 0;
  int maxSize = sizeof(flarmId->cn) / sizeof(TCHAR);
  while(flarmId->cn[i] != 0 && i < maxSize)
  {
    if (flarmId->cn[i] == 32)
    {
      flarmId->cn[i] = 0;
    }
    i++;
  }

  fseek((FILE*)hFile, 1, SEEK_CUR);
}

/**
 * Decodes the FLARMnet.org file and puts the wanted
 * characters into the res pointer
 * @param hFile File handle
 * @param charCount Number of character to decode
 * @param res Pointer to be written in
 */
void FlarmIdFile::GetAsString(HANDLE hFile, int charCount, TCHAR *res)
{
  int bytesToRead = charCount * 2;
  char bytes[100];
  //DWORD bytesRead;

  //ReadFile(hFile, bytes, bytesToRead, &bytesRead, NULL);
  fread(bytes, 1, bytesToRead, (FILE*)hFile);

  TCHAR *curChar = res;
  for (int z = 0; z < bytesToRead; z += 2)
    {
      char tmp[3];
      tmp[0] = bytes[z];
      tmp[1] = bytes[z+1];
      tmp[2] = 0;

      int i;
      sscanf(tmp, "%2x", &i);

      *curChar = (unsigned char)i;
      curChar ++;

    }
  *curChar = 0;

}

/**
 * Finds a FlarmId object based on the given FLARM id
 * @param id FLARM id
 * @return FlarmId object
 */
FlarmId* FlarmIdFile::GetFlarmIdItem(long id)
{
  FlarmIdMap::iterator iterFind = flarmIds.find(id);
  if( iterFind != flarmIds.end() )
    {
      return flarmIds[id];
    }

  return NULL;
}

/**
 * Finds a FlarmId object based on the given Callsign
 * @param cn Callsign
 * @return FlarmId object
 */
FlarmId* FlarmIdFile::GetFlarmIdItem(const TCHAR *cn)
{
  FlarmId *itemTemp = NULL;
  FlarmIdMap::iterator iterFind = flarmIds.begin();
  while( iterFind != flarmIds.end() )
    {
      itemTemp = (FlarmId*)(iterFind->second );
      if(wcscmp(itemTemp->cn, cn) == 0)
	{
	  return itemTemp;
	}
      iterFind++;
    }

  return NULL;
}

long FlarmId::GetId()
{
  long res;

  swscanf(id, TEXT("%6x"), &res);

  return res;
};


#endif
