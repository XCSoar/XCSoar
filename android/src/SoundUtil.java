/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

package org.xcsoar;

import java.util.Map;
import java.util.HashMap;
import java.io.File;
import android.media.MediaPlayer;
import android.content.Context;
import android.net.Uri;

public class SoundUtil {
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

    return run(MediaPlayer.create(context, id));
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
