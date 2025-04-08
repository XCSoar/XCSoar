// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Apple/Helpers.hpp"

#import <UIKit/UIKit.h>

/* Little helper function to return dimensions for the SafaArea insets
Needs some manual tweeking, mainly because we don't account for the 'dynamic island' size changes
 and instead just use a fixed height
If we update to SDL3, we can just use the built-in function SDL_GetWindowSafeArea instead
*/
SafeAreaInsets getSafeAreaInsets(void) {
    if (@available(iOS 11.0, *)) {
        UIWindow *keyWindow = UIApplication.sharedApplication.windows.firstObject;
        if (keyWindow) {
            UIEdgeInsets dynamicInsets = keyWindow.safeAreaInsets;
            return (SafeAreaInsets){
                static_cast<int>(dynamicInsets.top),
                static_cast<int>(dynamicInsets.left),
                static_cast<int>(dynamicInsets.bottom),
                static_cast<int>(dynamicInsets.right)
            };
        }
    }
    
    // Fallback to manual insets based on screen size
    CGSize screenSize = [UIScreen mainScreen].bounds.size;
    if (screenSize.width > screenSize.height) {
        CGFloat temp = screenSize.width;
        screenSize.width = screenSize.height;
        screenSize.height = temp;
    }
    
    if (CGSizeEqualToSize(screenSize, CGSizeMake(320, 480))) {
        return (SafeAreaInsets){20, 0, 0, 0}; // iPhone 4s and earlier
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(320, 568))) {
        return (SafeAreaInsets){20, 0, 0, 0}; // iPhone 5, 5s, SE (1st Gen)
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(375, 667))) {
        return (SafeAreaInsets){20, 0, 0, 0}; // iPhone 6, 6s, 7, 8, SE (2nd & 3rd Gen)
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(414, 736))) {
        return (SafeAreaInsets){20, 0, 0, 0}; // iPhone 6 Plus, 6s Plus, 7 Plus, 8 Plus
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(375, 812))) {
        return (SafeAreaInsets){44, 0, 34, 0}; // iPhone X, XS, 11 Pro, 12 Mini, 13 Mini
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(390, 844))) {
        return (SafeAreaInsets){47, 0, 34, 0}; // iPhone 12, 12 Pro, 13, 13 Pro, 14
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(393, 852))) {
        return (SafeAreaInsets){59, 0, 34, 0}; // iPhone 14 Pro
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(414, 896))) {
        return (SafeAreaInsets){44, 0, 34, 0}; // iPhone XR, XS Max, 11, 11 Pro Max
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(428, 926))) {
        return (SafeAreaInsets){47, 0, 34, 0}; // iPhone 12 Pro Max, 13 Pro Max
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(430, 932))) {
        return (SafeAreaInsets){47, 0, 34, 0}; // iPhone 14 Plus
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(430, 932))) {
        return (SafeAreaInsets){59, 0, 34, 0}; // iPhone 14 Pro Max
    } else if (CGSizeEqualToSize(screenSize, CGSizeMake(432, 934))) {
        return (SafeAreaInsets){60, 0, 36, 0}; // iPhone 16
    }
    
    return (SafeAreaInsets){20, 0, 0, 0}; // Default for unknown devices
}
