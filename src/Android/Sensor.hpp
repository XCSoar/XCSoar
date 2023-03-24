// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/State.hpp"

#include <jni.h>

/**
 * Support for the #AndroidSensor Java class.
 */
namespace AndroidSensor {

void
Initialise(JNIEnv *env) noexcept;

[[gnu::pure]]
PortState
GetState(JNIEnv *env, jobject object) noexcept;

} // namespace AndroidSensor
