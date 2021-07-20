// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Skysight.hpp"

#include "ActionInterface.hpp"
  
Skysight::Skysight() { 
  Init();
}

void Skysight::Init() {
  const auto settings = CommonInterface::GetComputerSettings().weather.skysight;
  region = settings.region.c_str();
  email = settings.email.c_str();
  password = settings.password.c_str();

  api = new SkysightAPI(email, password, region);
}
