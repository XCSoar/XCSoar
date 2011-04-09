/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.os.Build;
import android.os.PowerManager;
import android.os.Handler;
import android.os.Message;
import android.os.BatteryManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

public class XCSoar extends Activity {
    private static NativeView nativeView;

    PowerManager.WakeLock wakeLock;

  BroadcastReceiver batteryReceiver = new BroadcastReceiver() {
      @Override public void onReceive(Context context, Intent intent) {
        if (nativeView == null)
          return;

        int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, 0);
        int plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, 0);
        nativeView.setBatteryPercent(level, plugged);
      }
    };

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (!Loader.loaded) {
            TextView tv = new TextView(this);
            tv.setText("Failed to load the native XCSoar libary.\n" +
                       "Report this problem to us, and include the following information:\n" +
                       "ABI=" + Build.CPU_ABI + "\n" +
                       "PRODUCT=" + Build.PRODUCT + "\n" +
                       "FINGERPRINT=" + Build.FINGERPRINT + "\n" +
                       "error=" + Loader.error);
            setContentView(tv);
            return;
        }

        Timer.Initialize();
        InternalGPS.Initialize();

        try {
          BluetoothHelper.Initialize();
        } catch (VerifyError e) {
          // Android < 2.0 doesn't have Bluetooth support
        }

        // fullscreen mode
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN|WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                             WindowManager.LayoutParams.FLAG_FULLSCREEN|WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        TextView tv = new TextView(this);
        tv.setText("Loading XCSoar...");
        setContentView(tv);

      registerReceiver(batteryReceiver,
                       new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
    }

  private void quit() {
    nativeView = null;

    TextView tv = new TextView(XCSoar.this);
    tv.setText("Shutting down XCSoar...");
    setContentView(tv);

    finish();
  }

  Handler quitHandler = new Handler() {
      public void handleMessage(Message msg) {
        quit();
      }
    };

    public void initSDL()
    {
        if (!Loader.loaded)
            return;

        nativeView = new NativeView(this, quitHandler);
        setContentView(nativeView);
        // Receive keyboard events
        nativeView.setFocusableInTouchMode(true);
        nativeView.setFocusable(true);
        nativeView.requestFocus();

        PowerManager pm = (PowerManager)
            getSystemService(Context.POWER_SERVICE);
        wakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK|
                                  PowerManager.ACQUIRE_CAUSES_WAKEUP,
                                  "XCSoar");
        wakeLock.acquire();

        /* add an icon to the notification area while XCSoar runs, to
         * remind the user that we're sucking his battery empty */
        Notification notification = new Notification(R.drawable.icon, null,
                                                     System.currentTimeMillis());
        Intent intent = new Intent(this, XCSoar.class);
        PendingIntent contentIntent =
          PendingIntent.getActivity(this, 0, intent, 0);
        notification.setLatestEventInfo(this, "XCSoar", "XCSoar is running",
                                        contentIntent);
        notification.flags |= Notification.FLAG_ONGOING_EVENT;

        NotificationManager notificationManager = (NotificationManager)
          getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(1, notification);
    }

    @Override protected void onPause() {
        if (nativeView != null)
            nativeView.onPause();
        super.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        if (nativeView != null)
            nativeView.onResume();
        else
            initSDL();
    }

    @Override protected void onDestroy()
    {
      /* remove the notification icon */
      NotificationManager notificationManager = (NotificationManager)
        getSystemService(Context.NOTIFICATION_SERVICE);
      notificationManager.cancel(1);

      if (nativeView != null) {
            nativeView.exitApp();
        nativeView = null;
      }

        if (wakeLock != null) {
            wakeLock.release();
            wakeLock = null;
        }

        super.onDestroy();
        System.exit(0);
    }

    @Override public boolean onKeyDown(int keyCode, final KeyEvent event) {
        // Overrides Back key to use in our app
        if (nativeView != null)
            nativeView.onKeyDown(keyCode, event);
        return true;
    }

    @Override public boolean onKeyUp(int keyCode, final KeyEvent event) {
        if (nativeView != null)
            nativeView.onKeyUp(keyCode, event);
        return true;
    }

    @Override public boolean dispatchTouchEvent(final MotionEvent ev) {
        if (nativeView != null)
            nativeView.onTouchEvent(ev);
        return true;
    }
}
