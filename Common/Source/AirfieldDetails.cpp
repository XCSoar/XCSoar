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


#include "stdafx.h"
#include "Utils.h"
#include "Sizes.h"
#include "externs.h"
#include "Dialogs.h"
#include "resource.h"
#include "Utils.h"
#include "options.h"

#include <aygshell.h>

#include "AirfieldDetails.h"


HANDLE hAirfieldDetails;

extern TCHAR szRegistryAirfieldFile[];

static TCHAR  szAirfieldDetailsFile[MAX_PATH] = TEXT("\0");

void OpenAirfieldDetails() {

  GetRegistryString(szRegistryAirfieldFile, szAirfieldDetailsFile, MAX_PATH);
  SetRegistryString(szRegistryAirfieldFile, TEXT("\0"));

  hAirfieldDetails = INVALID_HANDLE_VALUE;
  hAirfieldDetails = CreateFile(szAirfieldDetailsFile,GENERIC_READ,0,NULL,
				OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if( hAirfieldDetails == INVALID_HANDLE_VALUE)
    {
      return;
    }

};


void CloseAirfieldDetails() {
  if (hAirfieldDetails == NULL) {
    return;
  }
  if (hAirfieldDetails != INVALID_HANDLE_VALUE) {
    // file was OK, so save the registry
    SetRegistryString(szRegistryAirfieldFile, szAirfieldDetailsFile);

    CloseHandle(hAirfieldDetails);
    hAirfieldDetails = NULL;
  }
};


void LookupAirfieldDetail(TCHAR *Name, TCHAR *Details) {
  int i;
  TCHAR UName[100];
  TCHAR NameA[100];
  TCHAR NameB[100];
  TCHAR NameC[100];
  TCHAR NameD[100];

  for(i=0;i<(int)NumberOfWayPoints;i++)
    {
      if ((WayPointList[i].Flags & AIRPORT) == AIRPORT) {
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


void ParseAirfieldDetails() {
  if(hAirfieldDetails == NULL)
    return;

  TCHAR TempString[200];
  TCHAR Details[5000];
  TCHAR Name[100];

  Details[0]= 0;
  Name[0]= 0;
  TempString[0]=0;

  BOOL inDetails = FALSE;
  int i;
  int k=0;

  if (hAirfieldDetails == INVALID_HANDLE_VALUE)
    return;

  while(ReadString(hAirfieldDetails,200,TempString))
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
	wcscat(Details,TempString);
	wcscat(Details,TEXT("\r\n"));

	// TODO: check the string is not too long
      }
    }

  if (inDetails) {
    LookupAirfieldDetail(Name, Details);
  }

}


void ReadAirfieldFile() {

  HWND hProgress;

  hProgress=CreateProgressDialog(gettext(TEXT("Loading Airfield Details File...")));

  {
    OpenAirfieldDetails();
    ParseAirfieldDetails();
    CloseAirfieldDetails();
  }

}

