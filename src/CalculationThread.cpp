// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CalculationThread.hpp"
#include "Computer/GlideComputer.hpp"
#include "Protection.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Hardware/CPU.hpp"

/**
 * Constructor of the CalculationThread class
 * @param _glide_computer The GlideComputer used for the CalculationThread
 */
CalculationThread::CalculationThread(DeviceBlackboard &_device_blackboard,
                                     GlideComputer &_glide_computer) noexcept
  :WorkerThread("CalcThread",
#ifdef KOBO
                /* throttle more on the Kobo, because the EPaper
                   screen cannot be updated that often */
                std::chrono::milliseconds{900},
#else
                std::chrono::milliseconds{450},
#endif
                std::chrono::milliseconds{100},
                std::chrono::milliseconds{50}),
   force(false),
   device_blackboard(_device_blackboard),
   glide_computer(_glide_computer) {
}

void
CalculationThread::SetComputerSettings(const ComputerSettings &new_value) noexcept
{
  const std::lock_guard lock{mutex};
  settings_computer = new_value;
}

void
CalculationThread::SetPolarSettings(const PolarSettings &new_value) noexcept
{
  const std::lock_guard lock{mutex};
  settings_computer.polar = new_value;
}

void
CalculationThread::SetScreenDistanceMeters(double new_value) noexcept
{
  const std::lock_guard lock{mutex};
  screen_distance_meters = new_value;
}

/**
 * Main loop of the CalculationThread
 */
void
CalculationThread::Tick() noexcept
{
#ifdef HAVE_CPU_FREQUENCY
  const ScopeLockCPU cpu;
#endif

  bool gps_updated;

  // update and transfer master info to glide computer
  {
    const std::lock_guard lock{device_blackboard.mutex};

    gps_updated = device_blackboard.Basic().location_available.Modified(glide_computer.Basic().location_available);

    // Copy data from DeviceBlackboard to GlideComputerBlackboard
    glide_computer.ReadBlackboard(device_blackboard.Basic());
  }

  bool force;
  {
    const std::lock_guard lock{mutex};
    // Copy settings from ComputerSettingsBlackboard to GlideComputerBlackboard
    glide_computer.ReadComputerSettings(settings_computer);

    force = this->force;
    if (force) {
      this->force = false;
      gps_updated = true;
    }
  }

  glide_computer.Expire();

  bool do_idle = false;

  if (gps_updated || force)
    // perform idle call if time advanced and slow calculations need to be updated
    do_idle |= glide_computer.ProcessGPS(force);

  // values changed, so copy them back now: ONLY CALCULATED INFO
  // should be changed in DoCalculations, so we only need to write
  // that one back (otherwise we may write over new data)
  {
    const std::lock_guard lock{device_blackboard.mutex};
    device_blackboard.ReadBlackboard(glide_computer.Calculated());
  }

  // if (new GPS data)
  if (gps_updated || force)
    // inform map new data is ready
    TriggerCalculatedUpdate();

  if (do_idle) {
    // do slow calculations last, to minimise latency
    glide_computer.ProcessIdle();
  }
}

void
CalculationThread::ForceTrigger() noexcept
{
  {
    const std::lock_guard lock{mutex};
    force = true;
  }

  WorkerThread::Trigger();
}
