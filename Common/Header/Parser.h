#if !defined(AFX_PARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_PARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include "NMEA/Info.h"

#include <tchar.h>

class NMEAParser {
 public:
  NMEAParser();
  static void UpdateMonitor(void);
  static bool ParseNMEAString(int portnum,
                              const TCHAR *String, NMEA_INFO *GPS_INFO);
  static void Reset(void);
  static bool PortIsFlarm(int portnum);
  void _Reset(void);

  bool ParseNMEAString_Internal(const TCHAR *String, NMEA_INFO *GPS_INFO);
  bool gpsValid;
  int nSatellites;

  bool activeGPS;
  bool isFlarm;

  static int StartDay;

 public:
  static void TestRoutine(NMEA_INFO *GPS_INFO);

  // these routines can be used by other parsers.
  static double ParseAltitude(const TCHAR *, const TCHAR *);
  static size_t ValidateAndExtract(const TCHAR *src, TCHAR *dst, size_t dstsz,
                                   const TCHAR **arr, size_t arrsz);
  static size_t ExtractParameters(const TCHAR *src, TCHAR *dst,
                                  const TCHAR **arr, size_t sz);
  static bool NMEAChecksum(const TCHAR *String);

  static void ExtractParameter(const TCHAR *Source,
			       TCHAR *Destination,
			       int DesiredFieldNumber);

 private:
  bool GSAAvailable;
  bool GGAAvailable;
  bool RMZAvailable;
  bool RMAAvailable;
  double RMZAltitude;
  double RMAAltitude;
  double LastTime;

  bool TimeHasAdvanced(double ThisTime, NMEA_INFO *GPS_INFO);
  static double TimeModify(double FixTime, NMEA_INFO* info);

  bool GLL(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool GGA(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool GSA(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool RMC(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool RMB(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool RMA(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool RMZ(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);

  bool WP0(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool WP1(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool WP2(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);

  // Additional sentances
  bool PTAS1(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);  // RMN: Tasman instruments.  TAS, Vario, QNE-altitude

  // FLARM sentances
  bool PFLAU(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  bool PFLAA(const TCHAR *String, const TCHAR **, size_t, NMEA_INFO *GPS_INFO);
};

extern double AccelerometerZero;
extern bool SetSystemTimeFromGPS;

void FLARM_RefreshSlots(NMEA_INFO *GPS_INFO);

extern bool EnableLogNMEA;
void LogNMEA(const TCHAR* text);

#endif
