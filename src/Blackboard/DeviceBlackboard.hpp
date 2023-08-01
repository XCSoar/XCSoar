// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BaseBlackboard.hpp"
#include "Blackboard/ComputerSettingsBlackboard.hpp"
#include "Device/Simulator.hpp"
#include "Device/Features.hpp"
#include "thread/Mutex.hxx"
#include "time/WrapClock.hpp"

#include <array>

class AtmosphericPressure;
class OperationEnvironment;
class RadioFrequency;

/**
 * Blackboard used by com devices: can write NMEA_INFO, reads DERIVED_INFO.
 * Also provides methods to emulate device updates e.g. from logger replay
 * 
 * The DeviceBlackboard is used as the global ground truth-state
 * since it is accessed quickly with only one mutex
 */
class DeviceBlackboard
  : public BaseBlackboard, public ComputerSettingsBlackboard
{
  friend class MergeThread;

  Simulator simulator;

  /**
   * Data from each physical device.
   */
  std::array<NMEAInfo, NUMDEV> per_device_data;

  /**
   * Merged data from the physical devices.
   */
  NMEAInfo real_data;

  /**
   * Data from simulator.
   */
  NMEAInfo simulator_data;

  /**
   * Data from replay.
   */
  NMEAInfo replay_data;

  /**
   * Clock management for #real_data and #replay_data.
   */
  WrapClock real_clock, replay_clock;

public:
  Mutex mutex;

public:
  DeviceBlackboard() noexcept;

  /**
   * Reads the given derived_info usually provided by the
   * GlideComputerBlackboard and saves it to the own Blackboard
   * @param derived_info Calculated information usually provided
   * by the GlideComputerBlackboard
   */
  void ReadBlackboard(const DerivedInfo &derived_info) noexcept {
    calculated_info = derived_info;
  }

  /**
   * Reads the given settings usually provided by the InterfaceBlackboard
   * and saves it to the own Blackboard
   * @param settings ComputerSettings usually provided by the
   * InterfaceBlackboard
   */
  void ReadComputerSettings(const ComputerSettings &settings) noexcept {
    computer_settings = settings;
  }

protected:
  NMEAInfo &SetBasic() noexcept { return gps_info; }
  MoreData &SetMoreData() noexcept { return gps_info; }

public:
  const NMEAInfo &RealState(unsigned i) const noexcept {
    return per_device_data[i];
  }

  NMEAInfo &SetRealState(unsigned i) noexcept {
    return per_device_data[i];
  }

  /**
   * Return a copy of a device's data after updating its clock via
   * NMEAInfo::UpdateClock().  The method takes care for locking and
   * unlocking the mutex.
   */
  NMEAInfo LockGetDeviceDataUpdateClock(unsigned i) noexcept {
    const std::lock_guard lock{mutex};
    per_device_data[i].UpdateClock();
    return per_device_data[i];
  }

  /**
   * Overwrites a device's data and schedule the MergeThread.  The
   * method takes care for locking and unlocking the mutex.
   */
  void LockSetDeviceDataScheduleMerge(unsigned i, const NMEAInfo &src) noexcept {
    {
      const std::lock_guard lock{mutex};
      per_device_data[i] = src;
    }

    ScheduleMerge();
  }

  NMEAInfo &SetSimulatorState() noexcept { return simulator_data; }
  NMEAInfo &SetReplayState() noexcept { return replay_data; }

public:
  const NMEAInfo &RealState() const noexcept { return real_data; }

  /**
   * Is the specified device a FLARM?
   *
   * This method does not lock the blackboard, it is unprotected.  It
   * is assumed that this method is not important enough to implement
   * proper locking.
   */
  [[gnu::pure]]
  bool IsFLARM(unsigned i) const noexcept {
    return RealState(i).flarm.IsDetected();
  }

  void SetStartupLocation(const GeoPoint &loc, double alt) noexcept;
  void ProcessSimulation() noexcept;
  void StopReplay() noexcept;

  void SetSimulatorLocation(const GeoPoint &location) noexcept;
  void SetTrack(Angle val) noexcept;
  void SetSpeed(double val) noexcept;
  void SetAltitude(double alt) noexcept;

  /**
   * Check the expiry time of the device connection with the wall
   * clock time.  This method locks the blackboard, i.e. it may be
   * called from any thread.
   *
   * @return true if the connection has just expired, false if the
   * connection status has not changed
   */
  void ExpireWallClock() noexcept;

  /**
   * Trigger the MergeThread, which will call Merge().  Call this
   * after a modification.  The caller doesn't need to hold the lock.
   */
  void ScheduleMerge() noexcept;

  /**
   * Copy real_data or simulator_data or replay_data to gps_info.
   * Caller must lock the blackboard.
   */
  void Merge() noexcept;
};
