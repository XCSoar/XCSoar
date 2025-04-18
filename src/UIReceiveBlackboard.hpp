// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class OperationEnvironment;

/**
 * Receive new data from DeviceBlackboard::Basic() into the
 * InterfaceBlackboard and propagate it.
 */
void
UIReceiveSensorData(OperationEnvironment &env);

/**
 * Receive new data from DeviceBlackboard::Calculated() into the
 * InterfaceBlackboard and propagate it.
 */
void
UIReceiveCalculatedData();
