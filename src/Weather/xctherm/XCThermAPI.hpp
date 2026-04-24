// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

class XCThermCompositeOverlay;
class OperationEnvironment;

/**
 * XCTherm Weather Forecast API client.
 * Handles JWT authentication, .pbf tile downloads, and overlay management.
 */
class XCThermAPI {
public:
  struct Settings {
    bool enabled = false;
    std::string email;
    std::string password;
    std::string model;         // "icon-ch" or "icon-uk-2km"
    uint32_t wave_height;      // meters MSL
    uint32_t vertical_wind_agl; // meters AGL
    bool use_wave_forecast;    // true=wave, false=vertical wind

    void SetDefaults() noexcept {
      enabled = false;
      email.clear();
      password.clear();
      model = "icon-ch";
      wave_height = 3000;
      vertical_wind_agl = 100;
      use_wave_forecast = true;
    }
  };

  static XCThermAPI &Instance();

  XCThermAPI();
  ~XCThermAPI();

  XCThermAPI(const XCThermAPI &) = delete;
  XCThermAPI &operator=(const XCThermAPI &) = delete;

  void UpdateSettings(const Settings &new_settings);
  const Settings &GetSettings() const { return settings; }

  bool IsReady() const;
  bool IsEnabled() const;

  /**
   * Download wave forecast tiles for an area around the given location.
   *
   * @param latitude Center latitude in degrees
   * @param longitude Center longitude in degrees
   * @param date Forecast date in "YYYY-MM-DD" format
   * @param overlay Composite overlay to fill with parsed tiles
   * @param env Optional progress reporting
   * @param downloaded_bytes Optional: total bytes downloaded
   * @return true if at least some tiles were loaded
   */
  bool FetchForecast(double latitude, double longitude,
                     const std::string &date,
                     XCThermCompositeOverlay &overlay,
                     OperationEnvironment *env = nullptr,
                     uint64_t *downloaded_bytes = nullptr);

private:
  Settings settings;
  std::string jwt_token;
  uint32_t token_expiry = 0;
  bool authenticating = false;

  bool Authenticate();
  bool IsTokenValid() const;

  bool DownloadTile(const std::string &model,
                    const std::string &date_yyyymmdd,
                    const std::string &run_cycle,
                    const std::string &layer,
                    int z, int x, int y,
                    std::vector<uint8_t> &out_data);

  std::string GetCachePath(const std::string &model,
                           const std::string &date_yyyymmdd,
                           const std::string &run_cycle,
                           const std::string &layer) const;

  static int LongitudeToTileX(double longitude, int zoom);
  static int LatitudeToTileY(double latitude, int zoom);
  static std::string FormatDateCompact(const std::string &date);
  static std::string BuildRunCycleUtc();
};
