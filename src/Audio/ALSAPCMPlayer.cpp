/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ALSAPCMPlayer.hpp"

#include "ALSAEnv.hpp"
#include "PCMDataSource.hpp"
#include "Util/Macros.hpp"
#include "LogFile.hpp"

#include "IO/Async/AsioUtil.hpp"

#include <alsa/asoundlib.h>


static void alsa_error_handler_stub(const char *, int, const char *,
                                    int, const char *, ...) {}


ALSAPCMPlayer::ALSAPCMPlayer(boost::asio::io_service &_io_service) :
  io_service(_io_service)
{
  snd_lib_error_set_handler(alsa_error_handler_stub);
}

ALSAPCMPlayer::~ALSAPCMPlayer()
{
  Stop();
}

bool
ALSAPCMPlayer::TryRecoverFromError(snd_pcm_t &alsa_handle, int error)
{
  assert(error < 0);

  if (-EPIPE == error)
    LogFormat("ALSA PCM buffer underrun");
  else if ((-EINTR == error) || (-ESTRPIPE == error))
    LogFormat("ALSA PCM error: %s - trying to recover",
              snd_strerror(error));
  else {
    // snd_pcm_recover() can only handle EPIPE, EINTR and ESTRPIPE
    LogFormat("Unrecoverable ALSA PCM error: %s",
              snd_strerror(error));
    return false;
  }

  int recover_error = snd_pcm_recover(&alsa_handle, error, 1);
  if (0 == recover_error) {
    LogFormat("ALSA PCM successfully recovered");
    return true;
  } else {
    LogFormat("snd_pcm_recover(0x%p, %d, 1) failed: %d - %s",
              &alsa_handle,
              error,
              recover_error,
              snd_strerror(recover_error));
    return false;
  }
}

bool
ALSAPCMPlayer::WriteFrames(snd_pcm_t &alsa_handle, int16_t *buffer,
                           size_t n, bool try_recover_on_error)
{
  assert(n > 0);
  assert(nullptr != buffer);

  snd_pcm_sframes_t write_ret =
      snd_pcm_writei(&alsa_handle, buffer, static_cast<snd_pcm_uframes_t>(n));
  if (write_ret < static_cast<snd_pcm_sframes_t>(n)) {
    if (write_ret < 0) {
      if (try_recover_on_error) {
        return TryRecoverFromError(alsa_handle, static_cast<int>(write_ret));
      } else {
        LogFormat("snd_pcm_writei(0x%p, 0x%p, %u) failed: %d - %s",
                  &alsa_handle,
                  buffer,
                  static_cast<unsigned>(n),
                  static_cast<int>(write_ret),
                  snd_strerror(static_cast<int>(write_ret)));
        return false;
      }
    } else {
      // Never observed this case. Should not happen? Cannot happen?
      LogFormat("Only %u of %u ALSA PCM frames written",
                static_cast<unsigned>(write_ret),
                static_cast<unsigned>(n));
    }
    return false;
  }

  assert(write_ret == static_cast<snd_pcm_sframes_t>(n));

  return true;
}

void
ALSAPCMPlayer::StartEventHandling()
{
  if (!poll_descs_registered) {
    for (auto &fd : read_poll_descs) {
      fd.async_read_some(boost::asio::null_buffers(),
                          std::bind(&ALSAPCMPlayer::OnReadEvent,
                                    this,
                                    std::ref(fd),
                                    std::placeholders::_1));
    }

    for (auto &fd : write_poll_descs) {
      fd.async_write_some(boost::asio::null_buffers(),
                          std::bind(&ALSAPCMPlayer::OnWriteEvent,
                                    this,
                                    std::ref(fd),
                                    std::placeholders::_1));
    }

    poll_descs_registered = true;
  }
}

void
ALSAPCMPlayer::StopEventHandling()
{
  if (poll_descs_registered) {
    for (auto &sd : read_poll_descs) {
      sd.cancel();
    }

    for (auto &sd : write_poll_descs) {
      sd.cancel();
    }

    poll_descs_registered = false;
  }
}

bool
ALSAPCMPlayer::OnEvent()
{
  snd_pcm_sframes_t n_available = snd_pcm_avail_update(alsa_handle.get());
  if (n_available < 0) {
    if (!TryRecoverFromError(static_cast<int>(n_available)))
      return false;

    n_available = static_cast<snd_pcm_sframes_t>(buffer_size / channels);
  }

  if (n_available < 0)
    return false;
  else if (0 == n_available)
    return true;

  size_t n_read = FillPCMBuffer(buffer.get(),
                                static_cast<size_t>(n_available));
  if (!WriteFrames(static_cast<size_t>(n_available)))
    return false;

  return (n_read == static_cast<size_t>(n_available));
}

void
ALSAPCMPlayer::OnReadEvent(boost::asio::posix::stream_descriptor &fd,
                       const boost::system::error_code &ec) {
  if (ec == boost::asio::error::operation_aborted)
    return;

  if (OnEvent())
    fd.async_read_some(boost::asio::null_buffers(),
                       std::bind(&ALSAPCMPlayer::OnReadEvent,
                                 this,
                                 std::ref(fd),
                                 std::placeholders::_1));
  else
    StopEventHandling();
}

void
ALSAPCMPlayer::OnWriteEvent(boost::asio::posix::stream_descriptor &fd,
                        const boost::system::error_code &ec) {
  if (ec == boost::asio::error::operation_aborted)
    return;

  if (OnEvent())
    fd.async_write_some(boost::asio::null_buffers(),
                        std::bind(&ALSAPCMPlayer::OnWriteEvent,
                                  this,
                                  std::ref(fd),
                                  std::placeholders::_1));
  else
    StopEventHandling();
}

bool
ALSAPCMPlayer::SetParameters(snd_pcm_t &alsa_handle, unsigned sample_rate,
                             bool big_endian_source, unsigned latency,
                             unsigned &channels) {
  /* adoption of alsa-libs's snd_pcm_set_params() function, which is not
   * available on SALSA, with a few detail enhancements. */

  snd_pcm_hw_params_t *hw_params;
  snd_pcm_hw_params_alloca(&hw_params);

  int alsa_error = snd_pcm_hw_params_any(&alsa_handle, hw_params);
  if (alsa_error < 0) {
    LogFormat("snd_pcm_hw_params_any(0x%p, 0x%p) failed: %d - %s",
              &alsa_handle,
              hw_params,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  /* Try to enable resampling, but do not give up if it fails. Probably we are
   * lucky and our ALSA device supports our sample rate natively. */
  snd_pcm_hw_params_set_rate_resample(&alsa_handle, hw_params, 1);

  alsa_error = snd_pcm_hw_params_set_access(&alsa_handle,
                                            hw_params,
                                            SND_PCM_ACCESS_RW_INTERLEAVED);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_hw_params_set_access(0x%p, 0x%p, "
                  "SND_PCM_ACCESS_RW_INTERLEAVED) failed: %d - %s",
              &alsa_handle,
              hw_params,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  alsa_error = snd_pcm_hw_params_set_format(&alsa_handle,
                                            hw_params,
                                            big_endian_source
                                                ? SND_PCM_FORMAT_S16_BE
                                                : SND_PCM_FORMAT_S16_LE);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_hw_params_set_format(0x%p, 0x%p, "
                  "%s) failed: %d - %s",
              &alsa_handle,
              hw_params,
              big_endian_source
                  ? "SND_PCM_FORMAT_S16_BE"
                  : "SND_PCM_FORMAT_S16_LE",
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  assert(1 == channels);
  alsa_error = snd_pcm_hw_params_set_channels_near(&alsa_handle,
                                                   hw_params,
                                                   &channels);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_hw_params_set_channels_near(0x%p, 0x%p, 0x%p) "
                  "failed: %d - %s",
              &alsa_handle,
              hw_params,
              &channels,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  alsa_error = snd_pcm_hw_params_set_rate(&alsa_handle,
                                          hw_params,
                                          sample_rate,
                                          0);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_hw_params_set_rate(0x%p, 0x%p, %u, 0) "
                  "failed: %d - %s",
              &alsa_handle,
              hw_params,
              sample_rate,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  snd_pcm_uframes_t buffer_size, period_size;

  alsa_error = snd_pcm_hw_params_set_buffer_time_near(&alsa_handle,
                                                      hw_params,
                                                      &latency,
                                                      nullptr);
  if (0 != alsa_error) {
    unsigned period_time = latency / 4;
    alsa_error = snd_pcm_hw_params_set_period_time_near(&alsa_handle,
                                                        hw_params,
                                                        &period_time,
                                                        nullptr);
    if (0 != alsa_error) {
      LogFormat("snd_pcm_hw_params_set_period_time_near(0x%p, 0x%p, 0x%p, "
                    "nullptr) failed: %d - %s",
                &alsa_handle,
                hw_params,
                &period_time,
                alsa_error,
                snd_strerror(alsa_error));
      return false;
    }

    alsa_error = snd_pcm_hw_params_get_period_size(hw_params,
                                                   &period_size,
                                                   nullptr);
    if (0 != alsa_error) {
      LogFormat("snd_pcm_hw_params_get_period_size(0x%p, 0x%p, nullptr) "
                    "failed: %d - %s",
                hw_params,
                &period_size,
                alsa_error,
                snd_strerror(alsa_error));
      return false;
    }

    buffer_size = period_size * 4;
    alsa_error = snd_pcm_hw_params_set_buffer_size_near(&alsa_handle,
                                                        hw_params,
                                                        &buffer_size);
    if (0 != alsa_error) {
      LogFormat("snd_pcm_hw_params_set_buffer_size_near(0x%p, 0x%p, 0x%p) "
                    "failed: %d - %s",
                &alsa_handle,
                hw_params,
                &buffer_size,
                alsa_error,
                snd_strerror(alsa_error));
      return false;
    }

    alsa_error = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
    if (0 != alsa_error) {
      LogFormat("snd_pcm_hw_params_get_buffer_size(0x%p, 0x%p) "
                    "failed: %d - %s",
                hw_params,
                &buffer_size,
                alsa_error,
                snd_strerror(alsa_error));
      return false;
    }
  } else {
    alsa_error = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
    if (0 != alsa_error) {
      LogFormat("snd_pcm_hw_params_get_buffer_size(0x%p, 0x%p) "
                    "failed: %d - %s",
                hw_params,
                &buffer_size,
                alsa_error,
                snd_strerror(alsa_error));
      return false;
    }

    alsa_error = snd_pcm_hw_params_get_buffer_time(hw_params,
                                                   &latency,
                                                   nullptr);
    if (0 != alsa_error) {
      LogFormat("snd_pcm_hw_params_get_buffer_time(0x%p, 0x%p, nullptr) "
                    "failed: %d - %s",
                hw_params,
                &latency,
                alsa_error,
                snd_strerror(alsa_error));
      return false;
    }

    unsigned period_time = latency / 4;
    alsa_error = snd_pcm_hw_params_set_period_time_near(&alsa_handle,
                                                        hw_params,
                                                        &period_time,
                                                        nullptr);
    if (0 != alsa_error) {
      LogFormat("snd_pcm_hw_params_set_period_time_near(0x%p, 0x%p, 0x%p, "
                    "nullptr) failed: %d - %s",
                &alsa_handle,
                hw_params,
                &period_time,
                alsa_error,
                snd_strerror(alsa_error));
      return false;
    }

    alsa_error = snd_pcm_hw_params_get_period_size(hw_params,
                                                   &period_size,
                                                   nullptr);
    if (0 != alsa_error) {
      LogFormat("snd_pcm_hw_params_get_period_size(0x%p, 0x%p, nullptr) "
                    "failed: %d - %s",
                hw_params,
                &period_size,
                alsa_error,
                snd_strerror(alsa_error));
      return false;
    }
  }

  alsa_error = snd_pcm_hw_params(&alsa_handle, hw_params);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_hw_params(0x%p, 0x%p) failed: %d - %s",
              &alsa_handle,
              hw_params,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  snd_pcm_sw_params_t *sw_params;
  snd_pcm_sw_params_alloca(&sw_params);

  alsa_error = snd_pcm_sw_params_current(&alsa_handle, sw_params);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_sw_params_current(0x%p, 0x%p) failed: %d - %s",
              &alsa_handle,
              sw_params,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  snd_pcm_uframes_t  start_threshold =
      (buffer_size / period_size) * period_size;
  alsa_error = snd_pcm_sw_params_set_start_threshold(&alsa_handle,
                                                     sw_params,
                                                     start_threshold);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_sw_params_set_start_threshold(0x%p, 0x%p, %u) "
                  "failed: %d - %s",
              &alsa_handle,
              sw_params,
              static_cast<unsigned>(start_threshold),
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  alsa_error = snd_pcm_sw_params_set_avail_min(&alsa_handle,
                                               sw_params,
                                               period_size);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_sw_params_set_avail_min(0x%p, 0x%p, %u) failed: %d - %s",
              &alsa_handle,
              sw_params,
              static_cast<unsigned>(period_size),
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  alsa_error = snd_pcm_sw_params(&alsa_handle, sw_params);
  if (0 != alsa_error) {
    LogFormat("snd_pcm_sw_params(0x%p, 0x%p) failed: %d - %s",
              &alsa_handle,
              sw_params,
              alsa_error,
              snd_strerror(alsa_error));
    return false;
  }

  return true;
}

bool
ALSAPCMPlayer::Start(PCMDataSource &_source)
{
  const unsigned new_sample_rate = _source.GetSampleRate();

  if ((nullptr != source) &&
      alsa_handle &&
      (source->GetSampleRate() == new_sample_rate)) {
    /* just change the source / resume playback */
    bool success = false;
    DispatchWait(io_service, [this, &_source, &success]() {
      bool recovered_from_underrun = false;

      switch (snd_pcm_state(alsa_handle.get())) {
      case SND_PCM_STATE_XRUN:
        if (0 != snd_pcm_prepare(alsa_handle.get()))
          return;
        else {
          recovered_from_underrun = true;
          success = true;
        }
        break;

      case SND_PCM_STATE_RUNNING:
        success = true;
        break;

      default:
        return;
      }

      if (success) {
        source = &_source;

        if (recovered_from_underrun) {
          const size_t n = buffer_size / channels;
          const size_t n_read = FillPCMBuffer(buffer.get(), n);
          if (!WriteFrames(n_read)) {
            success = false;
            return;
          }
        }

        if (success)
          StartEventHandling();
      }
    });
    if (success)
      return true;
  }

  Stop();

  assert(!alsa_handle);

  AlsaHandleUniquePtr new_alsa_handle = MakeAlsaHandleUniquePtr();
  {
    const char *alsa_device = ALSAEnv::GetALSADeviceName();

    snd_pcm_t *raw_alsa_handle;
    int alsa_error = snd_pcm_open(&raw_alsa_handle, alsa_device,
                                  SND_PCM_STREAM_PLAYBACK, 0);
    if (alsa_error < 0) {
      LogFormat("snd_pcm_open(0x%p, \"%s\", SND_PCM_STREAM_PLAYBACK, 0) "
                    "failed: %s",
                &alsa_handle, alsa_device, snd_strerror(alsa_error));
      return false;
    }
    new_alsa_handle = MakeAlsaHandleUniquePtr(raw_alsa_handle);
    assert(new_alsa_handle);
  }

  unsigned latency = ALSAEnv::GetALSALatency();

  channels = 1;
  bool big_endian_source = _source.IsBigEndian();
  if (!SetParameters(*new_alsa_handle, new_sample_rate, big_endian_source,
                     latency, channels))
    return false;

  snd_pcm_sframes_t n_available = snd_pcm_avail(new_alsa_handle.get());
  if (n_available <= 0) {
    LogFormat("snd_pcm_avail(0x%p) failed: %ld - %s",
              new_alsa_handle.get(),
              static_cast<long>(n_available),
              snd_strerror(static_cast<int>(n_available)));
    return false;
  }

  buffer_size = static_cast<snd_pcm_uframes_t>(n_available * channels);
  buffer = std::unique_ptr<int16_t[]>(new int16_t[buffer_size]);

  /* Why does Boost.Asio make it so hard to register a set of of standard
     poll() descriptors (struct pollfd)? */
  int poll_fds_count = snd_pcm_poll_descriptors_count(new_alsa_handle.get());
  if (poll_fds_count < 1) {
    LogFormat("snd_pcm_poll_descriptors_count(0x%p) returned %d",
              new_alsa_handle.get(),
              poll_fds_count);
    return false;
  }

  std::unique_ptr<struct pollfd[]> poll_fds(
      new struct pollfd[poll_fds_count]);
  BOOST_VERIFY(
      poll_fds_count ==
          snd_pcm_poll_descriptors(new_alsa_handle.get(),
                                   poll_fds.get(),
                                   static_cast<unsigned>(poll_fds_count)));

  for (int i = 0; i < poll_fds_count; ++i) {
    if ((poll_fds[i].events & POLLIN) || (poll_fds[i].events & POLLPRI)) {
      read_poll_descs.emplace_back(
          boost::asio::posix::stream_descriptor(io_service, poll_fds[i].fd));
    }
    if (poll_fds[i].events & POLLOUT) {
      write_poll_descs.emplace_back(
          boost::asio::posix::stream_descriptor(io_service, poll_fds[i].fd));
    }
  }

  source = &_source;
  size_t n_read = FillPCMBuffer(buffer.get(), static_cast<size_t>(n_available));

  if (0 == n_read) {
    LogFormat("ALSA PCMPlayer started with data source which "
                  "does not deliver any data");
    return false;
  }

  if (!WriteFrames(*new_alsa_handle, buffer.get(),
                   static_cast<size_t>(n_available), false))
    return false;

  alsa_handle = std::move(new_alsa_handle);

  StartEventHandling();

  return true;
}

void
ALSAPCMPlayer::Stop()
{
  if ((nullptr != alsa_handle) || poll_descs_registered) {
    DispatchWait(io_service, [&]() {
      StopEventHandling();

      if (nullptr != alsa_handle) {
        BOOST_VERIFY(0 == snd_pcm_drop(alsa_handle.get()));
        alsa_handle.reset();
      }
    });
  }

  read_poll_descs.clear();
  write_poll_descs.clear();

  source = nullptr;
}
