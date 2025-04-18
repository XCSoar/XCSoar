// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

struct PCMetSettings {
  struct Credentials {
    StaticString<64> username;
    StaticString<64> password;

    bool IsDefined() const {
      return !username.empty() && !password.empty();
    }

    void SetDefaults() {
      username.clear();
      password.clear();
    }
  };

  /**
   * Credentials for https://www.flugwetter.de/
   */
  Credentials www_credentials;

  /**
   * Credentials for ftp.pcmet.de
   */
  Credentials ftp_credentials;

  void SetDefaults() {
    www_credentials.SetDefaults();
    ftp_credentials.SetDefaults();
  }
};
