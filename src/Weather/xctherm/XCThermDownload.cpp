// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDownloadJob.hpp"
#include "XCThermAPI.hpp"
#include "XCThermGeoJSON.hpp"
#include "LogFile.hpp"
#include "Operation/ProgressListener.hpp"
#include "co/Task.hxx"
#include "lib/curl/Global.hxx"

#include <cstdlib>
#include <thread>

namespace {

class JobProgressListener final : public ProgressListener {
  const std::shared_ptr<XCThermDownloadJob> job;

public:
  explicit JobProgressListener(const std::shared_ptr<XCThermDownloadJob> &j) noexcept
    :job(j) {}

  void SetProgressRange(unsigned range) noexcept override {
    job->bytes_total.store(range);
  }

  void SetProgressPosition(unsigned position) noexcept override {
    job->bytes_now.store(position);
  }
};

} // namespace

Co::Task<void>
RunXCThermDownload(CurlGlobal &curl,
                   const std::shared_ptr<XCThermDownloadJob> &job)
{
  auto &api = XCThermAPI::Instance();
  JobProgressListener progress_listener{job};

  const unsigned total_slots = job->span_hours + 1;
  for (unsigned slot_i = 0; slot_i < total_slots; ++slot_i) {
    if (job->cancel.load())
      break;

    const bool is_past = (slot_i == 0);
    const unsigned slot_base = is_past
      ? (job->current_utc + 23) % 24
      : job->current_utc;
    const unsigned slot_offset = is_past ? 0u : slot_i;

    job->current_offset.store(slot_i + 1);
    job->bytes_now.store(0);
    job->bytes_total.store(0);

    std::string slot_date, slot_run_hour;
    unsigned slot_step = 0;
    if (!api.FindSlotForOffset(job->param, slot_base, slot_offset,
                               slot_date, slot_run_hour, slot_step)) {
      LogFmt("xctherm: no slot for slot#{} (base={}UTC +{}h) — skipping",
             slot_i, slot_base, slot_offset);
      job->any_slot_missing.store(true);
      continue;
    }

    const unsigned run_h = (unsigned)std::atoi(slot_run_hour.c_str());
    const unsigned forecast_utc = (run_h + slot_step) % 24;

    {
      std::lock_guard lock{job->result_mutex};
      if (slot_date > job->latest_run_date ||
          (slot_date == job->latest_run_date &&
           slot_run_hour > job->latest_run_hour)) {
        job->latest_run_date = slot_date;
        job->latest_run_hour = slot_run_hour;
      }
    }

    if (api.IsCachedAtRun(job->param, forecast_utc,
                          slot_date, slot_run_hour)) {
      LogFmt("xctherm: slot#{} ({}UTC) cached at run {}/{}Z — reused",
             slot_i, forecast_utc, slot_date, slot_run_hour);
      job->succeeded_or_cached.fetch_add(1);

      {
        std::lock_guard lock{job->result_mutex};
        if (job->first_forecast.IsEmpty()) {
          const std::string &cached =
            api.GetCachedGeoJSON(job->param, forecast_utc);
          if (!cached.empty()) {
            auto forecast = XCThermGeoJSON::Parse(cached, true);
            if (!forecast.IsEmpty()) {
              forecast.layer_name = job->target_label;
              job->first_forecast = std::move(forecast);
            }
          }
        }
      }
      continue;
    }

    bool slot_ok = false;
    bool slot_abandon = false;
    for (unsigned attempt = 0; !job->cancel.load(); ++attempt) {
      job->retry_attempt.store(attempt);
      job->retry_seconds_left.store(0);
      job->bytes_now.store(0);
      job->bytes_total.store(0);

      std::string geojson;
      int64_t wire_bytes = 0;

      const auto should_continue = [&job]() noexcept {
        return !job->cancel.load();
      };

      bool transient_failure = false;
      try {
        const bool ok = co_await api.CoDownloadGeoJSON(
          curl, job->param, slot_date, slot_run_hour, slot_step,
          geojson, &wire_bytes, &progress_listener, should_continue);

        if (job->cancel.load())
          break;

        if (ok) {
          job->total_wire_bytes.fetch_add((uint64_t)wire_bytes);
          job->succeeded_or_cached.fetch_add(1);
          job->newly_downloaded.fetch_add(1);

          {
            std::lock_guard lock{job->result_mutex};
            if (job->first_forecast.IsEmpty()) {
              auto forecast = XCThermGeoJSON::Parse(geojson, true);
              if (!forecast.IsEmpty()) {
                forecast.layer_name = job->target_label;
                job->first_forecast = std::move(forecast);
              }
            }
          }
          slot_ok = true;
          break;
        }
      } catch (const XCThermAPIError &e) {
        switch (e.kind) {
        case XCThermAPIError::Kind::NETWORK:
        case XCThermAPIError::Kind::SERVER_ERROR:
          transient_failure = true;
          break;
        case XCThermAPIError::Kind::NOT_FOUND:
          LogFmt("xctherm: slot#{}: {}", slot_i, e.what());
          slot_abandon = true;
          break;
        case XCThermAPIError::Kind::AUTH_FAILED:
        case XCThermAPIError::Kind::FORBIDDEN:
        case XCThermAPIError::Kind::OTHER_HTTP:
          LogFmt("xctherm: aborting job — {}", e.what());
          job->error_eptr = std::current_exception();
          job->finished_at = std::chrono::steady_clock::now();
          job->done.store(true);
          co_return;
        }
      } catch (...) {
        job->error_eptr = std::current_exception();
        job->finished_at = std::chrono::steady_clock::now();
        job->done.store(true);
        co_return;
      }

      if (slot_abandon)
        break;

      if (transient_failure) {
        constexpr unsigned RETRY_SECONDS = 5;
        LogFmt("xctherm: slot#{} attempt {} transient failure — retry in {}s",
               slot_i, attempt + 1, RETRY_SECONDS);
        for (unsigned s = RETRY_SECONDS; s > 0 && !job->cancel.load(); --s) {
          job->retry_seconds_left.store(s);
          for (int t = 0; t < 10 && !job->cancel.load(); ++t)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        job->retry_seconds_left.store(0);
      }
    }

    if (!slot_ok && !job->cancel.load())
      job->any_slot_missing.store(true);
  }

  job->finished_at = std::chrono::steady_clock::now();
  job->done.store(true);
}
