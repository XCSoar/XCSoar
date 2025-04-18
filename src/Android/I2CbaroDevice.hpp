// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

class SensorListener;

namespace I2CbaroDevice {

void
Initialise(JNIEnv *env) noexcept;

void
Deinitialise(JNIEnv *env) noexcept;

Java::LocalObject
Create(JNIEnv *env, jobject holder,
       unsigned index,
       unsigned twi_num, unsigned i2c_addr,
       unsigned sample_rate, unsigned flags,
       SensorListener &listener);

} // namespace I2CbaroDevice
