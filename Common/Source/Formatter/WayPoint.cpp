#include "Formatter/WayPoint.hpp"
#include "WayPoint.hpp"
#include "XCSoar.h"
#include "externs.h"
#include "MapWindow.h"

const TCHAR *FormatterWaypoint::Render(int *color) {
  int thewaypoint = ActiveWayPoint;
  LockTaskData();
  if(ValidTaskPoint(thewaypoint))
    {
      int index = Task[thewaypoint].Index;
      if ((index>=0) && (WayPointList[index].Reachable)) {
	*color = 2; // blue text
      } else {
	*color = 0; // black text
      }
      if ( DisplayTextType == DISPLAYFIRSTTHREE)
        {
          _tcsncpy(Text,WayPointList[index].Name,3);
          Text[3] = '\0';
        }
      else if( DisplayTextType == DISPLAYNUMBER)
        {
          _stprintf(Text,_T("%d"),
		    WayPointList[index].Number );
        }
      else
        {
          _tcsncpy(Text,WayPointList[index].Name,
                   (sizeof(Text)/sizeof(TCHAR))-1);
          Text[(sizeof(Text)/sizeof(TCHAR))-1] = '\0';
        }
    }
  else
    {
      Valid = false;
      RenderInvalid(color);
    }
  UnlockTaskData();

  return(Text);
}

// VENTA3 Alternate destinations
const TCHAR *FormatterAlternate::RenderTitle(int *color) {

  LockTaskData();
  if(ValidWayPoint(ActiveAlternate))
    {
      if ( DisplayTextType == DISPLAYFIRSTTHREE)
        {
          _tcsncpy(Text,WayPointList[ActiveAlternate].Name,3);
          Text[3] = '\0';
        }
      else if( DisplayTextType == DISPLAYNUMBER)
        {
          _stprintf(Text,_T("%d"),
		    WayPointList[ActiveAlternate].Number );
        }
      else
        {
          _tcsncpy(Text,WayPointList[ActiveAlternate].Name,
                   (sizeof(Text)/sizeof(TCHAR))-1);
          Text[(sizeof(Text)/sizeof(TCHAR))-1] = '\0';
        }
    }
  else
    {
      Valid = false;
      RenderInvalid(color);
    }
  UnlockTaskData();

  return(Text);
}


/*
 * Currently even if set for FIVV, colors are not used.
 */
const TCHAR *
FormatterAlternate::Render(int *color)
{
  LockTaskData();
  if(Valid && ValidWayPoint(ActiveAlternate)) {
	switch (WayPointCalc[ActiveAlternate].VGR ) {
		case 0:
			// impossible, give a magenta debug color;
			*color = 5;
			break;
		case 1:
#ifdef FIVV
			*color = 0; // green
#else
			*color = 0; // blue
#endif
			break;
		case 2:
#ifdef FIVV
			*color = 0; // yellow 4
#else
			*color = 0; // normale white
#endif
			break;
		case 3:
			*color = 1; // red
			break;
		default:
			// even more impossible, give debug color magenta
			*color = 5;
			break;
	}

	Value=WayPointCalc[ActiveAlternate].GR;

	_stprintf(Text,Format,Value);
  } else {
	Valid = false;
	RenderInvalid(color);
  }
   UnlockTaskData();
   return(Text);
}


void FormatterAlternate::AssignValue(int i) {
  LockTaskData();
   switch (i) {
	case 67:
		if (OnAlternate1 == false ) { // first run, activate calculations
			OnAlternate1 = true;
        		Value=INVALID_GR;
		} else {
			if ( ValidWayPoint(Alternate1) ) Value=WayPointCalc[Alternate1].GR;
			else Value=INVALID_GR;
		}
		break;
/*
		if ( ValidWayPoint(Alternate1) ) Value=WayPointCalc[Alternate1].GR;
		else Value=INVALID_GR;
		break;
*/
	case 68:
		if (OnAlternate2 == false ) { // first run, activate calculations
			OnAlternate2 = true;
        		Value=INVALID_GR;
		} else {
			if ( ValidWayPoint(Alternate2) ) Value=WayPointCalc[Alternate2].GR;
			else Value=INVALID_GR;
		}
		break;
	case 69:
		if (OnBestAlternate == false ) { // first run, waiting for slowcalculation loop
			OnBestAlternate = true;		// activate it
        		Value=INVALID_GR;
		} else {
			if ( ValidWayPoint(BestAlternate)) Value=WayPointCalc[BestAlternate].GR;
			else Value=INVALID_GR;
		}
		break;
	default:
		Value=66.6; // something evil to notice..
		break;
   }

   Valid=false;
   if (Value < INVALID_GR) {
    	Valid = true;
	if (Value >= 100 )
	  {
	    _tcscpy(Format, _T("%1.0f"));
	  }
	else
	  {
	    _tcscpy(Format, _T("%1.1f"));
	  }
   }

   UnlockTaskData();
}
