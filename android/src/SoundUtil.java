// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Map;
import java.util.HashMap;
import java.io.File;
import android.media.MediaPlayer;
import android.content.Context;
import android.content.res.Resources;
import android.net.Uri;
import android.util.Log;

public class SoundUtil {
  private static final String TAG = "SoundUtil";

  private static Map<String, Integer> resources = new HashMap<String, Integer>();

  static {
    resources.put("IDR_FAIL", R.raw.fail);
    resources.put("IDR_INSERT", R.raw.insert);
    resources.put("IDR_REMOVE", R.raw.remove);
    resources.put("IDR_WAV_BEEPBWEEP", R.raw.beep_bweep);
    resources.put("IDR_WAV_CLEAR", R.raw.beep_clear);
    resources.put("IDR_WAV_DRIP", R.raw.beep_drip);
  }

  public static boolean play(Context context, String name) {
    Integer id = resources.get(name);
    if (id == null)
      return false;

    try {
      return run(MediaPlayer.create(context, id));
    } catch (Resources.NotFoundException e) {
      /* Can happen if the installed APK's resources table does not match the
         R class (e.g. broken/partial install, sideload mismatch). */
      Log.e(TAG, "Missing res/raw sound for \"" + name + "\" (id=0x"
          + Integer.toHexString(id) + ")", e);
      return false;
    }
  }

  public static boolean playExternal(Context context, String path) {
    final File file = new File(path);
    if (!file.exists()) {
      return false;
    }

    return run(MediaPlayer.create(context, Uri.fromFile(file)));
  }

  private static boolean run(final MediaPlayer mp) {
    if (mp == null) {
      return false;
    }

    mp.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
      @Override
      public void onCompletion(MediaPlayer mediaPlayer) {
        mp.release();
      }
    });

    mp.start();
    return true;
  }
}
