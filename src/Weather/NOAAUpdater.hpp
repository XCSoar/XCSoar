// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOAAStore.hpp"

class CurlGlobal;
class ProgressListener;
namespace Co { template<typename T> class Task; }

namespace NOAAUpdater {

Co::Task<bool>
Update(NOAAStore &store, CurlGlobal &curl,
       ProgressListener &progress) noexcept;

Co::Task<bool>
Update(NOAAStore::Item &item, CurlGlobal &curl,
       ProgressListener &progress) noexcept;

}
