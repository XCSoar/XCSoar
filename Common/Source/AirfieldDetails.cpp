#include "AirfieldDetails.h"

#include "stdafx.h"
#include "Utils.h"
#include "Sizes.h"
#include "externs.h"

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

  for(i=0;i<NumberOfWayPoints;i++)
    {
      if ((WayPointList[i].Flags & AIRPORT) == AIRPORT) {
	if (_tcscmp(WayPointList[i].Name,Name)==0) {

	  if (WayPointList[i].Details) {
	    free(WayPointList[i].Details);
	  }
	  WayPointList[i].Details = (TCHAR*)malloc((_tcslen(Details)+1)*sizeof(TCHAR));
	  _tcscpy(WayPointList[i].Details, Details);
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
	Name[i]= 0;

	inDetails = TRUE;
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
  OpenAirfieldDetails();
  ParseAirfieldDetails();
  CloseAirfieldDetails();
}

