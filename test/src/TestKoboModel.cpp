// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Kobo/Model.hpp"
#include "TestUtil.hpp"

#include <span>
#include <string_view>

static KoboModel
Parse(std::string_view value) noexcept
{
  return ParseKoboModel(std::span{value.data(), value.size()});
}

int
main()
{
  plan_tests(15);

  ok1(Parse("SN-N365") == KoboModel::CLARA_BW);
  ok1(Parse("SN-P365") == KoboModel::CLARA_BW);
  ok1(Parse("SN-N367") == KoboModel::CLARA_COLOUR);
  ok1(Parse("hwcfg-data-SN-N367-more-data") == KoboModel::CLARA_COLOUR);
  ok1(Parse("SN-N999") == KoboModel::UNKNOWN);
  ok1(Parse("SN-N36") == KoboModel::UNKNOWN);

  ok1(IsKoboMediaTek(KoboModel::CLARA_BW));
  ok1(IsKoboMediaTek(KoboModel::CLARA_COLOUR));
  ok1(!IsKoboMediaTek(KoboModel::CLARA_2E));

  ok1(std::string_view{GetKoboWifiInterface(KoboModel::CLARA_BW)} == "wlan0");
  ok1(std::string_view{GetKoboWifiInterface(KoboModel::CLARA_COLOUR)} == "wlan0");
  ok1(std::string_view{GetKoboWifiInterface(KoboModel::CLARA_2E)} == "mlan0");
  ok1(std::string_view{GetKoboWifiInterface(KoboModel::GLO_HD)} == "eth0");

  ok1(std::string_view{GetKoboOnboardPartition(KoboModel::CLARA_BW)} ==
      "/dev/mmcblk0p12");
  ok1(std::string_view{GetKoboOnboardPartition(KoboModel::CLARA_HD)} ==
      "/dev/mmcblk0p3");

  return exit_status();
}
