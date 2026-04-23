// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlatformWifiBackend.hpp"
#include "WPASupplicantBackend.hpp"

#if defined(KOBO)
#include "Model.hpp"
#endif

#include <memory>

UniqueWifiBackend
CreatePlatformWifiBackend()
{
#if defined(KOBO)
  return std::make_unique<WPASupplicantBackend>(GetKoboWifiInterface());
#else
  return {};
#endif
}
