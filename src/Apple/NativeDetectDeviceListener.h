// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DetectDeviceListener.hpp"
#import <Foundation/Foundation.h>

@interface NativeDetectDeviceListener : NSObject
- (instancetype)initWithCppListener:(DetectDeviceListener *)listener;
- (void)onDeviceDetected:(int)type
                 address:(NSString *)address
                    name:(NSString *)name
                features:(uint64_t)features;
@end
