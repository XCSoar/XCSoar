// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

bool
KoboReboot();

bool
KoboPowerOff();

/**
 * Ask the KoboMenu parent to power off after XCSoar exits.
 * Returns false when the KoboMenu request channel is unavailable.
 */
bool
KoboRequestPowerOff() noexcept;

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
IsKoboWifiAutoOn();

bool
SetKoboWifiAutoOn(bool enabled);

void
ApplyKoboWifiAutoOn();

bool
KoboWifiOn();

bool
KoboWifiOff();

void
KoboExecNickel();

bool
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
