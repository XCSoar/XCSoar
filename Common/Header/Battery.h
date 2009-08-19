#ifndef XCSOAR_BATTERY_H
#define XCSOAR_BATTERY_H

#if !defined(GNAV) && WINDOWSPC < 1

extern int PDABatteryTemperature;
extern int PDABatteryPercent;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Battery status for SIMULATOR mode
//	30% reminder, 20% exit, 30 second reminders on warnings
#define BATTERY_WARNING 30
#define BATTERY_EXIT 20
#define BATTERY_REMINDER 30000

extern DWORD BatteryWarningTime;

typedef struct
{
  BYTE acStatus;
  // 0 offline
  // 1 online
  // 255 unknown
  BYTE chargeStatus;
  // 1 high
  // 2 low
  // 4 critical
  // 8 charging
  // 128 no system battery
  // 255 unknown
  BYTE BatteryLifePercent;
  // 0-100 or 255 if unknown
  // VENTA-TEST BATTERY
  DWORD BatteryVoltage;
  DWORD BatteryCurrent;
  DWORD BatteryAverageCurrent;
  DWORD BatterymAHourConsumed;
  DWORD BatteryTemperature;
  DWORD BatteryLifeTime;
  DWORD BatteryFullLifeTime;
// END VENTA-TEST

} BATTERYINFO;

#ifdef __cplusplus
extern "C" {
#endif

DWORD GetBatteryInfo(BATTERYINFO* pBatteryInfo);

#ifdef __cplusplus
}
#endif

#else /* GNAV || WINDOWSPC >= 1 */

enum {
  PDABatteryPercent = 100,
  PDABatteryTemperature = 0,
};

#endif

#endif
