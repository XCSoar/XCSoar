// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#if TARGET_OS_IPHONE
    background_task = UIBackgroundTaskInvalid;
    
    // Register for background/foreground notifications
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationDidEnterBackground:)
                                                 name:UIApplicationDidEnterBackgroundNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillEnterForeground:)
                                                 name:UIApplicationWillEnterForegroundNotification
                                               object:nil];
#endif
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
    
    // Configure for background location if we have "Always" permission
    if (status == kCLAuthorizationStatusAuthorizedAlways) {
      if (@available(iOS 9.0, *)) {
        manager.allowsBackgroundLocationUpdates = YES;
      }
      if (@available(iOS 11.0, *)) {
        manager.showsBackgroundLocationIndicator = NO;
      }
    }
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

#if TARGET_OS_IPHONE
- (void)applicationDidEnterBackground:(NSNotification *)notification
{
  // Start a background task to allow location updates to continue briefly
  background_task = [[UIApplication sharedApplication] 
    beginBackgroundTaskWithName:@"LocationUpdates" 
    expirationHandler:^{
      // Clean up when the background task expires
      if (background_task != UIBackgroundTaskInvalid) {
        [[UIApplication sharedApplication] endBackgroundTask:background_task];
        background_task = UIBackgroundTaskInvalid;
      }
    }];
}

- (void)applicationWillEnterForeground:(NSNotification *)notification
{
  // End the background task when returning to foreground
  if (background_task != UIBackgroundTaskInvalid) {
    [[UIApplication sharedApplication] endBackgroundTask:background_task];
    background_task = UIBackgroundTaskInvalid;
  }
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  if (background_task != UIBackgroundTaskInvalid) {
    [[UIApplication sharedApplication] endBackgroundTask:background_task];
    background_task = UIBackgroundTaskInvalid;
  }
}
#endif

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
  // Configure location manager for background operation
  location_manager.distanceFilter = 10.0; // Update every 10 meters
  location_manager.pausesLocationUpdatesAutomatically = NO;
  
  if ([location_manager
      respondsToSelector: @selector(requestWhenInUseAuthorization)]) {
    CLAuthorizationStatus status = [CLLocationManager authorizationStatus];
    if (status == kCLAuthorizationStatusAuthorizedAlways) {
      // We already have "Always" permission, configure for background and start
      if (@available(iOS 9.0, *)) {
        location_manager.allowsBackgroundLocationUpdates = YES;
      }
      if (@available(iOS 11.0, *)) {
        location_manager.showsBackgroundLocationIndicator = NO;
      }
      [location_manager startUpdatingLocation];
    } else if (status == kCLAuthorizationStatusAuthorizedWhenInUse) {
      // We have "When In Use", request upgrade to "Always" for background usage
      if (@available(iOS 11.0, *)) {
        [location_manager requestAlwaysAuthorization];
      } else {
        [location_manager startUpdatingLocation];
      }
    } else if (status == kCLAuthorizationStatusNotDetermined) {
      // First time - request "When In Use" first
      [location_manager requestWhenInUseAuthorization];
    } else {
      // Denied or restricted - start without background capability
      [location_manager startUpdatingLocation];
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
