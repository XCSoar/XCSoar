/*
   $Id$


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


#include "StdAfx.h"
#include "Utils.h"
#include "Sizes.h"
#include "externs.h"
#include "Dialogs.h"
#include "resource.h"
#include "Utils.h"
#include "options.h"

#include <aygshell.h>

#include "AirfieldDetails.h"
#include <zzip/lib.h>
#include "wcecompat/ts_string.h"

ZZIP_FILE* zAirfieldDetails = NULL;

static TCHAR  szAirfieldDetailsFile[MAX_PATH] = TEXT("\0");

void OpenAirfieldDetails() {
  char zfilename[MAX_PATH] = "\0";

  zAirfieldDetails = NULL;

  GetRegistryString(szRegistryAirfieldFile, szAirfieldDetailsFile, MAX_PATH);

  if (_tcslen(szAirfieldDetailsFile)>0) {
    ExpandLocalPath(szAirfieldDetailsFile);
    unicode2ascii(szAirfieldDetailsFile, zfilename, MAX_PATH);
    SetRegistryString(szRegistryAirfieldFile, TEXT("\0"));
  } else {
    static TCHAR szMapFile[MAX_PATH] = TEXT("\0");
    static TCHAR szFile[MAX_PATH] = TEXT("\0");
    GetRegistryString(szRegistryMapFile, szMapFile, MAX_PATH);
    if (_tcslen(szMapFile)>0) {
      ExpandLocalPath(szMapFile);
      _tcscpy(szFile,szMapFile);
      wcscat(szFile,TEXT("/airfields.txt"));
      unicode2ascii(szFile, zfilename, MAX_PATH);
    } else {
      zfilename[0]= 0;
    }
  }
  if (strlen(zfilename)>0) {
    zAirfieldDetails = zzip_fopen(zfilename,"rb");
  }
};


void CloseAirfieldDetails() {
  if (zAirfieldDetails == NULL) {
    return;
  }
  // file was OK, so save the registry
  ContractLocalPath(szAirfieldDetailsFile);
  SetRegistryString(szRegistryAirfieldFile, szAirfieldDetailsFile);

  zzip_fclose(zAirfieldDetails);
  zAirfieldDetails = NULL;
};


void LookupAirfieldDetail(TCHAR *Name, TCHAR *Details) {
  int i;
  TCHAR UName[100];
  TCHAR NameA[100];
  TCHAR NameB[100];
  TCHAR NameC[100];
  TCHAR NameD[100];

  if (!WayPointList) return;

  for(i=0;i<(int)NumberOfWayPoints;i++)
    {
      if (((WayPointList[i].Flags & AIRPORT) == AIRPORT) ||
	  ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT)) {
	_tcscpy(UName, WayPointList[i].Name);
	CharUpper(UName);

	_stprintf(NameA,TEXT("%s A/F"),Name);
	_stprintf(NameB,TEXT("%s AF"),Name);
	_stprintf(NameC,TEXT("%s A/D"),Name);
	_stprintf(NameD,TEXT("%s AD"),Name);

	if ((_tcscmp(UName, Name)==0)
	  ||(_tcscmp(UName, NameA)==0)
	  ||(_tcscmp(UName, NameB)==0)
	  ||(_tcscmp(UName, NameC)==0)
	  ||(_tcscmp(UName, NameD)==0)
	    ){

	  if (WayPointList[i].Details) {
	    free(WayPointList[i].Details);
	  }
	  WayPointList[i].Details =
	    (TCHAR*)malloc((_tcslen(Details)+1)*sizeof(TCHAR));
	  _tcscpy(WayPointList[i].Details, Details);

	  return;

	}
      }
    }
}


#define DETAILS_LENGTH 5000

void ParseAirfieldDetails() {

  if(zAirfieldDetails == NULL)
    return;

  TCHAR TempString[READLINE_LENGTH+1];
  TCHAR Details[DETAILS_LENGTH+1];
  TCHAR Name[201];

  Details[0]= 0;
  Name[0]= 0;
  TempString[0]=0;

  BOOL inDetails = FALSE;
  int i;
  int k=0;

  while(ReadString(zAirfieldDetails,READLINE_LENGTH,TempString))
    {
      if(TempString[0]=='[') { // Look for start

	if (inDetails) {
	  LookupAirfieldDetail(Name, Details);
	  Details[0]= 0;
	  Name[0]= 0;
	}

	// extract name
	for (i=1; i<200; i++) {
	  if (TempString[i]==']') {
	    break;
	  }
	  Name[i-1]= TempString[i];
	}
	Name[i-1]= 0;

	inDetails = TRUE;

        if (k % 20 == 0) {
          StepProgressDialog();
        }
        k++;

      } else {
	// append text to details string
        if (_tcslen(Details)+_tcslen(TempString)+3<DETAILS_LENGTH) {
          wcscat(Details,TempString);
          wcscat(Details,TEXT("\r\n"));
        }
      }
    }

  if (inDetails) {
    LookupAirfieldDetail(Name, Details);
  }

}


void ReadAirfieldFile() {

  StartupStore(TEXT("ReadAirfieldFile\n"));

  CreateProgressDialog(gettext(TEXT("Loading Airfield Details File...")));

  {
    OpenAirfieldDetails();
    ParseAirfieldDetails();
    CloseAirfieldDetails();
  }

}

