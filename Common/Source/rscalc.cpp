// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results
// Jarmo Lammi 1999 - 2001
// Last update July 21st, 2001

#include "stdafx.h"

#define PI 3.1415926
#define SUN_DIAMETER 0.53     // Sunradius degrees
#define AIR_REFRACTION 34.0/60.0 // athmospheric refraction degrees //

#include <windows.h>
#include "XCSoar.h"
#include "externs.h"

class SunEphemeris {


  double L,g,daylen;

  //   Get the days to J2000
  //   h is UT in decimal hours
  //   FNday only works between 1901 to 2099 - see Meeus chapter 7

  double FNday (int y, int m, int d, float h) {
    long int luku = - 7 * (y + (m + 9)/12)/4 + 275*m/9 + d;
    // type casting necessary on PC DOS and TClite to avoid overflow
    luku+= (long int)y*367;
    return (double)luku - 730531.5 + h/24.0;
  };

  //   the function below returns an angle in the range
  //   0 to 2*PI

  double FNrange (double x) {
    double b = 0.5*x / PI;
    double a = 2.0*PI * (b - (long)(b));
    if (a < 0) a = 2.0*PI + a;
    return a;
  };

  // Calculating the hourangle
  //
  double f0(double lat, double declin) {
    double fo,dfo;
    // Correction: different sign at S HS
    dfo = DEG_TO_RAD*(0.5*SUN_DIAMETER + AIR_REFRACTION); if (lat < 0.0) dfo = -dfo;
    fo = tan(declin + dfo) * tan(lat*DEG_TO_RAD);
    if (fo>0.99999) fo=1.0; // to avoid overflow //
    fo = asin(fo) + PI/2.0;
    return fo;
  };

  // Calculating the hourangle for twilight times
  //
  double f1(double lat, double declin) {
    double fi,df1;
    // Correction: different sign at S HS
    df1 = DEG_TO_RAD * 6.0; if (lat < 0.0) df1 = -df1;
    fi = tan(declin + df1) * tan(lat*DEG_TO_RAD);
    if (fi>0.99999) fi=1.0; // to avoid overflow //
    fi = asin(fi) + PI/2.0;
    return fi;
  };

  //   Find the ecliptic longitude of the Sun

  double FNsun (double d) {

    //   mean longitude of the Sun

    L = FNrange(280.461 * DEG_TO_RAD + .9856474 * DEG_TO_RAD * d);

    //   mean anomaly of the Sun

    g = FNrange(357.528 * DEG_TO_RAD + .9856003 * DEG_TO_RAD * d);

    //   Ecliptic longitude of the Sun

    return FNrange(L + 1.915 * DEG_TO_RAD * sin(g) + .02 * DEG_TO_RAD * sin(2 * g));
  };

  // Display decimal hours in hours and minutes
  void showhrmn(double dhr) {
    int hr,mn;
    hr=(int) dhr;
    mn =(int) (dhr - (double) hr)*60;

    //  printf("%0d:%0d",hr,mn);
  };


 public:

  double twam,altmax,noont,settm,riset,twpm;

  int CalcSunTimes(float longit, float latit){

    //    float intz;
    double tzone,d,lambda;
    double obliq,alpha,delta,LL,equation,ha,hb,twx;
    int y, m, day, h;

    // testing

    TIME_ZONE_INFORMATION TimeZoneInformation;
    GetTimeZoneInformation(&TimeZoneInformation);

   
#ifdef _SIM_
    m=7; day=21; y=2005; h=0;
#else
    m = GPS_INFO.Month;
    y = GPS_INFO.Year;
    day = GPS_INFO.Day;
    h = ((int)GPS_INFO.Time)/3600;
    h = (h % 24);
#endif

    int localtime = ((int)GPS_INFO.Time-TimeZoneInformation.Bias*60);
    tzone = -TimeZoneInformation.Bias/60.0;

    d = FNday(y, m, day, (float)h);

    //   Use FNsun to find the ecliptic longitude of the
    //   Sun

    lambda = FNsun(d);

    //   Obliquity of the ecliptic

    obliq = 23.439 * DEG_TO_RAD - .0000004 * DEG_TO_RAD * d;

    //   Find the RA and DEC of the Sun

    alpha = atan2(cos(obliq) * sin(lambda), cos(lambda));
    delta = asin(sin(obliq) * sin(lambda));

    // Find the Equation of Time
    // in minutes
    // Correction suggested by David Smith
    LL = L - alpha;
    if (L < PI) LL += 2.0*PI;
    equation = 1440.0 * (1.0 - LL / PI/2.0);
    ha = f0(latit,delta);
    hb = f1(latit,delta);
    twx = hb - ha;  // length of twilight in radians
    twx = 12.0*twx/PI;              // length of twilight in hours

    //  printf("ha= %.2f   hb= %.2f \n",ha,hb);

    // Conversion of angle to hours and minutes //
    daylen = RAD_TO_DEG*ha/7.5;
    if (daylen<0.0001) {daylen = 0.0;}
    // arctic winter     //

    riset = 12.0 - 12.0 * ha/PI + tzone - longit/15.0 + equation/60.0;
    settm = 12.0 + 12.0 * ha/PI + tzone - longit/15.0 + equation/60.0;
    noont = riset + 12.0 * ha/PI;
    altmax = 90.0 + delta * RAD_TO_DEG - latit; 

    // Correction for S HS suggested by David Smith
    // to express altitude as degrees from the N horizon
    if (latit < delta * RAD_TO_DEG) altmax = 180.0 - altmax;

    twam = riset - twx;     // morning twilight begin
    twpm = settm + twx;     // evening twilight end

    if (riset > 24.0) riset-= 24.0;
    if (settm > 24.0) settm-= 24.0;

    /*
      puts("\n Sunrise and set");
      puts("===============");

      printf("  year  : %d \n",(int)y);
      printf("  month : %d \n",(int)m);
      printf("  day   : %d \n\n",(int)day);
      printf("Days since Y2K :  %d \n",(int)d);

      printf("Latitude :  %3.1f, longitude: %3.1f, timezone: %3.1f \n",(float)latit,(float)longit,(float)tzone);
      printf("Declination   :  %.2f \n",delta * RAD_TO_DEG);
      printf("Daylength     : "); showhrmn(daylen); puts(" hours \n");
      printf("Civil twilight: ");
      showhrmn(twam); puts("");
      printf("Sunrise       : ");
      showhrmn(riset); puts("");

      printf("Sun altitude ");
      // Amendment by D. Smith
      printf(" %.2f degr",altmax);
      printf(latit>=0.0 ? " South" : " North");
      printf(" at noontime "); showhrmn(noont); puts("");
      printf("Sunset        : ");
      showhrmn(settm);  puts("");
      printf("Civil twilight: ");
      showhrmn(twpm);  puts("\n");
    */
    return 0;
  }


};


SunEphemeris mysun;

double DoSunEphemeris(double lon, double lat) {

  mysun.CalcSunTimes((float)lon, (float)lat);
  return (double)mysun.settm;

};

