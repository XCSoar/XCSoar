/* Copyright_License {

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

class GliderLinkReceiver
  extends BroadcastReceiver
  implements AndroidSensor
{
  private static final String TAG = "XCSoar";
  public static final String ACTION = "link.glider.gliderlink.target_position";

  private final Context context;

  private final SensorListener listener;
  private final SafeDestruct safeDestruct = new SafeDestruct();

  private final Handler handler;

  private int state = STATE_LIMBO;

  public GliderLinkReceiver(final Context context, SensorListener listener) {
    this.context = context;
    this.listener = listener;

    handler = new Handler(context.getMainLooper());
    handler.post(new Runnable() {
      @Override
      public void run() {
        context.registerReceiver(GliderLinkReceiver.this, new IntentFilter(GliderLinkReceiver.ACTION));
      }
    });
  }

  @Override
  public void close() {
    safeDestruct.beginShutdown();

    handler.post(new Runnable() {
      @Override
      public void run() {
        context.unregisterReceiver(GliderLinkReceiver.this);
      }
    });

    safeDestruct.finishShutdown();
  }

  @Override
  public int getState() {
    return state;
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    if (!safeDestruct.increment())
      return;

    try {
      JSONObject json = new JSONObject(intent.getStringExtra("json"));

      if (state != STATE_READY) {
        state = STATE_READY;
        listener.onSensorStateChanged();
      }

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

      listener.onGliderLinkTraffic(pos.getLong("gid"),
                                   pos.getString("callsign"),
                                   pos.getDouble("latitude"),
                                   pos.getDouble("longitude"),
                                   pos.getDouble("altitude"),
                                   pos.getDouble("gspeed"),
                                   pos.getDouble("vspeed"),
                                   pos.getInt("bearing"));
    } catch (JSONException e) {
      Log.e(TAG, e.getLocalizedMessage(), e);
    } finally {
      safeDestruct.decrement();
    }
  }
}
