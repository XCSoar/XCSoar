// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

class SensorListener;

/**
 * Glue code to use the Java class NativeSensorListener from C++.
 */
namespace NativeSensorListener {

void
Initialise(JNIEnv *env) noexcept;

void
Deinitialise(JNIEnv *env) noexcept;

Java::LocalObject
Create(JNIEnv *env, SensorListener &l) noexcept;

} // namespace NativeDetectDeviceListener
