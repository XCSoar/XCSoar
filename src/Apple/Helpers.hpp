// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#import <UIKit/UIKit.h>

// Define a struct to hold the inset values
struct SafeAreaInsets {
    int top;
    int right;
    int bottom;
    int left;
};

SafeAreaInsets getSafeAreaInsets();
