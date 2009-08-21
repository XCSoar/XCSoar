#include "Polar/WinPilot.hpp"
#include "McReady.h"
#include "Utils.h"
#include "externs.h"
#include "Registry.hpp"
#include "Utils.h"
#include "LocalPath.hpp"

#include <math.h>

void PolarWinPilot2XCSoar(double POLARV[3], double POLARW[3], double ww[2]) {
  double d;
  double v1,v2,v3;
  double w1,w2,w3;

  v1 = POLARV[0]/3.6; v2 = POLARV[1]/3.6; v3 = POLARV[2]/3.6;
  //	w1 = -POLARV[0]/POLARLD[0];
  //    w2 = -POLARV[1]/POLARLD[1];
  //    w3 = -POLARV[2]/POLARLD[2];
  w1 = POLARW[0]; w2 = POLARW[1]; w3 = POLARW[2];

  d = v1*v1*(v2-v3)+v2*v2*(v3-v1)+v3*v3*(v1-v2);
  if (d == 0.0)
    {
      POLAR[0]=0;
    }
  else
    {
      POLAR[0]=((v2-v3)*(w1-w3)+(v3-v1)*(w2-w3))/d;
    }
  d = v2-v3;
  if (d == 0.0)
    {
      POLAR[1]=0;
    }
  else
    {
      POLAR[1] = (w2-w3-POLAR[0]*(v2*v2-v3*v3))/d;
    }


  WEIGHTS[0] = 70;                      // Pilot weight
  WEIGHTS[1] = ww[0]-WEIGHTS[0];        // Glider empty weight
  WEIGHTS[2] = ww[1];                   // Ballast weight

  POLAR[2] = (double)(w3 - POLAR[0] *v3*v3 - POLAR[1]*v3);

  // now scale off weight
  POLAR[0] = POLAR[0] * (double)sqrt(WEIGHTS[0] + WEIGHTS[1]);
  POLAR[2] = POLAR[2] / (double)sqrt(WEIGHTS[0] + WEIGHTS[1]);

}

bool ReadWinPilotPolar(void) {

  TCHAR	szFile[MAX_PATH] = TEXT("\0");
  TCHAR ctemp[80];
  TCHAR TempString[READLINE_LENGTH+1];
  FILE *file;

  double POLARV[3];
  double POLARW[3];
  double ww[2];
  bool foundline = false;

#ifdef HAVEEXCEPTIONS
  __try{
#endif

    ww[0]= 403.0; // 383
    ww[1]= 101.0; // 121
    POLARV[0]= 115.03;
    POLARW[0]= -0.86;
    POLARV[1]= 174.04;
    POLARW[1]= -1.76;
    POLARV[2]= 212.72;
    POLARW[2]= -3.4;

    GetRegistryString(szRegistryPolarFile, szFile, MAX_PATH);
    ExpandLocalPath(szFile);

    #ifndef HAVEEXCEPTIONS
    SetRegistryString(szRegistryPolarFile, TEXT("\0"));
    #endif

    file = _tfopen(szFile, TEXT("rt"));

    if (file != NULL) {

#ifdef HAVEEXCEPTIONS
      __try{
#endif
      int *p=NULL; // test, force an exception
      p=0;

        while(ReadStringX(file,READLINE_LENGTH,TempString) && (!foundline)){

          if(_tcsstr(TempString,TEXT("*")) != TempString) // Look For Comment
            {
              PExtractParameter(TempString, ctemp, 0);
              ww[0] = StrToDouble(ctemp,NULL);

              PExtractParameter(TempString, ctemp, 1);
              ww[1] = StrToDouble(ctemp,NULL);

              PExtractParameter(TempString, ctemp, 2);
              POLARV[0] = StrToDouble(ctemp,NULL);
              PExtractParameter(TempString, ctemp, 3);
              POLARW[0] = StrToDouble(ctemp,NULL);

              PExtractParameter(TempString, ctemp, 4);
              POLARV[1] = StrToDouble(ctemp,NULL);
              PExtractParameter(TempString, ctemp, 5);
              POLARW[1] = StrToDouble(ctemp,NULL);

              PExtractParameter(TempString, ctemp, 6);
              POLARV[2] = StrToDouble(ctemp,NULL);
              PExtractParameter(TempString, ctemp, 7);
              POLARW[2] = StrToDouble(ctemp,NULL);

              PolarWinPilot2XCSoar(POLARV, POLARW, ww);
	      GlidePolar::WingArea = 0.0;

              foundline = true;
            }
        }

        // file was OK, so save it
        if (foundline) {
          ContractLocalPath(szFile);
          SetRegistryString(szRegistryPolarFile, szFile);
        }
#ifdef HAVEEXCEPTIONS
      }__finally
#endif
      {
        fclose(file);
      }
    }
#ifdef HAVEEXCEPTIONS
  }__except(EXCEPTION_EXECUTE_HANDLER){
    foundline = false;
  }
#endif
  return(foundline);

}

// *LS-3	WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
// 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,	-3.4
