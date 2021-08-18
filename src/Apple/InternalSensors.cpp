/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Apple/InternalSensors.hpp"
#include "Device/SensorListener.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/FloatDuration.hxx"
#include "time/SystemClock.hxx"

#include <TargetConditionals.h>

#include <unistd.h>

@implementation LocationDelegate
-(instancetype) init: (SensorListener *) _listener
{
  self = [super init];
  if (self) {
    self->listener = _listener;
    gregorian_calendar = [[NSCalendar alloc]
      initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
  }
  return self;
}

-(double) getSecondsOfDay: (NSDate*) date
{
  NSDateComponents *components = [gregorian_calendar
    components: NSIntegerMax
    fromDate: date];
  [components setHour:0];
  [components setMinute:0];
  [components setSecond:0];
  NSDate *midnight = [gregorian_calendar dateFromComponents:components];
  return [date timeIntervalSinceDate: midnight];
}

#if TARGET_OS_IPHONE
- (void) locationManager:(CLLocationManager *)manager
    didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
  if ((status == kCLAuthorizationStatusAuthorizedAlways)
      || (status == kCLAuthorizationStatusAuthorizedWhenInUse)) {
    [manager startUpdatingLocation];
  }
}
#endif

-(void) locationManager:(CLLocationManager *)manager
    didUpdateLocations:(NSArray *)locations
{
  CLLocation *location;
  if (locations)
    location = locations.lastObject;
  else
    location = nil;

  if (!location || !location.timestamp || location.horizontalAccuracy < 0.0) {
    self->listener->OnConnected(1);
    return;
  }

  const auto time = TimePointAfterUnixEpoch(FloatDuration{[location.timestamp timeIntervalSince1970]});

  const GeoPoint l(Angle::Degrees(location.coordinate.longitude),
                   Angle::Degrees(location.coordinate.latitude));

  self->listener->OnConnected(2);
  self->listener->OnLocationSensor(time, -1, l,
                                   location.verticalAccuracy >= 0.0,
                                   /* CoreLocation provides geoidal
                                      altitude */
                                   true,
                                   location.altitude,
                                   location.course >= 0.0,
                                   location.course,
                                   location.speed >= 0.0,
                                   location.speed,
                                   true, location.horizontalAccuracy);
}

- (void)locationManager:(CLLocationManager *)manager
    didFailWithError:(NSError *)error
{
  self->listener->OnConnected(0);
}
@end


InternalSensors::InternalSensors(SensorListener &_listener)
  :listener(_listener)
{
  if ([NSThread isMainThread]) {
    Init();
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      Init();
    });
  }
}

InternalSensors::~InternalSensors()
{
  if ([NSThread isMainThread]) {
    Deinit();
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      Deinit();
    });
  }
}

void InternalSensors::Init()
{
  location_manager = [[CLLocationManager alloc] init];
  location_delegate = [[LocationDelegate alloc] init: &listener];
  location_manager.desiredAccuracy =
      kCLLocationAccuracyBestForNavigation;
  location_manager.delegate = location_delegate;
#if TARGET_OS_IPHONE
  if ([location_manager
      respondsToSelector: @selector(requestWhenInUseAuthorization)]) {
    CLAuthorizationStatus status = [CLLocationManager authorizationStatus];
    if ((status == kCLAuthorizationStatusAuthorizedAlways)
        || (status == kCLAuthorizationStatusAuthorizedWhenInUse)) {
      [location_manager startUpdatingLocation];
    } else {
      [location_manager requestWhenInUseAuthorization];
    }
  } else {
    [location_manager startUpdatingLocation];
  }
#else
  [location_manager startUpdatingLocation];
#endif
}

void InternalSensors::Deinit()
{
  [location_manager stopUpdatingLocation];
}
