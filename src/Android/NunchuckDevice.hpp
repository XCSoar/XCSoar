// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

class SensorListener;

namespace NunchuckDevice {

void
Initialise(JNIEnv *env) noexcept;

void
Deinitialise(JNIEnv *env) noexcept;

Java::LocalObject
Create(JNIEnv *env, jobject holder,
       unsigned twi_num, unsigned sample_rate,
       SensorListener &listener);

} // namespace NunchuckDevice
