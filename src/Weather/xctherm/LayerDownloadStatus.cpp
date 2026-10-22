// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LayerDownloadStatus.hpp"

#ifdef HAVE_HTTP

#include "XCThermAPI.hpp"
#include "XCThermCatalog.hpp"
#include "XCThermDownloadJob.hpp"
#include "XCThermForecastTime.hpp"
#include "XCThermMapOverlay.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "lib/fmt/ToBuffer.hxx"

#include <ctime>

namespace XCTherm {

static LayerDownloadInfo download_info_ch[
  GetRegion(Region::CH).layer_count];
static LayerDownloadInfo download_info_uk[
  GetRegion(Region::UK).layer_count];

LayerDownloadInfo *
GetLayerDownloadInfo(unsigned model) noexcept
{
  if (ToRegion(model) == Region::UK)
    return download_info_uk;
  return download_info_ch;
}

void
MarkLayerPending(unsigned model, unsigned layer_index,
                 unsigned span_hours) noexcept
{
  auto *info = GetLayerDownloadInfo(model);
  auto &row = info[layer_index];
  row.status = LayerDownloadInfo::PENDING;
  row.pending_total = span_hours + 1;
  row.pending_index = 0;
  row.pending_bytes_now = 0;
  row.pending_bytes_total = 0;
  row.retry_attempt = 0;
  row.retry_seconds_left = 0;
}

void
CopyJobProgress(LayerDownloadInfo &row,
                const XCThermDownloadJob &job) noexcept
{
  row.pending_index = job.current_offset.load();
  row.pending_total = job.span_hours + 1;
  row.pending_bytes_now = job.bytes_now.load();
  row.pending_bytes_total = job.bytes_total.load();
  row.retry_attempt = job.retry_attempt.load();
  row.retry_seconds_left = job.retry_seconds_left.load();
}

static void
FormatIssuedUtc(std::string &out, const std::string &date,
                const std::string &hour) noexcept
{
  if (date.size() == 8 && hour.size() == 2)
    out = std::string(FmtBuffer<32>("{}-{}-{} {} UTC",
                                    date.substr(0, 4),
                                    date.substr(4, 2),
                                    date.substr(6, 2),
                                    hour).c_str());
  else
    out = "?";
}

void
RehydrateLayerRowsFromCache(unsigned model) noexcept
{
  const auto &region = GetRegion(model);
  auto *info = GetLayerDownloadInfo(model);
  auto &api = XCThermAPI::Instance();

  api.EnableDiskCache();

  for (unsigned i = 0; i < region.layer_count; ++i) {
    if (info[i].status != LayerDownloadInfo::NONE)
      continue;

    const auto summary =
      api.GetCachedLayerSummary(region.layers[i].api_parameter);
    if (summary.hours.empty())
      continue;

    LayerDownloadInfo &row = info[i];
    row.status = LayerDownloadInfo::DONE;
    row.span_hours = (unsigned)summary.hours.size();
    row.future_hours = summary.future_hours;
    row.new_downloads = 0;
    row.wire_mb = 0.0;
    row.speed_mbs = 0.0;
    row.pending_index = 0;
    row.pending_total = 0;
    row.pending_bytes_now = 0;
    row.pending_bytes_total = 0;
    row.retry_attempt = 0;
    row.retry_seconds_left = 0;

    FormatIssuedUtc(row.issued_utc, summary.latest_run_date,
                    summary.latest_run_hour);

    if (summary.latest_downloaded_at > 0) {
      const std::time_t t = (std::time_t)summary.latest_downloaded_at;
      std::tm *lt = std::localtime(&t);
      char tbuf[16];
      if (lt && std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", lt) > 0)
        row.download_time = tbuf;
    }
  }
}

StaticString<200>
FormatLayerStatus(unsigned model, unsigned layer_index,
                  unsigned span_hours_setting) noexcept
{
  StaticString<200> text;
  const auto &region = GetRegion(model);
  if (layer_index >= region.layer_count) {
    text = _("None");
    return text;
  }

  const auto *info = GetLayerDownloadInfo(model);
  switch (info[layer_index].status) {
  case LayerDownloadInfo::PENDING: {
    const auto &p = info[layer_index];
    if (p.retry_seconds_left > 0)
      text.Format(_("Slot %u: reconnect in %us (try #%u)"),
                  p.pending_index,
                  p.retry_seconds_left,
                  p.retry_attempt + 1);
    else if (p.pending_bytes_total > 0) {
      const double now_mb = (double)p.pending_bytes_now / (1024.0 * 1024.0);
      const double tot_mb = (double)p.pending_bytes_total / (1024.0 * 1024.0);
      text.Format(_("Slot %u/%u: %.2f / %.2f MB"),
                  p.pending_index, p.pending_total, now_mb, tot_mb);
    } else if (p.pending_bytes_now > 0) {
      const double now_mb = (double)p.pending_bytes_now / (1024.0 * 1024.0);
      text.Format(_("Slot %u/%u: %.2f MB"),
                  p.pending_index, p.pending_total, now_mb);
    } else if (p.pending_total > 0)
      text.Format(_("Slot %u/%u: connecting..."),
                  p.pending_index, p.pending_total);
    else
      text = _("Connecting...");
    break;
  }
  case LayerDownloadInfo::DONE: {
    const unsigned future = info[layer_index].future_hours;
    if (info[layer_index].new_downloads == 0)
      text.Format(_("%u/%uh | Issued %s | %s"),
                  future, span_hours_setting,
                  info[layer_index].issued_utc.c_str(),
                  info[layer_index].download_time.c_str());
    else
      text.Format(_("%u/%uh (%u new) | Issued %s | %.2f MB wire "
                    "%.1f MB/s | %s"),
                  future, span_hours_setting,
                  info[layer_index].new_downloads,
                  info[layer_index].issued_utc.c_str(),
                  info[layer_index].wire_mb,
                  info[layer_index].speed_mbs,
                  info[layer_index].download_time.c_str());
    break;
  }
  case LayerDownloadInfo::FAILED:
    text = _("Download failed");
    break;
  case LayerDownloadInfo::CANCELED:
    text = C_("Status", "Cancelled");
    break;
  default:
    text = _("Not downloaded");
    break;
  }

  return text;
}

FinishDownloadResult
ApplyFinishedDownload(const std::shared_ptr<XCThermDownloadJob> &job) noexcept
{
  auto *info = GetLayerDownloadInfo(job->model);
  auto &row = info[job->target_index];

  const unsigned ok = job->succeeded_or_cached.load();
  const unsigned nu = job->newly_downloaded.load();
  const bool canceled = job->cancel.load();

  if (ok == 0) {
    row.status = canceled
      ? LayerDownloadInfo::CANCELED
      : LayerDownloadInfo::FAILED;
    row.pending_index = 0;
    row.pending_total = 0;
    row.pending_bytes_now = 0;
    row.pending_bytes_total = 0;
    row.retry_attempt = 0;
    row.retry_seconds_left = 0;
    return canceled ? FinishDownloadResult::CANCELED
                    : FinishDownloadResult::FAILED;
  }

  ApplyJobPreviewToMap(job);

  const double span_secs = std::chrono::duration<double>(
    std::chrono::steady_clock::now() - job->started_at).count();
  const double wire_mb =
    (double)job->total_wire_bytes.load() / (1024.0 * 1024.0);

  row.status = LayerDownloadInfo::DONE;
  row.wire_mb = wire_mb;
  row.speed_mbs = span_secs > 0 ? wire_mb / span_secs : 0.0;
  row.span_hours = ok;
  row.new_downloads = nu;
  row.future_hours = XCThermAPI::Instance()
    .GetCachedLayerSummary(job->param).future_hours;
  row.pending_index = 0;
  row.pending_total = 0;
  row.pending_bytes_now = 0;
  row.pending_bytes_total = 0;
  row.retry_attempt = 0;
  row.retry_seconds_left = 0;

  FormatIssuedUtc(row.issued_utc, job->latest_run_date, job->latest_run_hour);

  std::time_t now = std::time(nullptr);
  std::tm *lt = std::localtime(&now);
  char tbuf[16];
  if (lt != nullptr && std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", lt) > 0)
    row.download_time = tbuf;

  if (nu > 0) {
    const unsigned current_utc = GetUtcTimeParts().hour;
    const unsigned dropped =
      XCThermAPI::Instance().PruneStaleRuns(job->param, current_utc);
    if (dropped > 0)
      LogFmt("xctherm: stale-run sweep dropped {} entries for {}",
             dropped, job->param);
  }

  return job->any_slot_missing.load()
    ? FinishDownloadResult::PARTIAL
    : FinishDownloadResult::OK;
}

} // namespace XCTherm

#endif
