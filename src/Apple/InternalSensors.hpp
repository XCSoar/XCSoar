// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

class SensorListener;

#import <CoreLocation/CoreLocation.h>
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
#import <CoreMotion/CoreMotion.h>
#endif

@interface LocationDelegate : NSObject <CLLocationManagerDelegate>
{
  @private SensorListener *listener;
  @private NSCalendar *gregorian_calendar;
}

-(instancetype) init __attribute__((unavailable()));

-(instancetype) init: (SensorListener *) _listener;
@end

// InternalSensors implementation which uses the Apple CoreLocation API
class InternalSensors {
  SensorListener &listener;
  CLLocationManager *location_manager;
  LocationDelegate *location_delegate;
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
  CMAltimeter *altimeter;
#endif

  void Init();
  void Deinit();

public:
  explicit InternalSensors(SensorListener &_listener);
  ~InternalSensors();
};
