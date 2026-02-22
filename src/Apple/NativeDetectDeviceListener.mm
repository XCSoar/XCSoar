// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#import "NativeDetectDeviceListener.h"

@implementation NativeDetectDeviceListener {
  DetectDeviceListener *_cppListener;
}

- (instancetype)initWithCppListener:(DetectDeviceListener *)listener
{
  self = [super init];
  if (self) {
    _cppListener = listener;
  }
  return self;
}

- (void)onDeviceDetected:(int)type
                 address:(NSString *)address
                    name:(NSString *)name
                features:(uint64_t)features
{
  if (_cppListener) {
    _cppListener->OnDeviceDetected(
        static_cast<DetectDeviceListener::Type>(type), [address UTF8String],
        name ? [name UTF8String] : nullptr, features);
  }
}
@end
