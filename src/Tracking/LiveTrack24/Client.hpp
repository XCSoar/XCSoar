// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "SessionID.hpp"

#include <string>

class CurlGlobal;
namespace Co { template<typename T> class Task; }

namespace LiveTrack24 {

/**
 * A client for the LiveTracking API v2 on t2.livetrack24.com.  Every
 * method throws with the server's message when a request fails.
 */
class Client final {
  CurlGlobal &curl;
  const AppInfo app;

public:
  Client(CurlGlobal &_curl, const AppInfo &_app) noexcept
    :curl(_curl), app(_app) {}

  /**
   * Logs in and returns the user id.
   *
   * @param username Case-insensitive username
   * @param password Case-insensitive password
   */
  Co::Task<UserID> GetUserID(const char *username, const char *password,
                             std::string_view app_secret);

  /**
   * Sends one fix.  @p info goes with packet 1 only.
   */
  Co::Task<void> SendPosition(const Session &session, unsigned packet_id,
                              const Fix &fix, const TrackInfo *info);

  /** Ends the session after landing. */
  Co::Task<void> EndTracking(const Session &session, unsigned packet_id);

private:
  /**
   * Requests the path and returns the data lines of a successful
   * reply.
   */
  Co::Task<std::string> Request(std::string path);
};

} // namespace LiveTrack24
