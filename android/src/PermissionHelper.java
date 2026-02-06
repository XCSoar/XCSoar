// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Map;
import java.util.TreeMap;
import java.util.Queue;
import java.util.LinkedList;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.provider.Settings;

import android.app.NotificationManager;


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

  /**
   * When true, requestPermission() silently returns false for
   * location and notification permissions instead of showing
   * rationale dialogs.  Set by suppressPermissionDialogs() after
   * the user clicks "Not Now" on the onboarding disclosure page.
   */
  private volatile boolean permissionDialogsSuppressed = false;

  /**
   * State for the pending native disclosure dialog.  When
   * processNextPermission() needs to show a disclosure, it saves
   * the permission/handler here and calls the native disclosure.
   * When the user responds, onDisclosureResult() uses these to
   * proceed.
   */
  private String pendingDisclosurePermission = null;
  private PermissionHandler pendingDisclosureHandler = null;
  private boolean pendingDisclosureRequestBothBluetooth = false;
  private boolean pendingDisclosureRequestBackgroundAfter = false;

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

      /* If permission was denied and the system won't show the
         dialog anymore ("Don't ask again"), open the app Settings
         page directly so the user can grant it manually. */
      if (!granted && grantResults.length > 0 &&
          grantResults[0] == PackageManager.PERMISSION_DENIED &&
          permissions.length > 0 &&
          Build.VERSION.SDK_INT >= 23 &&
          !activity.shouldShowRequestPermissionRationale(permissions[0])) {
        try {
          Intent intent = new Intent(
            Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
          intent.setData(android.net.Uri.parse(
            "package:" + activity.getPackageName()));
          activity.startActivity(intent);
        } catch (Exception e) {
          /* Ignore - worst case the user doesn't get redirected */
        }
      }

      handler.onRequestPermissionsResult(granted);
    }
  }

  private static final String FOREGROUND_SERVICE_LOCATION =
    "android.permission.FOREGROUND_SERVICE_LOCATION";

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
   * Check if a permission is Android 14+ foreground service location permission.
   */
  private static boolean isForegroundServiceLocationPermission(String permission) {
    return FOREGROUND_SERVICE_LOCATION.equals(permission);
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
   * Check if notifications are enabled for this app.
   * Returns true if notifications are enabled, false otherwise.
   */
  public boolean areNotificationsEnabled() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      NotificationManager notificationManager =
        (NotificationManager)activity.getSystemService(
          Activity.NOTIFICATION_SERVICE);
      return notificationManager == null ||
        notificationManager.areNotificationsEnabled();
    }
    return true;
  }

  /**
   * Check if all location permissions are granted and invoke callback if ready.
   * Called from permission handlers after location permissions are granted.
   */
  private void checkLocationPermissionsCallback() {
    if (areAllLocationPermissionsGranted() && onLocationPermissionsGranted != null) {
      mainHandler.post(onLocationPermissionsGranted);
    }

    /* Notify native code that the location permission chain completed */
    notifyNativePermissionResult(
      isPermissionGranted(Manifest.permission.ACCESS_FINE_LOCATION));
  }

  /**
   * Notify native code that a permission request completed.
   * Safe to call from any thread; does nothing if native is not loaded.
   */
  private static void notifyNativePermissionResult(boolean granted) {
    if (Loader.loaded)
      NativeView.onPermissionResult(granted);
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

          }
        });
    }
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

    /* If the user chose "Not Now" on the onboarding disclosure page,
       silently skip location and notification permission dialogs for
       this session.  The user already saw the prominent disclosure
       and explicitly declined. */
    if (permissionDialogsSuppressed &&
        (isLocationPermission(permission) ||
         isForegroundServiceLocationPermission(permission) ||
         isNotificationPermission(permission))) {
      if (handler != null)
        handler.onRequestPermissionsResult(false);
      return false;
    }

    /* For location, foreground service location, Bluetooth, and notification permissions,
       always show disclosure dialog before requesting permission (required by Google Play
       policy). For other permissions, only show rationale if user previously denied. */
    final boolean isLocationPerm = isLocationPermission(permission);
    final boolean isBluetoothPerm = isBluetoothPermission(permission);
    final boolean isNotificationPerm = isNotificationPermission(permission);
    final boolean isForegroundServiceLocationPerm =
      isForegroundServiceLocationPermission(permission);

    /* Queue the permission request to prevent overlapping dialogs.
       Synchronize queue operations to ensure thread-safety with processNextPermission(). */
    synchronized (this) {
      permissionQueue.offer(new PendingPermissionRequest(permission, handler));
      processNextPermission();
    }
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

    /* For location, foreground service location, Bluetooth, and notification permissions,
       always show disclosure dialog before requesting permission (required by Google Play
       policy). For other permissions, only show rationale if user previously denied. */
    final boolean isLocationPerm = isLocationPermission(permission);
    final boolean isBluetoothPerm = isBluetoothPermission(permission);
    final boolean isNotificationPerm = isNotificationPermission(permission);
    final boolean isForegroundServiceLocationPerm =
      isForegroundServiceLocationPermission(permission);

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

    /* Show native disclosure dialog for foreground location,
       Bluetooth, and notification permissions (Google Play prominent
       disclosure requirement).  Background location and foreground
       service location don't need a separate disclosure because the
       foreground location disclosure already covers them.  For other
       permissions, show disclosure if the user previously denied. */
    final PermissionHandler finalHandler = handler;
    final boolean isForegroundLoc = isForegroundLocationPermission(permission);
    final boolean isBackgroundLoc = isBackgroundLocationPermission(permission);
    final boolean needsDisclosure =
      isForegroundLoc ||
      isBluetoothPerm || isNotificationPerm ||
      (!isBackgroundLoc && !isForegroundServiceLocationPerm &&
       Build.VERSION.SDK_INT >= 23 &&
       activity.shouldShowRequestPermissionRationale(permission));

    if (needsDisclosure && Loader.loaded) {
      /* Save state for onDisclosureResult() callback */
      pendingDisclosurePermission = permission;
      pendingDisclosureHandler = finalHandler;
      pendingDisclosureRequestBackgroundAfter =
        isForegroundLocationPermission(permission) &&
        android.os.Build.VERSION.SDK_INT >= 29 &&
        !isPermissionGranted(Manifest.permission.ACCESS_BACKGROUND_LOCATION);
      pendingDisclosureRequestBothBluetooth =
        isBluetoothPerm &&
        android.os.Build.VERSION.SDK_INT >= 31 &&
        ((Manifest.permission.BLUETOOTH_SCAN.equals(permission) &&
          !isPermissionGranted(Manifest.permission.BLUETOOTH_CONNECT)) ||
         (Manifest.permission.BLUETOOTH_CONNECT.equals(permission) &&
          !isPermissionGranted(Manifest.permission.BLUETOOTH_SCAN)));

      /* Show native disclosure dialog; onDisclosureResult() will
         be called when the user responds */
      NativeView.showPermissionDisclosure(permission);
      return;
    }

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

  @Override
  public boolean isNotificationPermissionGranted() {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU)
      /* Notification permission not needed before Android 13 */
      return true;

    return isPermissionGranted(Manifest.permission.POST_NOTIFICATIONS);
  }

  @Override
  public void requestAllLocationPermissionsDirect() {
    mainHandler.post(new Runnable() {
        @Override
        public void run() {
          requestLocationChainDirect();
        }
      });
  }

  @Override
  public void requestNotificationPermissionDirect() {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU)
      return;

    if (isPermissionGranted(Manifest.permission.POST_NOTIFICATIONS))
      return;

    mainHandler.post(new Runnable() {
        @Override
        public void run() {
          doRequestPermission(Manifest.permission.POST_NOTIFICATIONS,
            new PermissionHandler() {
              @Override
              public void onRequestPermissionsResult(boolean granted) {
                notifyNativePermissionResult(granted);
              }
            });
        }
      });
  }

  @Override
  public void suppressPermissionDialogs() {
    permissionDialogsSuppressed = true;
  }

  @Override
  public void onDisclosureResult(final boolean accepted) {
    /* This may be called from the native UI thread (via JNI),
       so dispatch everything to the Java main thread */
    mainHandler.post(new Runnable() {
        @Override
        public void run() {
          final String permission = pendingDisclosurePermission;
          final PermissionHandler handler = pendingDisclosureHandler;
          final boolean requestBothBluetooth = pendingDisclosureRequestBothBluetooth;
          final boolean requestBackgroundAfter = pendingDisclosureRequestBackgroundAfter;

          pendingDisclosurePermission = null;
          pendingDisclosureHandler = null;
          pendingDisclosureRequestBothBluetooth = false;
          pendingDisclosureRequestBackgroundAfter = false;

          if (permission == null) {
            isProcessingPermission = false;
            processNextPermission();
            return;
          }

          if (!accepted) {
            isProcessingPermission = false;
            if (handler != null)
              handler.onRequestPermissionsResult(false);
            processNextPermission();
            return;
          }

          /* User accepted disclosure - proceed with system permission dialog */
          final boolean isBluetooth = isBluetoothPermission(permission);
          requestPermissionAfterDialog(permission, new PermissionHandler() {
              @Override
              public void onRequestPermissionsResult(boolean granted) {
                isProcessingPermission = false;
                if (handler != null)
                  handler.onRequestPermissionsResult(granted);
                processNextPermission();
              }
            }, requestBothBluetooth, requestBackgroundAfter, isBluetooth);
        }
      });
  }

  /**
   * Chain location permission requests directly (no rationale dialogs).
   * Called from the onboarding disclosure page which already serves as
   * the Google-Play-required prominent disclosure.
   *
   * Step 1: ACCESS_FINE_LOCATION
   * Step 2: ACCESS_BACKGROUND_LOCATION (API 29+)
   * Step 3: FOREGROUND_SERVICE_LOCATION (API 34+)
   */
  private void requestLocationChainDirect() {
    if (isPermissionGranted(Manifest.permission.ACCESS_FINE_LOCATION)) {
      /* Foreground already granted, proceed to background */
      requestBackgroundLocationDirect();
      return;
    }

    doRequestPermission(Manifest.permission.ACCESS_FINE_LOCATION,
      new PermissionHandler() {
        @Override
        public void onRequestPermissionsResult(boolean granted) {
          if (granted) {
            requestBackgroundLocationDirect();
          } else {
            notifyNativePermissionResult(false);
          }
        }
      });
  }

  private void requestBackgroundLocationDirect() {
    if (Build.VERSION.SDK_INT < 29) {
      /* No background location permission needed before Android 10 */
      checkLocationPermissionsCallback();
      return;
    }

    if (isPermissionGranted(Manifest.permission.ACCESS_BACKGROUND_LOCATION)) {
      requestForegroundServiceLocationDirect();
      return;
    }

    doRequestPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION,
      new PermissionHandler() {
        @Override
        public void onRequestPermissionsResult(boolean granted) {
          if (granted)
            requestForegroundServiceLocationDirect();
          else
            checkLocationPermissionsCallback();
        }
      });
  }

  private void requestForegroundServiceLocationDirect() {
    if (Build.VERSION.SDK_INT < 34) {
      /* No foreground service location permission before Android 14 */
      checkLocationPermissionsCallback();
      return;
    }

    if (isPermissionGranted(FOREGROUND_SERVICE_LOCATION)) {
      checkLocationPermissionsCallback();
      return;
    }

    doRequestPermission(FOREGROUND_SERVICE_LOCATION,
      new PermissionHandler() {
        @Override
        public void onRequestPermissionsResult(boolean granted) {
          checkLocationPermissionsCallback();
        }
      });
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

