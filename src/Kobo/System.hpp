// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

bool
KoboReboot();

bool
KoboPowerOff();

bool
KoboUmountData();

bool
KoboMountData();

bool
KoboExportUSBStorage();

void
KoboUnexportUSBStorage();

[[gnu::pure]]
bool
IsKoboWifiOn();

bool
KoboWifiOn();

bool
KoboWifiOff();

void
KoboExecNickel();

void
KoboRunXCSoar(const char *mode);

void
KoboRunTelnetd();

void
KoboRunFtpd();

bool
KoboCanChangeBacklightBrightness();

int
KoboGetBacklightBrightness();

void
KoboSetBacklightBrightness(int percent);

const char *
KoboGetBacklightColourFile() noexcept;

bool
KoboCanChangeBacklightColour() noexcept;

bool
KoboGetBacklightColour(unsigned int &colour) noexcept;

void
KoboSetBacklightColour(int colour) noexcept;
