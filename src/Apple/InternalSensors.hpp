// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>

class SensorListener;

#import <CoreLocation/CoreLocation.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <CoreMotion/CoreMotion.h>
#endif

@interface LocationDelegate : NSObject <CLLocationManagerDelegate>
{
  @private SensorListener *listener;
  @private NSCalendar *gregorian_calendar;
#if TARGET_OS_IPHONE
  @private UIBackgroundTaskIdentifier background_task;
#endif
}

-(instancetype) init __attribute__((unavailable()));

-(instancetype) init: (SensorListener *) _listener;
@end

// InternalSensors implementation which uses the Apple CoreLocation API
class InternalSensors {
  SensorListener &listener;
  CLLocationManager *location_manager;
  LocationDelegate *location_delegate;
#if TARGET_OS_IPHONE
  CMAltimeter *altimeter;
#endif

  void Init();
  void Deinit();

public:
  explicit InternalSensors(SensorListener &_listener);
  ~InternalSensors();
};

#endif // __APPLE__