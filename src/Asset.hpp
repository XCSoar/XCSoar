// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID
#include "Android/Product.hpp"
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

/**
 * Returns whether this is a debug build.
 */
constexpr
static inline bool
IsDebug() noexcept
{
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}

/**
 * Returns whether the application is running on Android
 */
constexpr
static inline bool
IsAndroid() noexcept
{
#if defined(ANDROID)
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on an Apple device
 */
constexpr
static inline bool
IsApple() noexcept
{
#if defined(__APPLE__)
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on a macOS device
 */
constexpr
static inline bool
IsMacOSX() noexcept
{
#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on an iOS device
 */
constexpr
static inline bool
IsIOS() noexcept
{
#if defined(__APPLE__) && TARGET_OS_IPHONE
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on a Kobo e-book reader.
 */
constexpr
static inline bool
IsKobo() noexcept
{
#ifdef KOBO
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on an embedded platform.
 * @return True if host hardware is an embedded platform, False otherwise
 */
constexpr
static inline bool
IsEmbedded() noexcept
{
  return IsAndroid() || IsKobo() || IsIOS();
}

/**
 * Returns whether the application is compiled with IOIOLib
 */
constexpr
static inline bool
HasIOIOLib() noexcept
{
#ifdef ANDROID
  return true;
#else
  return false;
#endif
}

/**
 * Does this device have a pointer device? (mouse or touch screen)
 * @return True if a touch screen or mouse is assumed for the hardware
 * that XCSoar is running on, False if the hardware has only buttons
 */
#if (defined(USE_CONSOLE) && !defined(KOBO)) || defined(USE_WAYLAND)
[[gnu::pure]]
bool
HasPointer() noexcept;
#else
constexpr
static inline bool
HasPointer() noexcept
{
  return true;
}

#endif

/**
 * Does this device have a touch screen?  This is useful to know for
 * sizing controls, as a touch screen may require bigger areas.
 */
#if defined(USE_LIBINPUT) || defined(USE_WAYLAND)
[[gnu::pure]]
bool
HasTouchScreen() noexcept;
#else
constexpr
static inline bool
HasTouchScreen() noexcept
{
#if defined(RASPBERRY_PI)
  return true;
#else
  return IsAndroid() || IsKobo() || IsIOS();
#endif
}
#endif

/**
 * Does this device have a keyboard device?
 * @return True if a keyboard is assumed for the hardware
 * that XCSoar is running on, False if the hardware has no keyboard
 */
#if defined(USE_LIBINPUT) || defined(USE_WAYLAND)
[[gnu::pure]]
bool
HasKeyboard() noexcept;
#else
constexpr
static inline bool
HasKeyboard() noexcept
{
  return !IsEmbedded();
}
#endif

/**
 * Does this device have a cursor keys?  These may be used to navigate
 * in modal dialogs.  Without cursor keys, focused controls do not
 * need to be highlighted.
 */
#ifndef ANDROID
constexpr
#endif
static inline bool
HasCursorKeys() noexcept
{
  /* we assume that all Windows (CE) devices have cursor keys; some do
     not, but that's hard to detect */

#ifdef ANDROID
  return has_cursor_keys;
#else
  return !IsKobo() && !IsIOS();
#endif
}

/**
 * Does this device have a display with colors?
 */
static constexpr bool
HasColors() noexcept
{
#if defined(GREYSCALE)
  return false;
#else
  return !IsKobo();
#endif
}

/**
 * Is dithering black&white used on the display?
 */
static constexpr bool
IsDithered() noexcept
{
#ifdef DITHER
  return true;
#else
  return false;
#endif
}

/**
 * Does this device have an electronic paper screen, such as E-Ink?
 * Such screens need some special cases, because they are very slow
 * and show ghosting.  Animations shall be disabled when this function
 * returns true.
 */
static constexpr bool
HasEPaper() noexcept
{
  return IsKobo();
}
