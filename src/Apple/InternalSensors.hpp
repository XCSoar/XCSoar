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

/**
 * Objective-C delegate class that handles CoreLocation callbacks.
 * Bridges between Apple's location services and XCSoar's SensorListener interface.
 * Manages background execution and data format conversions.
 */
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

/**
 * Cross-platform sensor interface implementation for Apple platforms.
 * 
 * Provides access to device sensors including:
 * - GPS location services via CoreLocation
 * - Barometric pressure sensing via CoreMotion (iOS only)
 * 
 * Handles platform-specific permission models and thread safety requirements.
 * All sensor data is forwarded to the provided SensorListener interface.
 * 
 * @note macOS support is limited to location services only
 * @note iOS includes additional barometric pressure support when available
 */
class InternalSensors {
  SensorListener &listener;
  CLLocationManager *location_manager;
  LocationDelegate *location_delegate;
#if TARGET_OS_IPHONE
  CMAltimeter *altimeter;
  CMMotionActivityManager *motion_activity_manager;
  NSOperationQueue *motion_activity_queue;
#endif

  void Init();
  void Deinit();
  
#if TARGET_OS_IPHONE
  /**
   * Initialize and start barometric pressure updates from device altimeter.
   * Configures CMAltimeter with appropriate handler for pressure readings.
   */
  void StartAltimeterUpdates();
#endif

public:
  /**
   * Construct sensor interface with the specified listener.
   * Automatically initializes all available sensors on the main thread.
   * 
   * @param _listener Target for sensor data callbacks
   */
  explicit InternalSensors(SensorListener &_listener);
  ~InternalSensors();
};

#endif // __APPLE__