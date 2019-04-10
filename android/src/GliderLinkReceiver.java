/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2018 The XCSoar Project
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

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import android.content.Context;
import android.os.Handler;


class GliderLinkReceiver extends BroadcastReceiver {
  private static final String TAG = "XCSoar";
  public static final String ACTION = "link.glider.gliderlink.target_position";

  /**
   * Index of this device in the global list. This value is extracted directly
   * from this object by the C++ wrapper code.
   */
  private final int index;

  private static Handler handler;
  private Context context;

  /**
   * Global initialization of the class.  Must be called from the main
   * event thread, because the Handler object must be bound to that
   * thread.
   */
  public static void Initialize() {
    handler = new Handler();
  }

  public GliderLinkReceiver(final Context context, int index) {
    this.index = index;
    this.context = context;

    handler.post(new Runnable() {
      @Override
      public void run() {
        context.registerReceiver(GliderLinkReceiver.this, new IntentFilter(GliderLinkReceiver.ACTION));
      }
    });
  }

  public void close() {
    handler.post(new Runnable() {
      @Override
      public void run() {
        context.unregisterReceiver(GliderLinkReceiver.this);
      }
    });
  }

  private static native void setGliderLinkInfo(int deviceId, long gid, String callsign,
          double latitude, double longitude, double altitude,
          double gspeed, double vspeed, int bearing);

  @Override
  public void onReceive(Context context, Intent intent) {
    try {
      JSONObject json = new JSONObject(intent.getStringExtra("json"));

      JSONObject pos = json.getJSONObject("position");
      
      /* Sample data:
             {  
                "position":{  
                   "gid":3333,
                   "callsign":"4D",
                   "senderTime":1527263825128,
                   "receivedTime":1527263825128,
                   "latitude":37.56716816,
                   "longitude":-122.02447995000023,
                   "altitude":1701,
                   "bearing":270,
                   "gspeed":30,
                   "vspeed":3,
                   "accuracy":0
                }
             }
      */

      setGliderLinkInfo(index, pos.getLong("gid"), pos.getString("callsign"), pos.getDouble("latitude"),
              pos.getDouble("longitude"), pos.getDouble("altitude"), pos.getDouble("gspeed"), pos.getDouble("vspeed"),
              pos.getInt("bearing"));
    } catch (JSONException e) {
      Log.e(TAG, e.getLocalizedMessage(), e);
    }
  }
}
