// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Map;
import java.util.TreeMap;
import java.util.Queue;
import java.util.LinkedList;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PermissionInfo;
import android.os.Build;
import android.os.Handler;
import android.provider.Settings;
import android.app.NotificationManager;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.widget.TextView;

/**
 * Helper class for managing Android permissions.
 * Handles permission requests, rationale dialogs, and permission state checking.
 * This class encapsulates all permission-related logic, making it easier to
 * maintain and test.
 */
public class PermissionHelper implements PermissionManager {
  private final Activity activity;
  private final Handler mainHandler;
  private final Runnable onLocationPermissionsGranted;

  private final Map<Integer, PermissionHandler> permissionHandlers =
    new TreeMap<Integer, PermissionHandler>();
  private int nextPermissionHandlerId = 0;

  /* Track pending Bluetooth permission requests to avoid duplicate disclosures */
  private final java.util.Set<String> pendingBluetoothPermissions =
    new java.util.HashSet<String>();

  /* Permission request queue to prevent overlapping dialogs */
  private static class PendingPermissionRequest {
    final String permission;
    final PermissionHandler handler;

    PendingPermissionRequest(String permission, PermissionHandler handler) {
      this.permission = permission;
      this.handler = handler;
    }
  }

  private final Queue<PendingPermissionRequest> permissionQueue = new LinkedList<>();
  private boolean isProcessingPermission = false;

  /**
   * Create a PermissionHelper instance.
   * @param activity The Activity that will handle permission requests
   * @param mainHandler Handler for posting UI operations to main thread
   * @param onLocationPermissionsGranted Callback to invoke when location permissions are granted
   */
  public PermissionHelper(Activity activity, Handler mainHandler,
                          Runnable onLocationPermissionsGranted) {
    this.activity = activity;
    this.mainHandler = mainHandler;
    this.onLocationPermissionsGranted = onLocationPermissionsGranted;
  }

  /**
   * Handle the result of a permission request.
   * This should be called from Activity.onRequestPermissionsResult().
   */
  public synchronized void onRequestPermissionsResult(int requestCode, String[] permissions,
                                                       int[] grantResults) {
    PermissionHandler handler = permissionHandlers.remove(requestCode);
    if (handler != null) {
      // grantResults is empty when user cancels
      // For multiple permissions, all must be granted
      boolean granted = grantResults.length > 0;
      for (int i = 0; i < grantResults.length; i++) {
        if (grantResults[i] != PackageManager.PERMISSION_GRANTED) {
          granted = false;
          break;
        }
      }

      // If permission was denied and we can't show rationale, user selected "Don't ask again"
      if (!granted && grantResults.length > 0 && 
          grantResults[0] == PackageManager.PERMISSION_DENIED &&
          permissions.length > 0 &&
          !activity.shouldShowRequestPermissionRationale(permissions[0])) {
        // Show a message directing user to settings
        final String permissionLabel = getPermissionLabel(permissions[0]);
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
              android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(activity);
              builder.setTitle("Permission Required");
              builder.setMessage("XCSoar needs " + permissionLabel + " permission, but it was previously denied. " +
                               "Please grant it in Settings > Apps > XCSoar > Permissions.");
              builder.setPositiveButton("Open Settings", new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    intent.setData(android.net.Uri.parse("package:" + activity.getPackageName()));
                    activity.startActivity(intent);
                  }
                });
              builder.setNegativeButton("Cancel", null);
              builder.show();
            }
          });
      }

      handler.onRequestPermissionsResult(granted);
    }
  }

  private static String getBackgroundLocationRationale() {
    return "XCSoar needs permission to access your location in the background (when the app is closed) for flight logging and score calculation. " +
      "If you choose not to allow background location, calculation results may be incomplete.";
  }

  private static String getPermissionRationale(String permission) {
    if (isForegroundLocationPermission(permission))
      return "XCSoar needs permission to access your GPS location - obviously, because XCSoar's purpose is to help you navigate an aircraft.";
    else if (isBackgroundLocationPermission(permission))
      return getBackgroundLocationRationale();
    else if (isBluetoothPermission(permission))
      return "If you want XCSoar to connect to Bluetooth sensors, it needs your permission.";
    else if (Manifest.permission.POST_NOTIFICATIONS.equals(permission))
      return "XCSoar needs permission to show notifications. A notification is required by Android for background operation, and provides a quick way to return to the app. Background operation is essential for continuous flight logging and safety warnings.";
    else
      return null;
  }

  /**
   * Get a human-friendly label for a permission string.
   * Attempts to use PackageManager to get a localized label,
   * falling back to a predefined map or simplified string.
   */
  private String getPermissionLabel(String permission) {
    PackageManager packageManager = activity.getPackageManager();
    try {
      PermissionInfo permissionInfo = packageManager.getPermissionInfo(permission, 0);
      CharSequence label = permissionInfo.loadLabel(packageManager);
      if (label != null && label.length() > 0)
        return label.toString();
    } catch (PackageManager.NameNotFoundException e) {
      /* Fall through to fallback map */
    }

    /* Fallback map for common permissions */
    if (isForegroundLocationPermission(permission))
      return "Location";
    else if (isBackgroundLocationPermission(permission))
      return "Background Location";
    else if (Manifest.permission.BLUETOOTH_CONNECT.equals(permission))
      return "Bluetooth Connect";
    else if (Manifest.permission.BLUETOOTH_SCAN.equals(permission))
      return "Bluetooth Scan";
    else if (Manifest.permission.WRITE_EXTERNAL_STORAGE.equals(permission))
      return "Storage";
    else if (Manifest.permission.POST_NOTIFICATIONS.equals(permission))
      return "Notifications";
    else {
      /* Simplified fallback: extract last component of permission name */
      int lastDot = permission.lastIndexOf('.');
      if (lastDot >= 0 && lastDot < permission.length() - 1)
        return permission.substring(lastDot + 1).replace('_', ' ');
      return permission;
    }
  }

  /**
   * Check if a permission is a location permission (foreground or background).
   */
  private static boolean isLocationPermission(String permission) {
    return Manifest.permission.ACCESS_FINE_LOCATION.equals(permission) ||
           Manifest.permission.ACCESS_BACKGROUND_LOCATION.equals(permission);
  }

  /**
   * Check if a permission is foreground location permission.
   */
  private static boolean isForegroundLocationPermission(String permission) {
    return Manifest.permission.ACCESS_FINE_LOCATION.equals(permission);
  }

  /**
   * Check if a permission is background location permission.
   */
  private static boolean isBackgroundLocationPermission(String permission) {
    return Manifest.permission.ACCESS_BACKGROUND_LOCATION.equals(permission);
  }

  /**
   * Check if a single permission is granted (without side effects).
   * This is a pure check method that doesn't queue requests or show dialogs.
   */
  private boolean isPermissionGranted(String permission) {
    if (android.os.Build.VERSION.SDK_INT < 23)
      /* we don't need to request permissions on this old Android version */
      return true;

    return activity.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
  }

  /**
   * Check if all required location permissions are granted.
   */
  private boolean areAllLocationPermissionsGranted() {
    // Check foreground location permission (required on all Android versions)
    if (!isPermissionGranted(Manifest.permission.ACCESS_FINE_LOCATION))
      return false;
    
    // On Android 10+ (API 29+), also check background location
    if (Build.VERSION.SDK_INT >= 29) {
      return isPermissionGranted(Manifest.permission.ACCESS_BACKGROUND_LOCATION);
    }
    
    return true; // Android < 10 only needs foreground
  }

  /**
   * Check if a permission is a Bluetooth permission (connect or scan).
   */
  private static boolean isBluetoothPermission(String permission) {
    return Manifest.permission.BLUETOOTH_CONNECT.equals(permission) ||
           Manifest.permission.BLUETOOTH_SCAN.equals(permission);
  }

  /**
   * Check if a permission is notification permission (Android 13+).
   */
  private static boolean isNotificationPermission(String permission) {
    return Manifest.permission.POST_NOTIFICATIONS.equals(permission);
  }

  /**
   * Check if notifications are enabled for this app, and prompt user to enable
   * them if disabled. This should be called after POST_NOTIFICATIONS permission
   * is granted on Android 13+.
   */
  public void checkNotificationEnablement() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      NotificationManager notificationManager = (NotificationManager)activity.getSystemService(Activity.NOTIFICATION_SERVICE);
      if (notificationManager != null && !notificationManager.areNotificationsEnabled()) {
        /* Notifications are disabled at app level. Show dialog directing user to settings. */
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
              android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(activity);
              builder.setTitle("Notifications Disabled");
              builder.setMessage("XCSoar needs notifications enabled to run in the background for continuous flight logging and safety warnings. Please enable notifications in system settings.");
              builder.setPositiveButton("Open Settings", new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                      Intent intent = new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS);
                      intent.putExtra(Settings.EXTRA_APP_PACKAGE, activity.getPackageName());
                      activity.startActivity(intent);
                    } else {
                      /* On Android < 8.0, open general app settings */
                      Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                      intent.setData(android.net.Uri.parse("package:" + activity.getPackageName()));
                      activity.startActivity(intent);
                    }
                  }
                });
              builder.setNegativeButton("Cancel", null);
              builder.show();
            }
          });
      }
    }
  }

  /**
   * Build HTML template for permission rationale dialog.
   * Uses HTML so the privacy policy link is clickable.
   */
  private static String buildPermissionRationaleHtml(String rationale) {
    return "<p>" +
      "XCSoar is free software developed by volunteers just for fun. " +
      "The project is non-profit - you don't pay for XCSoar, and we don't sell your data (or anything else). " +
      "</p>" +
      "<p><big>" +
      rationale +
      "</big></p>" +
      "<p>" +
      "All those accesses are only in your own interest; we don't collect your data and we don't track you (unless you explicitly ask XCSoar to). " +
      "</p>" +
      "<p>" +
      "More details can be found in the <a href=\"https://github.com/XCSoar/XCSoar/blob/master/PRIVACY.md\">Privacy policy</a>. " +
      "</p>";
  }

  /**
   * Check if all location permissions are granted and invoke callback if ready.
   * Called from permission handlers after location permissions are granted.
   */
  private void checkLocationPermissionsCallback() {
    if (areAllLocationPermissionsGranted() && onLocationPermissionsGranted != null) {
      mainHandler.post(onLocationPermissionsGranted);
    }
  }

  /**
   * Request permission after user accepts the rationale dialog.
   * Handles both Bluetooth permissions case, single permission case, and background location follow-up.
   */
  private void requestPermissionAfterDialog(String permission, PermissionHandler handler,
                                             boolean requestBothBluetooth, boolean requestBackgroundAfter,
                                             boolean isBluetooth) {
    if (requestBothBluetooth) {
      final String[] bluetoothPermissions = {
        Manifest.permission.BLUETOOTH_SCAN,
        Manifest.permission.BLUETOOTH_CONNECT
      };
      doRequestPermissions(bluetoothPermissions, new PermissionHandler() {
          @Override
          public void onRequestPermissionsResult(boolean granted) {
            pendingBluetoothPermissions.remove(Manifest.permission.BLUETOOTH_SCAN);
            pendingBluetoothPermissions.remove(Manifest.permission.BLUETOOTH_CONNECT);
            isProcessingPermission = false;
            if (handler != null)
              handler.onRequestPermissionsResult(granted);
            processNextPermission();
          }
        });
    } else {
      if (isBluetooth)
        pendingBluetoothPermissions.add(permission);
      doRequestPermission(permission, new PermissionHandler() {
          @Override
          public void onRequestPermissionsResult(boolean granted) {
            if (isBluetooth)
              pendingBluetoothPermissions.remove(permission);
            if (handler != null)
              handler.onRequestPermissionsResult(granted);

            if (granted && requestBackgroundAfter &&
                android.os.Build.VERSION.SDK_INT >= 29) {
              final String bgPermission = Manifest.permission.ACCESS_BACKGROUND_LOCATION;
              if (!isPermissionGranted(bgPermission)) {
                mainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                      requestPermission(bgPermission, new PermissionHandler() {
                          @Override
                          public void onRequestPermissionsResult(boolean bgGranted) {
                            /* After background location permission is granted,
                               check if all location permissions are granted and invoke callback */
                            if (bgGranted) {
                              checkLocationPermissionsCallback();
                            }
                          }
                        });
                    }
                  });
              } else {
                /* Background permission already granted, check if ready to invoke callback */
                checkLocationPermissionsCallback();
              }
            } else if (granted && isForegroundLocationPermission(permission) &&
                       android.os.Build.VERSION.SDK_INT < 29) {
              /* On Android < 10, only foreground location is needed.
                 Check if ready to invoke callback after foreground location is granted. */
              checkLocationPermissionsCallback();
            }

            /* After POST_NOTIFICATIONS is granted, check if notifications are enabled
               at the app level and prompt user to enable them if disabled. */
            if (granted && isNotificationPermission(permission) &&
                android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
              mainHandler.post(new Runnable() {
                  @Override
                  public void run() {
                    checkNotificationEnablement();
                  }
                });
            }
          }
        });
    }
  }

  private void showRequestPermissionRationale(final String permission,
                                              final String rationale,
                                              final PermissionHandler handler) {
    showRequestPermissionRationale(permission, rationale, handler, false, false);
  }

  private void showRequestPermissionRationale(final String permission,
                                              final String rationale,
                                              final PermissionHandler handler,
                                              final boolean requestBackgroundAfter,
                                              final boolean requestBothBluetooth) {
    final String html = buildPermissionRationaleHtml(rationale);

    final TextView tv  = new TextView(activity);
    tv.setMovementMethod(LinkMovementMethod.getInstance());
    tv.setText(Html.fromHtml(html));

    final boolean[] dialogAccepted = {false};

    final AlertDialog dialog = new AlertDialog.Builder(activity)
      .setTitle("Requesting your permission")
      .setView(tv)
      .setPositiveButton("OK", null)
      .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialog, int which) {
            if (handler != null)
              handler.onRequestPermissionsResult(false);
          }
        })
      .setOnCancelListener(new DialogInterface.OnCancelListener() {
          @Override
          public void onCancel(DialogInterface dialog) {
            if (handler != null)
              handler.onRequestPermissionsResult(false);
          }
        })
      .setOnDismissListener(new DialogInterface.OnDismissListener() {
          @Override
          public void onDismiss(DialogInterface d) {
            if (!dialogAccepted[0])
              return;

            final boolean isBluetooth = isBluetoothPermission(permission);
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                  requestPermissionAfterDialog(permission, handler, requestBothBluetooth,
                                             requestBackgroundAfter, isBluetooth);
                }
              });
          }
        })
      .create();

    /* Set OK button to dismiss dialog - onDismiss will handle permission request */
    dialog.setOnShowListener(new DialogInterface.OnShowListener() {
        @Override
        public void onShow(DialogInterface d) {
          dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
              @Override
              public void onClick(View v) {
                dialogAccepted[0] = true;
                dialog.dismiss();
              }
            });
        }
      });

    dialog.show();
  }

  private boolean showRequestPermissionRationaleIndirect(final String permission,
                                                         final PermissionHandler handler) {
    final String rationale = getPermissionRationale(permission);
    if (rationale == null)
      return false;

    /* For ACCESS_FINE_LOCATION, check if we'll also need background location.
       If so, we'll show a separate disclosure for background after foreground
       is granted (as required by Google Play policy). */
    final boolean requestBackgroundAfter =
      isForegroundLocationPermission(permission) &&
      android.os.Build.VERSION.SDK_INT >= 29 &&
      !isPermissionGranted(Manifest.permission.ACCESS_BACKGROUND_LOCATION);

    /* For Bluetooth permissions, check if both SCAN and CONNECT are needed.
       If so, show combined disclosure and request both together. */
    final boolean isBluetoothPerm = isBluetoothPermission(permission);
    final boolean requestBothBluetooth =
      isBluetoothPerm &&
      android.os.Build.VERSION.SDK_INT >= 31 &&
      ((Manifest.permission.BLUETOOTH_SCAN.equals(permission) &&
        activity.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) ||
       (Manifest.permission.BLUETOOTH_CONNECT.equals(permission) &&
        activity.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED));

    mainHandler.post(new Runnable() {
        @Override public void run() {
          showRequestPermissionRationale(permission, rationale, handler, requestBackgroundAfter, requestBothBluetooth);
        }
      });

    return true;
  }

  private synchronized int addPermissionHandler(PermissionHandler handler) {
    final int id = nextPermissionHandlerId++;

    if (handler != null)
      permissionHandlers.put(id, handler);

    return id;
  }

  private void doRequestPermission(String permission,
                                   PermissionHandler handler) {
    doRequestPermissions(new String[]{permission}, handler);
  }

  private void doRequestPermissions(String[] permissions,
                                     PermissionHandler handler) {
    if (android.os.Build.VERSION.SDK_INT >= 23) {
      /* Check if activity is in valid state */
      if (activity.isFinishing() || (android.os.Build.VERSION.SDK_INT >= 17 && activity.isDestroyed())) {
        if (handler != null)
          handler.onRequestPermissionsResult(false);
        return;
      }

      /* For Bluetooth permissions on Android 12+, verify they're actually needed */
      for (String perm : permissions) {
        if (isBluetoothPermission(perm) &&
            android.os.Build.VERSION.SDK_INT < 31) {
          /* Bluetooth permissions not needed on Android < 12, granted automatically */
          if (handler != null)
            handler.onRequestPermissionsResult(true);
          return;
        }
      }

      int requestCode = addPermissionHandler(handler);

      try {
        activity.requestPermissions(permissions, requestCode);
      } catch (IllegalStateException e) {
        if (handler != null)
          handler.onRequestPermissionsResult(false);
      } catch (Exception e) {
        if (handler != null)
          handler.onRequestPermissionsResult(false);
      }
    } else if (handler != null) {
      /* On older Android versions, permissions are granted at install time */
      handler.onRequestPermissionsResult(true);
    }
  }

  @Override
  public boolean requestPermission(String permission, PermissionHandler handler) {
    if (android.os.Build.VERSION.SDK_INT < 23)
      /* we don't need to request permissions on this old Android
         version */
      return true;

    /* Skip permission requests in simulator mode - permissions are not needed */
    if (Loader.loaded && NativeView.isSimulator())
      return true;

    if (isPermissionGranted(permission))
      /* we already have the permission */
      return true;

    /* For location, Bluetooth, and notification permissions, always show disclosure dialog before
       requesting permission (required by Google Play policy). For other
       permissions, only show rationale if user previously denied. */
    final boolean isLocationPerm = isLocationPermission(permission);
    final boolean isBluetoothPerm = isBluetoothPermission(permission);
    final boolean isNotificationPerm = isNotificationPermission(permission);

    /* Queue the permission request to prevent overlapping dialogs */
    permissionQueue.offer(new PendingPermissionRequest(permission, handler));
    processNextPermission();
    return false;
  }

  /**
   * Process the next permission request in the queue.
   * Only one permission request is processed at a time to prevent overlapping dialogs.
   */
  private synchronized void processNextPermission() {
    /* Don't process if already processing or queue is empty */
    if (isProcessingPermission || permissionQueue.isEmpty())
      return;

    final PendingPermissionRequest request = permissionQueue.poll();
    if (request == null)
      return;

    final String permission = request.permission;
    
    /* Skip if permission is already granted */
    if (isPermissionGranted(permission)) {
      if (request.handler != null)
        request.handler.onRequestPermissionsResult(true);
      processNextPermission();
      return;
    }

    isProcessingPermission = true;
    PermissionHandler handler = request.handler;

    /* For location, Bluetooth, and notification permissions, always show disclosure dialog before
       requesting permission (required by Google Play policy). For other
       permissions, only show rationale if user previously denied. */
    final boolean isLocationPerm = isLocationPermission(permission);
    final boolean isBluetoothPerm = isBluetoothPermission(permission);
    final boolean isNotificationPerm = isNotificationPermission(permission);

    /* For Bluetooth permissions, check if the other one is also in the queue or needed.
       If so, remove it from queue and handle both together. */
    if (isBluetoothPerm && android.os.Build.VERSION.SDK_INT >= 31) {
      final String otherBluetoothPermission =
        Manifest.permission.BLUETOOTH_SCAN.equals(permission) ?
        Manifest.permission.BLUETOOTH_CONNECT :
        Manifest.permission.BLUETOOTH_SCAN;

      boolean otherNeeded = activity.checkSelfPermission(otherBluetoothPermission) != PackageManager.PERMISSION_GRANTED;

      /* Check if the other Bluetooth permission is in the queue */
      PendingPermissionRequest otherRequest = null;
      for (PendingPermissionRequest req : permissionQueue) {
        if (otherBluetoothPermission.equals(req.permission)) {
          otherRequest = req;
          break;
        }
      }

      /* If the other Bluetooth permission is already being processed (in pendingBluetoothPermissions),
         or if it's already granted, skip this one - it will be handled when the other one is processed */
      if (pendingBluetoothPermissions.contains(otherBluetoothPermission) ||
          (!otherNeeded && isBluetoothPermission(permission))) {
        /* Other Bluetooth permission is already being processed or already granted.
           Skip this one and mark as granted since it will be handled together. */
        isProcessingPermission = false;
        if (handler != null)
          handler.onRequestPermissionsResult(true);
        processNextPermission();
        return;
      }

      if (otherRequest != null && otherNeeded) {
        /* Remove the other Bluetooth permission from queue - we'll handle both together */
        permissionQueue.remove(otherRequest);
        pendingBluetoothPermissions.add(permission);
        pendingBluetoothPermissions.add(otherBluetoothPermission);
        /* Update handler to also call the other permission's handler */
        final PermissionHandler originalHandler = handler;
        final PermissionHandler otherHandler = otherRequest.handler;
        handler = new PermissionHandler() {
            @Override
            public void onRequestPermissionsResult(boolean granted) {
              if (originalHandler != null)
                originalHandler.onRequestPermissionsResult(granted);
              if (otherHandler != null)
                otherHandler.onRequestPermissionsResult(granted);
            }
          };
      } else if (otherNeeded) {
        /* Both are needed, will show combined disclosure */
        pendingBluetoothPermissions.add(permission);
        pendingBluetoothPermissions.add(otherBluetoothPermission);
      } else if (pendingBluetoothPermissions.contains(otherBluetoothPermission)) {
        pendingBluetoothPermissions.add(permission);
      }
    }

    /* Always show consent dialog for location, Bluetooth, and notification permissions */
    final PermissionHandler finalHandler = handler;
    if ((isLocationPerm || isBluetoothPerm || isNotificationPerm ||
         activity.shouldShowRequestPermissionRationale(permission)) &&
        showRequestPermissionRationaleIndirect(permission, new PermissionHandler() {
            @Override
            public void onRequestPermissionsResult(boolean granted) {
              isProcessingPermission = false;
              if (finalHandler != null)
                finalHandler.onRequestPermissionsResult(granted);
              processNextPermission();
            }
          }))
      return;

    doRequestPermission(permission, new PermissionHandler() {
        @Override
        public void onRequestPermissionsResult(boolean granted) {
          isProcessingPermission = false;
          if (finalHandler != null)
            finalHandler.onRequestPermissionsResult(granted);
          processNextPermission();
        }
      });
  }

  @Override
  public synchronized void cancelRequestPermission(PermissionHandler handler) {
    permissionHandlers.values().remove(handler);
  }

  @Override
  public boolean areLocationPermissionsGranted() {
    return areAllLocationPermissionsGranted();
  }

  /**
   * Resume processing the permission queue.
   * Call this when the Activity resumes (e.g., after returning from Settings)
   * to continue processing any pending permission requests.
   */
  public void resumePermissionProcessing() {
    processNextPermission();
  }
}

