// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

/**
 * JNI entry points for short UI sounds: bundled {@code res/raw} samples via
 * {@link SoundPool}, with {@link MediaPlayer} for early playback or external
 * paths.
 */
public final class SoundUtil {
  private static final String TAG = "SoundUtil";

  private SoundUtil() {
  }

  public static void preload(Context context) {
    BundledSoundBank.schedulePreload(context.getApplicationContext());
  }

  /**
   * @param name native token, e.g. {@code IDR_FAIL} — see static map in
   *     {@link BundledSoundBank}
   */
  public static boolean play(Context context, String name) {
    return UiSounds.playBundled(context.getApplicationContext(), name);
  }

  public static boolean playExternal(Context context, String path) {
    return UiSounds.playExternalFile(context.getApplicationContext(), path);
  }

  // --- volume (explicit scaling; sonification routing is OEM-dependent) ---

  private static final class VolumePolicy {
    private VolumePolicy() {
    }

    static float normalized(AudioManager am, int streamType) {
      int max = am.getStreamMaxVolume(streamType);
      if (max <= 0)
        return 0f;
      return (float)am.getStreamVolume(streamType) / (float)max;
    }

    /** Louder of music vs notification stream — user-perceived “audible” mix. */
    static float userAudibleMix(AudioManager am) {
      float music = normalized(am, AudioManager.STREAM_MUSIC);
      float notification =
          normalized(am, AudioManager.STREAM_NOTIFICATION);
      return Math.max(music, notification);
    }
  }

  // --- sonification AudioAttributes + transient focus ---

  private static final class SonificationRouting {
    private SonificationRouting() {
    }

    static AudioAttributes attributes() {
      return new AudioAttributes.Builder()
          .setUsage(AudioAttributes.USAGE_ASSISTANCE_SONIFICATION)
          .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
          .build();
    }

    static void applyTo(MediaPlayer player) {
      player.setAudioAttributes(attributes());
    }

    /**
     * @return runnable that abandons the focus acquired here
     */
    @SuppressWarnings("deprecation")
    static Runnable acquireTransientFocus(AudioManager am) {
      final AudioManager.OnAudioFocusChangeListener listener =
          focusChange -> {
          };

      final AudioFocusRequest[] holder = new AudioFocusRequest[1];
      int result = AudioManager.AUDIOFOCUS_REQUEST_FAILED;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        holder[0] = new AudioFocusRequest.Builder(
            AudioManager.AUDIOFOCUS_GAIN_TRANSIENT)
            .setAudioAttributes(attributes())
            .setOnAudioFocusChangeListener(listener)
            .build();
        result = am.requestAudioFocus(holder[0]);
      } else {
        result = am.requestAudioFocus(listener, AudioManager.STREAM_MUSIC,
            AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
      }

      if (result != AudioManager.AUDIOFOCUS_REQUEST_GRANTED)
        Log.w(TAG, "Transient audio focus not granted (result=" + result
            + ")");

      return () -> {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
            && holder[0] != null)
          am.abandonAudioFocusRequest(holder[0]);
        else
          am.abandonAudioFocus(listener);
      };
    }
  }

  // --- bundled res/raw registry + SoundPool lifecycle ---

  private static final class BundledSoundBank {
    private static final Map<String, Integer> RAW_RES_ID_BY_TOKEN =
        new HashMap<>();

    static {
      RAW_RES_ID_BY_TOKEN.put("IDR_FAIL", R.raw.fail);
      RAW_RES_ID_BY_TOKEN.put("IDR_INSERT", R.raw.insert);
      RAW_RES_ID_BY_TOKEN.put("IDR_REMOVE", R.raw.remove);
      RAW_RES_ID_BY_TOKEN.put("IDR_WAV_BEEPBWEEP", R.raw.beep_bweep);
      RAW_RES_ID_BY_TOKEN.put("IDR_WAV_CLEAR", R.raw.beep_clear);
      RAW_RES_ID_BY_TOKEN.put("IDR_WAV_DRIP", R.raw.beep_drip);
    }

    private static final Map<String, Integer> POOL_SAMPLE_ID_BY_TOKEN =
        new HashMap<>();

    private static final Handler mainHandler =
        new Handler(Looper.getMainLooper());

    private static SoundPool pool;
    private static volatile boolean poolReady;
    private static volatile boolean preloadScheduled;

    /**
     * Run delayed abandon early so the next {@code requestAudioFocus} does not
     * stack on top of a pending delayed abandon.
     */
    private static Runnable pendingDelayedFocusRelease;

    private BundledSoundBank() {
    }

    static void schedulePreload(Context appContext) {
      mainHandler.post(() -> startPreloadIfNeeded(appContext));
    }

    private static synchronized void startPreloadIfNeeded(Context app) {
      if (poolReady || preloadScheduled)
        return;
      preloadScheduled = true;

      final SoundPool sp = new SoundPool.Builder()
          .setMaxStreams(8)
          .setAudioAttributes(SonificationRouting.attributes())
          .build();

      final int expectedLoads = RAW_RES_ID_BY_TOKEN.size();
      final AtomicInteger completedLoads = new AtomicInteger(0);

      sp.setOnLoadCompleteListener((soundPool, sampleId, status) -> {
        if (status != 0)
          Log.e(TAG, "SoundPool load failed status=" + status
              + " sampleId=" + sampleId);
        if (completedLoads.incrementAndGet() == expectedLoads) {
          pool = soundPool;
          poolReady = true;
        }
      });

      for (Map.Entry<String, Integer> e : RAW_RES_ID_BY_TOKEN.entrySet()) {
        int poolId = sp.load(app, e.getValue().intValue(), 1);
        POOL_SAMPLE_ID_BY_TOKEN.put(e.getKey(), Integer.valueOf(poolId));
      }
    }

    static Integer rawResourceId(String token) {
      return RAW_RES_ID_BY_TOKEN.get(token);
    }

    static boolean isPoolReady() {
      return poolReady && pool != null;
    }

    static void flushPendingFocusRelease() {
      if (pendingDelayedFocusRelease == null)
        return;
      final Runnable pending = pendingDelayedFocusRelease;
      mainHandler.removeCallbacks(pending);
      pendingDelayedFocusRelease = null;
      pending.run();
    }

    static boolean tryPlayPooled(Context app, String token) {
      if (!isPoolReady())
        return false;
      Integer sampleId = POOL_SAMPLE_ID_BY_TOKEN.get(token);
      if (sampleId == null)
        return false;

      flushPendingFocusRelease();

      AudioManager am =
          (AudioManager)app.getSystemService(Context.AUDIO_SERVICE);
      float gain = VolumePolicy.userAudibleMix(am);
      if (gain <= 0f)
        return false;

      Runnable releaseFocus = SonificationRouting.acquireTransientFocus(am);

      int streamId =
          pool.play(sampleId.intValue(), gain, gain, 1, 0, 1f);

      if (streamId == 0) {
        Log.w(TAG, "SoundPool.play returned 0 (token=\"" + token + "\")");
        releaseFocus.run();
        return false;
      }

      pendingDelayedFocusRelease = () -> {
        releaseFocus.run();
        pendingDelayedFocusRelease = null;
      };
      mainHandler.postDelayed(pendingDelayedFocusRelease, 800);
      return true;
    }
  }

  // --- one-shot MediaPlayer (fallback + external files) ---

  @FunctionalInterface
  private interface MediaSourceBinder {
    void attachTo(MediaPlayer player) throws IOException;
  }

  private static final class OneShotMediaPlayer {
    private OneShotMediaPlayer() {
    }

    static MediaPlayer createPrepared(MediaSourceBinder source)
        throws IOException {
      MediaPlayer player = new MediaPlayer();
      try {
        SonificationRouting.applyTo(player);
        source.attachTo(player);
        player.prepare();
        return player;
      } catch (IOException e) {
        player.release();
        throw e;
      } catch (RuntimeException e) {
        /* IllegalStateException, IllegalArgumentException, SecurityException,
         * Resources.NotFoundException, etc. — always release before propagate */
        player.release();
        throw e;
      }
    }

    /**
     * Starts playback; {@code player} must already be {@link MediaPlayer#prepare}d.
     */
    static boolean start(Context app, MediaPlayer player) {
      AudioManager am =
          (AudioManager)app.getSystemService(Context.AUDIO_SERVICE);
      float gain = VolumePolicy.userAudibleMix(am);
      if (gain <= 0f) {
        player.release();
        return false;
      }
      player.setVolume(gain, gain);

      Runnable releaseFocus = SonificationRouting.acquireTransientFocus(am);

      player.setOnErrorListener((p, what, extra) -> {
        Log.e(TAG, "MediaPlayer error what=" + what + " extra=" + extra);
        releaseFocus.run();
        p.release();
        return true;
      });

      player.setOnCompletionListener(p -> {
        releaseFocus.run();
        p.release();
      });

      try {
        player.start();
      } catch (IllegalStateException e) {
        Log.e(TAG, "MediaPlayer.start failed", e);
        releaseFocus.run();
        player.release();
        return false;
      }
      return true;
    }
  }

  // --- orchestration ---

  private static final class UiSounds {
    private UiSounds() {
    }

    static boolean playBundled(Context app, String token) {
      Integer rawId = BundledSoundBank.rawResourceId(token);
      if (rawId == null)
        return false;

      if (BundledSoundBank.tryPlayPooled(app, token))
        return true;

      try {
        BundledSoundBank.flushPendingFocusRelease();
        final int resId = rawId.intValue();
        MediaPlayer prepared = OneShotMediaPlayer.createPrepared(player -> {
          try (AssetFileDescriptor afd =
                   app.getResources().openRawResourceFd(resId)) {
            player.setDataSource(afd.getFileDescriptor(),
                afd.getStartOffset(), afd.getLength());
          }
        });
        return OneShotMediaPlayer.start(app, prepared);
      } catch (Resources.NotFoundException e) {
        Log.e(TAG, "Missing res/raw sound token=\"" + token + "\" (id=0x"
            + Integer.toHexString(rawId) + ")", e);
        return false;
      } catch (IOException e) {
        Log.e(TAG, "Failed to open/play bundled token=\"" + token + "\"", e);
        return false;
      } catch (RuntimeException e) {
        Log.e(TAG, "Bundled sound failed token=\"" + token + "\"", e);
        return false;
      }
    }

    static boolean playExternalFile(Context app, String path) {
      if (path == null || path.isEmpty()) {
        Log.w(TAG, "playExternalFile: null or empty path");
        return false;
      }

      File file = new File(path);
      if (!file.exists())
        return false;

      String absolutePath = file.getAbsolutePath();
      try {
        BundledSoundBank.flushPendingFocusRelease();
        MediaPlayer prepared =
            OneShotMediaPlayer.createPrepared(
                player -> player.setDataSource(absolutePath));
        return OneShotMediaPlayer.start(app, prepared);
      } catch (IOException e) {
        Log.e(TAG, "Failed to play external \"" + path + "\"", e);
        return false;
      } catch (RuntimeException e) {
        Log.e(TAG, "MediaPlayer failed for external \"" + path + "\"", e);
        return false;
      }
    }
  }
}
