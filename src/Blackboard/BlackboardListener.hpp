// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct MoreData;
struct DerivedInfo;
struct ComputerSettings;
struct UISettings;

/**
 * This class can be registered to receive GPS updates and other
 * changes.
 */
class BlackboardListener {
public:
  /**
   * New GPS data has been received, but calculations results have not
   * been updated yet.
   */
  virtual void OnGPSUpdate(const MoreData &basic) = 0;

  /**
   * New GPS data has been received and calculations results have been
   * updated.
   */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) = 0;

  /**
   * The user has modified the computer settings.
   */
  virtual void OnComputerSettingsUpdate(const ComputerSettings &settings) = 0;

  /**
   * The user has modified the UI settings.
   */
  virtual void OnUISettingsUpdate(const UISettings &settings) = 0;
};

/**
 * A dummy class that implements all abstract methods as no-ops.
 * Inherit this class and only implement the methods you're interested
 * in.
 */
class NullBlackboardListener : public BlackboardListener {
public:
  void OnGPSUpdate(const MoreData &basic) override;

  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;

  void OnComputerSettingsUpdate(const ComputerSettings &settings) override;

  void OnUISettingsUpdate(const UISettings &settings) override;
};
