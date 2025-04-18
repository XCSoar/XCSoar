// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractReplay.hpp"
#include "IGC/IGCExtensions.hpp"

#include <memory>

class NLineReader;
struct IGCFix;

class IgcReplay: public AbstractReplay
{
  std::unique_ptr<NLineReader> reader;

  IGCExtensions extensions;

public:
  IgcReplay(std::unique_ptr<NLineReader> &&_reader);
  ~IgcReplay() override;

  bool Update(NMEAInfo &data) override;

private:
  /**
   * Parse a line.
   *
   * @return true if a new fix was found
   */
  bool ScanBuffer(const char *buffer, IGCFix &fix, NMEAInfo &basic);

  /**
   * Read from the IGC file until a new fix was found.
   *
   * @return false on end-of-file
   */
  bool ReadPoint(IGCFix &fix, NMEAInfo &basic);
};
