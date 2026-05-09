// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "CloudPolicy.hpp"

#include "Tracking/SkyLines/Server.hpp"

#include <chrono>

struct CloudData;
struct GeoPoint;

/**
 * Connects SkyLinesTracking::Server callbacks to #CloudData and outbound
 * datagram responses.
 */
class CloudGlue {
  CloudData &data;
  const CloudPolicy &policy;
  SkyLinesTracking::Server &server;

public:
  constexpr
  CloudGlue(CloudData &_data, const CloudPolicy &_policy,
            SkyLinesTracking::Server &_server) noexcept
    :data(_data), policy(_policy), server(_server) {}

  /** Register or refresh a client and fan out immediate traffic updates. */
  void OnFix(const SkyLinesTracking::Server::Client &client,
             std::chrono::milliseconds time_of_day,
             const GeoPoint &location, int altitude,
             std::chrono::steady_clock::time_point now,
             bool &schedule_expire);

  /** Answer a NEAR traffic request from a known client. */
  void OnTrafficRequest(const SkyLinesTracking::Server::Client &client,
                        bool near,
                        std::chrono::steady_clock::time_point now);

  /** Log wave submissions (no shared storage). */
  void OnWaveSubmit(const SkyLinesTracking::Server::Client &client,
                    std::chrono::milliseconds time_of_day,
                    const GeoPoint &a, const GeoPoint &b,
                    int bottom_altitude, int top_altitude, double lift);

  /** Store a thermal and notify subscribers in range. */
  void OnThermalSubmit(const SkyLinesTracking::Server::Client &client,
                       std::chrono::milliseconds time_of_day,
                       const GeoPoint &bottom_location,
                       int bottom_altitude,
                       const GeoPoint &top_location,
                       int top_altitude,
                       double lift,
                       std::chrono::steady_clock::time_point now);

  /** Send thermals near the requesting client. */
  void OnThermalRequest(const SkyLinesTracking::Server::Client &client,
                        std::chrono::steady_clock::time_point now);
};
