/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#include "Conversion.hpp"
#include "DateTime.hpp"
#include "Math/Angle.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

#define IMI_SECONDS_IN_MINUTE       (60)
#define IMI_SECONDS_IN_HOUR      (60*60)
#define IMI_SECONDS_IN_DAY    (24*60*60)
#define IMI_ISLEAP(y) ((y & 3) == 0) //simple version valid for years 2000-2099
#define IMI_DAYS_IN_YEAR(year) (IMI_ISLEAP(year) ? 366 : 365)

static const IMI::IMIBYTE IMI_DAYS_IN_MONTH[12] =
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

void
IMI::unicode2usascii(const TCHAR* unicode, char* ascii, int outSize)
{
#ifdef _UNICODE
  WideCharToMultiByte(CP_ACP, 0, unicode, -1, ascii, outSize, "?", NULL);
#else
  strncpy(ascii, unicode, outSize - 1);
  ascii[outSize - 1] = 0;
#endif
}

IMI::AngleConverter::AngleConverter(Angle angle)
{
  sign = (angle.sign() == -1) ? 1 : 0;
  double mag = angle.magnitude_degrees();
  degrees = static_cast<IMIDWORD> (mag);
  milliminutes = static_cast<IMIDWORD> ((mag - degrees) * 60 * 1000);
}

void
IMI::Convert(IMI::IMIDATETIMESEC in, BrokenDateTime &out)
{
  if (in >= IMI_SECONDS_IN_DAY) {
    // find year
    for (out.year = 0; out.year <= 99; out.year++) {
      unsigned secondsinyear = IMI_DAYS_IN_YEAR(out.year) * IMI_SECONDS_IN_DAY;
      if (in < secondsinyear)
        break;

      in -= secondsinyear;
    }

    // find month
    for (out.month = 0; out.month < 12; out.month++) {
      unsigned secondsinmonth = IMI_DAYS_IN_MONTH[out.month]
          * IMI_SECONDS_IN_DAY;
      if (out.month == 1 && IMI_ISLEAP(out.year))
        secondsinmonth += IMI_SECONDS_IN_DAY;

      if (in < secondsinmonth)
        break;
      in -= secondsinmonth;
    }

    // calculate day
    out.day = (uint8_t)(in / IMI_SECONDS_IN_DAY);
    in -= (out.day) * IMI_SECONDS_IN_DAY;
  } else {
    out.year = 0;
    out.month = 0;
    out.day = 0;
  }

  // hour, minutes and seconds
  out.hour = (uint8_t)(in / IMI_SECONDS_IN_HOUR);
  in -= (out.hour) * IMI_SECONDS_IN_HOUR;

  out.minute = (uint8_t)(in / IMI_SECONDS_IN_MINUTE);
  in -= (out.minute) * IMI_SECONDS_IN_MINUTE;

  out.second = (uint8_t)in;

  out.year += 2000;
  out.month++;
  out.day++;
  out.hour++;
  out.minute++;
  out.second++;
}
