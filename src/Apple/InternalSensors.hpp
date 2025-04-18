// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class SensorListener;

#import <CoreLocation/CoreLocation.h>

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

  void Init();
  void Deinit();

public:
  explicit InternalSensors(SensorListener &_listener);
  ~InternalSensors();
};
