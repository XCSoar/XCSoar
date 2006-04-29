
#include "stdafx.h"
#include "Utils.h"

#define EGM96SIZE 16200

unsigned char* egm96data= NULL;

extern HINSTANCE hInst;

void OpenGeoid(void) {
  LPTSTR lpRes;
  HRSRC hResInfo;
  HGLOBAL hRes;
  int len;
  hResInfo = FindResource (hInst, TEXT("IDR_RASTER_EGM96S"), TEXT("RASTERDATA"));

  if (hResInfo == NULL) {
    // unable to find the resource
    egm96data = NULL;
    return;
  }
  // Load the wave resource.
  hRes = LoadResource (hInst, hResInfo);
  if (hRes == NULL) {
    // unable to load the resource
    egm96data = NULL;
    return;
  }

  // Lock the wave resource and do something with it.
  lpRes = (LPTSTR)LockResource (hRes);

  if (lpRes) {
    len = SizeofResource(hInst,hResInfo);
    if (len==EGM96SIZE) {
      egm96data = (unsigned char*)malloc(len);
      strncpy((char*)egm96data,(char*)lpRes,len);
    } else {
      egm96data = NULL;
      return;
    }
  }
  return;
}


void CloseGeoid(void) {
  if (egm96data) {
    free(egm96data);
    egm96data = NULL;
  }
}


double LookupGeoidSeparation(double lat, double lon) {
  if (!egm96data) return 0.0;

  int ilat, ilon;
  ilat = iround((90.0-lat)/2.0);
  if (lon<0) {
    lon+= 360.0;
  }
  ilon = iround(lon/2.0);

  int offset = ilat*180+ilon;
  if (offset>=EGM96SIZE)
    return 0.0;
  if (offset<0)
    return 0.0;

  double val = (double)(egm96data[offset])-127;
  return val;
}

