// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Product name configuration for easy rebranding of forks.
 * 
 * To rebrand XCSoar to another name, you can either:
 * 1. Override PRODUCT_NAME at build time: make PRODUCT_NAME=MyProduct
 * 2. Modify this file directly for permanent changes
 * 
 * See REBRANDING_GUIDE.md for complete rebranding instructions.
 */

#include <tchar.h>

#ifndef PRODUCT_NAME
#define PRODUCT_NAME "XCSoar"
#endif

/* Lowercase version for file paths and Unix conventions */
#ifndef PRODUCT_NAME_LC
#define PRODUCT_NAME_LC "xcsoar"
#endif

/* ASCII version for help text and logging */
#define PRODUCT_NAME_A PRODUCT_NAME

/* Android package name (reverse domain notation) */
#ifndef ANDROID_PACKAGE
#define ANDROID_PACKAGE "org.xcsoar"
#endif

/* Data directory names by platform:
 * - Windows: %LOCALAPPDATA%\XCSoarData
 * - macOS: ~/XCSoarData (visible, macOS users prefer this)
 * - iOS: ~/Documents/XCSoarData (visible via iTunes)
 * - Linux/Unix: ~/.xcsoar (hidden, follows Unix conventions)
 * - System-wide: /etc/xcsoar (global config on Linux/Unix)
 */
#ifndef PRODUCT_DATA_DIR
#define PRODUCT_DATA_DIR PRODUCT_NAME "Data"
#endif

#ifndef PRODUCT_DATA_DIR_T
#define PRODUCT_DATA_DIR_T _T(PRODUCT_DATA_DIR)
#endif
#define PRODUCT_UNIX_SYSCONF_DIR "/etc/" PRODUCT_NAME_LC
#define PRODUCT_UNIX_HOME_DIR "." PRODUCT_NAME_LC
