// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/Task.hxx"

class AllocatedPath;
class ProgressListener;
class CurlGlobal;
struct BrokenDateTime;

namespace EDL {

Co::Task<AllocatedPath>
EnsureDownloaded(BrokenDateTime forecast, unsigned isobar,
                 CurlGlobal &curl, ProgressListener &progress);

} // namespace EDL
