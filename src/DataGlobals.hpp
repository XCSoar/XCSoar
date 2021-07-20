// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class RaspStore;
class RasterTerrain;
class Skysight;

/**
 * This namespace provides helper functions to access generic global
 * data objects.  Use them when you don't know where else to get them.
 * This is a last resort only, don't use it if you have a better way
 * to do it.
 *
 * This namespace exists to avoid direct access to #MainWindow and
 * others, because that would mean the code is not reusable in other
 * applications, while the functions in this namespace can easily be
 * replaced in another program.
 */
namespace DataGlobals {

/**
 * Unset the current #RasterTerrain instance in all XCSoar subsystems.
 *
 * This must be called while all threads are suspended via
 * SuspendAllThreads().
 */
void
UnsetTerrain() noexcept;

void
SetTerrain(std::unique_ptr<RasterTerrain> _terrain) noexcept;

std::shared_ptr<RaspStore>
GetRasp() noexcept;

void
SetRasp(std::shared_ptr<RaspStore> rasp) noexcept;

/**
 * Determine the home waypoint and startup location.  Call this after
 * loading a new waypoint or terrain file.
 */
void
UpdateHome(bool reset) noexcept;

std::shared_ptr<Skysight> GetSkysight();
void SetSkysight(std::shared_ptr<Skysight> skysight);
};
