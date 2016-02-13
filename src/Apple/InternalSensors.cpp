/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Thread/Mutex.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"

#include <TargetConditionals.h>

#include <unistd.h>

@implementation LocationDelegate
-(instancetype) init: (unsigned int) index_
{
  self = [super init];
  if (self) {
    self->index = index_;
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
  if ((status == kCLAuthorizationStatusAuthorized)
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

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(self->index);
  basic.UpdateClock();
  if (location) {
    basic.alive.Update(basic.clock);
  } else {
    basic.alive.Clear();
  }

  basic.gps.nonexpiring_internal_gps = true;

  basic.airspeed_available.Clear();
  if (location && (location.speed >= 0.0)) {
    basic.ground_speed = location.speed;
    basic.ground_speed_available.Update(basic.clock);
  } else {
    basic.ground_speed_available.Clear();
  }

  if (location && location.timestamp) {
    basic.time = [self getSecondsOfDay: location.timestamp];
    basic.time_available.Update(basic.clock);
    basic.date_time_utc = BrokenDateTime::FromUnixTimeUTC(
        [location.timestamp timeIntervalSince1970]);
  } else {
    basic.time_available.Clear();
  }

  if (location && (location.horizontalAccuracy >= 0.0)) {
    basic.gps.hdop = location.horizontalAccuracy;
    basic.gps.real = true;
    basic.location = GeoPoint(Angle::Degrees(location.coordinate.longitude),
                              Angle::Degrees(location.coordinate.latitude));
    basic.location_available.Update(basic.clock);
  } else {
    basic.location_available.Clear();
  }

  if (location && (location.verticalAccuracy >= 0.0)) {
    basic.gps_altitude = location.altitude;
    basic.gps_altitude_available.Update(basic.clock);
  } else {
    basic.gps_altitude_available.Clear();
  }

  if (location && (location.course >= 0.0)) {
    basic.track = Angle::Degrees(location.course);
    basic.track_available.Update(basic.clock);
  } else {
    basic.track_available.Clear();
  }

  device_blackboard->ScheduleMerge();
}

- (void)locationManager:(CLLocationManager *)manager
    didFailWithError:(NSError *)error
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(self->index);
  if ([error code] != kCLErrorHeadingFailure) {
    basic.alive.Clear();
    basic.location_available.Clear();
  }
  device_blackboard->ScheduleMerge();
}
@end


InternalSensors::InternalSensors(unsigned int _index)
    : index(_index)
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
  location_delegate = [[LocationDelegate alloc] init: index];
  location_manager.desiredAccuracy =
      kCLLocationAccuracyBestForNavigation;
  location_manager.delegate = location_delegate;
#if TARGET_OS_IPHONE
  if ([location_manager
      respondsToSelector: @selector(requestWhenInUseAuthorization)]) {
    CLAuthorizationStatus status = [CLLocationManager authorizationStatus];
    if ((status == kCLAuthorizationStatusAuthorized)
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

InternalSensors * InternalSensors::Create(unsigned int index)
{
  return new InternalSensors(index);
}
