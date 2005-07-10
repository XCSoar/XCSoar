//   $Id: AirfieldDetails.cpp,v 1.8 2005/07/10 11:30:42 jwharington Exp $


#include "AirfieldDetails.h"

#include "stdafx.h"
#include "Utils.h"
#include "Sizes.h"
#include "externs.h"
#include "Dialogs.h"
#include "resource.h"
#include "Utils.h"

#include <aygshell.h>

HANDLE hAirfieldDetails;

extern TCHAR szRegistryAirfieldFile[];

void OpenAirfieldDetails() {

  static TCHAR  szFile[MAX_PATH] = TEXT("\0");

  GetRegistryString(szRegistryAirfieldFile, szFile, MAX_PATH);

  hAirfieldDetails = NULL;
  hAirfieldDetails = CreateFile(szFile,GENERIC_READ,0,NULL,
				OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if( hAirfieldDetails == NULL)
    {
      return;
    }

};


void CloseAirfieldDetails() {
  if (hAirfieldDetails) {
    CloseHandle(hAirfieldDetails);
    hAirfieldDetails = NULL;
  }
};


void LookupAirfieldDetail(TCHAR *Name, TCHAR *Details) {
  int i;
  TCHAR UName[100];

  for(i=0;i<(int)NumberOfWayPoints;i++)
    {
      if ((WayPointList[i].Flags & AIRPORT) == AIRPORT) {
	_tcscpy(UName, WayPointList[i].Name);
	CharUpper(UName);

	if (_tcscmp(UName, Name)==0) {
	  
	  if (WayPointList[i].Details) {
	    free(WayPointList[i].Details);
	  }
	  WayPointList[i].Details = (TCHAR*)malloc((_tcslen(Details)+1)*sizeof(TCHAR));
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
  TCHAR Details[1000];
  TCHAR Name[100];

  BOOL inDetails = FALSE;
  int i;
  int k=0;

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

  hProgress=CreateProgressDialog(TEXT("Loading Airfield Details File..."));

  {
    OpenAirfieldDetails();
    ParseAirfieldDetails();
    CloseAirfieldDetails();
  }

}

