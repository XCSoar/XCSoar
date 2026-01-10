// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.app.Service;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.os.Binder;
import android.util.Log;

/**
 * Foreground service to ensure XCSoar continues running in the background.
 *
 * This service is essential for:
 * 1. Continuous IGC flight logging - IGC files must be written continuously,
 *    even when the screen is off or app is minimized, otherwise they are invalid.
 * 2. Safety warnings - The full app must run in background to warn users of
 *    airspace incursions, terrain warnings, glide slope violations, and other
 *    critical flight safety alerts.
 *
 * The service uses foregroundServiceType="dataSync" to run reliably in the background
 * for continuous IGC logging and safety warnings. A visible notification is required
 * by Android for all foreground services.
 *
 * The notification also provides a convenient way for users to quickly return to
 * the app and see that XCSoar is actively running.
 *
 * This is bad style for general-purpose Android apps, don't imitate
 * unless you have an excuse as good as ours ;-)
 */
public class MyService extends Service {
  private static final String TAG = "XCSoar";

  private static final String NOTIFICATION_CHANNEL_ID = "xcsoar";

  private NotificationManager notificationManager;

  @Override public void onCreate() {
    super.onCreate();

    notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      String name = "XCSoar";

      /* Use IMPORTANCE_DEFAULT to ensure notification is visible.
         Sound can be disabled via channel settings if needed. */
      int importance = NotificationManager.IMPORTANCE_DEFAULT;

      NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID,
                                                            name, importance);
      /* Disable sound for this channel */
      channel.setSound(null, null);
      channel.enableVibration(false);
      notificationManager.createNotificationChannel(channel);
    }
  }

  private static Notification.Builder createNotificationBuilder(Context context) {
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
      ? new Notification.Builder(context, NOTIFICATION_CHANNEL_ID)
      : new Notification.Builder(context);
  }

  private static Notification createNotification(Context context, PendingIntent intent) {
    Notification.Builder builder = createNotificationBuilder(context)
      .setOngoing(true)
      .setContentIntent(intent)
      .setContentTitle("XCSoar")
      .setContentText("XCSoar is running")
      .setSmallIcon(R.drawable.notification_icon);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      /* Make notification visible on lock screen so user knows XCSoar is running
         and can quickly return to the app. Background operation is essential for
         continuous IGC logging and safety warnings. */
      builder.setVisibility(Notification.VISIBILITY_PUBLIC);
    }

    return builder.build();
  }

  @Override public int onStartCommand(Intent intent, int flags, int startId) {
    /* Display a notification while XCSoar runs in background.
       This notification is required by Android for foreground services and provides
       a quick way for users to return to the app. Background operation is essential
       for continuous IGC logging and safety warnings. */
    Intent intent2 = new Intent(this, XCSoar.class);
    int pendingIntentFlags = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
      ? PendingIntent.FLAG_IMMUTABLE
      : 0;
    PendingIntent contentIntent =
      PendingIntent.getActivity(this, 0, intent2, pendingIntentFlags);
    Notification notification = createNotification(this, contentIntent);

    notificationManager.notify(1, notification);

    try {
      startForeground(1, notification);
    } catch (Exception e) {
      Log.e(TAG, "Service.startForeground() failed", e);
    }

    /* We want this service to continue running until it is explicitly
       stopped, so return sticky */
    return START_STICKY;
  }

  @Override public void onDestroy() {
    super.onDestroy();

    try {
      /* Stop foreground service and remove notification */
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        stopForeground(STOP_FOREGROUND_REMOVE);
      } else {
        stopForeground(true);
      }
      notificationManager.cancel(1);
    } catch (Exception e) {
      /* Ignore exceptions during service destruction */
      Log.d(TAG, "MyService: Exception during cleanup", e);
    }
  }

  @Override public IBinder onBind(Intent intent) {
    return null;
  }
}
