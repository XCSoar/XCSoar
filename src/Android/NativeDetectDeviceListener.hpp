// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

class DetectDeviceListener;

/**
 * Glue code to use the Java class NativeDetectDeviceListener from C++.
 */
namespace NativeDetectDeviceListener {

void
Initialise(JNIEnv *env) noexcept;

void
Deinitialise(JNIEnv *env) noexcept;

Java::LocalObject
Create(JNIEnv *env, DetectDeviceListener &cb) noexcept;

} // namespace NativeDetectDeviceListener
