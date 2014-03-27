/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#import <CoreLocation/CoreLocation.h>

#include <unistd.h>

@interface LocationDelegate : NSObject <CLLocationManagerDelegate>
{
 @private
  unsigned int index;

 @private
  NSCalendar *gregorian_calendar;
}

-(instancetype) init __attribute__((unavailable()));

-(instancetype) init: (unsigned int) index_;
@end


@implementation LocationDelegate
-(instancetype) init: (unsigned int) index_
{
    self = [super init];
    if (self) {
        self->index = index_;
        gregorian_calendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    }
    return self;
}

-(void) dealloc
{
  [gregorian_calendar dealloc];
  [super dealloc];
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
  return [midnight timeIntervalSinceDate: midnight];
}

-(void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
  CLLocation *location = locations.lastObject;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(self->index);
  basic.UpdateClock();
  if (location) {
    basic.alive.Update(basic.clock);
  } else {
    basic.alive.Clear();
  }

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
    basic.date_time_utc = BrokenDateTime::FromUnixTimeUTC([location.timestamp timeIntervalSince1970]);
  } else {
    basic.time_available.Clear();
  }

  if (location) {
    basic.gps.real = true;
    basic.location = GeoPoint(Angle::Degrees(location.coordinate.longitude),
                              Angle::Degrees(location.coordinate.latitude));
    basic.location_available.Update(basic.clock);
  } else {
    basic.location_available.Clear();
  }

  if (location) {
    basic.gps_altitude = location.altitude;
    basic.gps_altitude_available.Update(basic.clock);
  } else {
    basic.gps_altitude_available.Clear();
  }

  if (location) {
    basic.track = Angle::Degrees(location.course);
    basic.track_available.Update(basic.clock);
  } else {
    basic.track_available.Clear();
  }

  device_blackboard->ScheduleMerge();
}
@end


struct InternalSensorsPrivate
{
  unsigned int index;
  CLLocationManager *locationManager;
  LocationDelegate* locationDelegate;
};


InternalSensors::InternalSensors(unsigned int index)
    : private_data(new InternalSensorsPrivate)
{
  private_data->index = index;

  if ([NSThread isMainThread]) {
    init();
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      init();
    });
  }
}

InternalSensors::~InternalSensors()
{
  if ([NSThread isMainThread]) {
    deinit();
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      deinit();
    });
  }
  delete private_data;
}

void InternalSensors::init()
{
  private_data->locationManager = [[CLLocationManager alloc] init];
  private_data->locationDelegate = [[LocationDelegate alloc] init: private_data->index];
  private_data->locationManager.desiredAccuracy = kCLLocationAccuracyBestForNavigation;
  private_data->locationManager.delegate = private_data->locationDelegate;
  [private_data->locationManager startUpdatingLocation];
}

void InternalSensors::deinit()
{
  [private_data->locationManager stopUpdatingLocation];
  [private_data->locationManager dealloc];
  [private_data->locationDelegate dealloc];
}

InternalSensors * InternalSensors::create(unsigned int index)
{
  return new InternalSensors(index);
}
