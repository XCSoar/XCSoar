#include "Formatter/TeamCode.hpp"
#include "externs.h"

const TCHAR *FormatterTeamCode::Render(int *color) {

  if(ValidWayPoint(TeamCodeRefWaypoint))
    {
      *color = 0; // black text
       _tcsncpy(Text,CALCULATED_INFO.OwnTeamCode,5);
       Text[5] = '\0';
    }
  else
    {
      RenderInvalid(color);
    }

  return(Text);
}


const TCHAR *FormatterDiffTeamBearing::Render(int *color) {

  if(ValidWayPoint(TeamCodeRefWaypoint) && TeammateCodeValid) {
    Valid = true;

    Value = CALCULATED_INFO.TeammateBearing -  GPS_INFO.TrackBearing;

    if (Value < -180.0)
      Value += 360.0;
    else
      if (Value > 180.0)
        Value -= 360.0;

#ifndef __MINGW32__
    if (Value > 1)
      _stprintf(Text, TEXT("%2.0f°»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("«%2.0f°"), -Value);
    else
      _tcscpy(Text, TEXT("«»"));
#else
    if (Value > 1)
      _stprintf(Text, TEXT("%2.0fÂ°Â»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("Â«%2.0fÂ°"), -Value);
    else
      _tcscpy(Text, TEXT("Â«Â»"));
#endif
    *color = 0;

  } else {
    Valid = false;
    RenderInvalid(color);
  }

  return(Text);
}


