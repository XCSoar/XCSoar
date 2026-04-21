// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Handle vhf:…#standby / vhf:…#active links (internal scheme).
 * @param station_name optional name from markdown [text](url); nullptr if raw
 * @return true if @p url is a vhf: link (handled or user was notified)
 */
bool
HandleVhfLink(const char *url, const char *station_name);
