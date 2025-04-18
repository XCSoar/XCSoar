// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

class SensorListener;

namespace VoltageDevice {

void
Initialise(JNIEnv *env) noexcept;

void
Deinitialise(JNIEnv *env) noexcept;

Java::LocalObject
Create(JNIEnv *env, jobject holder,
       unsigned sample_rate,
       SensorListener &listener);

} // namespace VoltageDevice
