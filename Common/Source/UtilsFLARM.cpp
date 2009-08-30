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

#include "UtilsFLARM.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"


#ifdef NEWFLARMDB
#include "FlarmIdFile.h"
FlarmIdFile file;
#endif

int NumberOfFLARMNames = 0;

typedef struct {
  long ID;
  TCHAR Name[21];
} FLARM_Names_t;

#define MAXFLARMNAMES 200

FLARM_Names_t FLARM_Names[MAXFLARMNAMES];

void CloseFLARMDetails() {
  int i;
  for (i=0; i<NumberOfFLARMNames; i++) {
    //    free(FLARM_Names[i]);
  }
  NumberOfFLARMNames = 0;
}

void OpenFLARMDetails() {
  StartupStore(TEXT("OpenFLARMDetails\n"));

  if (NumberOfFLARMNames) {
    CloseFLARMDetails();
  }

  TCHAR filename[MAX_PATH];
  LocalPath(filename,TEXT("xcsoar-flarm.txt"));

  FILE *file = _tfopen(filename, TEXT("rt"));
  if (file == NULL)
    return;

  TCHAR line[READLINE_LENGTH];
  while (ReadStringX(file, READLINE_LENGTH, line)) {
    long id;
    TCHAR Name[MAX_PATH];

    if (_stscanf(line, TEXT("%lx=%s"), &id, Name) == 2) {
      if (AddFlarmLookupItem(id, Name, false) == false)
	{
	  break; // cant add anymore items !
	}
    }
  }

  fclose(file);
}


void SaveFLARMDetails(void)
{
  TCHAR filename[MAX_PATH];
  LocalPath(filename,TEXT("xcsoar-flarm.txt"));

  FILE *file = _tfopen(filename, TEXT("wt"));
  if (file == NULL)
    return;

  TCHAR wsline[READLINE_LENGTH];
  char cline[READLINE_LENGTH];

  for (int z = 0; z < NumberOfFLARMNames; z++)
    {
      wsprintf(wsline, TEXT("%lx=%s\r\n"), FLARM_Names[z].ID,FLARM_Names[z].Name);

#ifdef _UNICODE
      WideCharToMultiByte( CP_ACP, 0, wsline,
			   _tcslen(wsline)+1,
			   cline,
			   READLINE_LENGTH, NULL, NULL);
#else
      strcpy(cline, wsline);
#endif

      fputs(cline, file);
    }

  fclose(file);
}


int LookupSecondaryFLARMId(int id)
{
  for (int i=0; i<NumberOfFLARMNames; i++)
    {
      if (FLARM_Names[i].ID == id)
	{
	  return i;
	}
    }
  return -1;
}

int LookupSecondaryFLARMId(const TCHAR *cn)
{
  for (int i=0; i<NumberOfFLARMNames; i++)
    {
      if (_tcscmp(FLARM_Names[i].Name, cn) == 0)
	{
	  return i;
	}
    }
  return -1;
}


const TCHAR* LookupFLARMDetails(long id) {

  // try to find flarm from userFile
  int index = LookupSecondaryFLARMId(id);
  if (index != -1)
    {
      return FLARM_Names[index].Name;
    }

#ifdef NEWFLARMDB
  // try to find flarm from FLARMNet.org File
  FlarmId* flarmId = file.GetFlarmIdItem(id);
  if (flarmId != NULL)
    {
      return flarmId->cn;
    }
#endif
  return NULL;
}


int LookupFLARMDetails(const TCHAR *cn)
{
  // try to find flarm from userFile
  int index = LookupSecondaryFLARMId(cn);
  if (index != -1)
    {
      return FLARM_Names[index].ID;
    }

#ifdef NEWFLARMDB
  // try to find flarm from FLARMNet.org File
  FlarmId* flarmId = file.GetFlarmIdItem(cn);
  if (flarmId != NULL)
    {
      return flarmId->GetId();
    }
#endif
  return 0;
}


bool AddFlarmLookupItem(int id, const TCHAR *name, bool saveFile)
{
  int index = LookupSecondaryFLARMId(id);

  if (index == -1)
    {
      if (NumberOfFLARMNames < MAXFLARMNAMES - 1)
	{
	  // create new record
	  FLARM_Names[NumberOfFLARMNames].ID = id;
	  _tcsncpy(FLARM_Names[NumberOfFLARMNames].Name, name,20);
	  FLARM_Names[NumberOfFLARMNames].Name[20]=0;
	  NumberOfFLARMNames++;
	  SaveFLARMDetails();
	  return true;
	}
    }
  else
    {
      // modify existing record
      FLARM_Names[index].ID = id;
      _tcsncpy(FLARM_Names[index].Name, name,20);
      FLARM_Names[index].Name[20]=0;
      if (saveFile)
	{
	  SaveFLARMDetails();
	}
      return true;
    }
  return false;
}
