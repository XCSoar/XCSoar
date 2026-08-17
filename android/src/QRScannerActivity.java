// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;
import android.view.Gravity;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Scans a task QR code with the device camera and hands the decoded
 * text to native code.
 *
 * This is the in-app counterpart to {@link ReceiveTaskActivity}: that
 * one receives an "xctsk:" link decoded by some other app, this one
 * decodes the code itself so the pilot never leaves XCSoar.  Both end
 * up in the same native task-receive path.
 *
 * The caller (XCSoar.scanQRCode()) has already acquired the camera
 * permission.
 */
public class QRScannerActivity extends Activity {
  private static final String TAG = "XCSoar";

  /**
   * Preferred analysis resolution.  Big enough to resolve a dense
   * XCTrack task code, small enough to decode at frame rate.
   */
  private static final int TARGET_WIDTH = 1280;
  private static final int TARGET_HEIGHT = 720;

  private TextureView textureView;
  private TextView statusView;

  private HandlerThread cameraThread;
  private Handler cameraHandler;

  private CameraManager cameraManager;
  private String cameraId;
  private Size previewSize;

  /* written by the camera thread in onOpened()/onConfigured() and read
     by the UI thread in onPause(), hence volatile */
  private volatile CameraDevice camera;
  private volatile CameraCaptureSession captureSession;
  private volatile ImageReader imageReader;
  private volatile Surface previewSurface;

  /**
   * Set while the activity is paused.  The camera opens
   * asynchronously, so it can arrive after onPause() has already
   * looked for something to close; this tells the callback to hand it
   * straight back instead of leaving the camera held.
   */
  private volatile boolean paused = true;

  /**
   * Set once a code has been decoded, so that the frames still in
   * flight cannot deliver a second result.
   */
  private final AtomicBoolean delivered = new AtomicBoolean();

  @Override protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    setContentView(buildContentView());

    cameraManager = (CameraManager)getSystemService(CAMERA_SERVICE);

    /* One thread for the whole Activity, rather than one per resume:
       the camera framework keeps posting callbacks while close() runs,
       and a thread torn down in onPause() would still be receiving
       them ("sending message to a Handler on a dead thread"). */
    cameraThread = new HandlerThread("QRScanner");
    cameraThread.start();
    cameraHandler = new Handler(cameraThread.getLooper());
  }

  @Override protected void onDestroy() {
    final HandlerThread thread = cameraThread;
    final Handler handler = cameraHandler;
    cameraThread = null;
    cameraHandler = null;

    if (handler != null)
      /* Quit from the camera thread itself, so the teardown onPause()
         posted there has finished first.  Calling quitSafely() straight
         from here races it, and the camera framework then finds the
         looper gone while it is still delivering close callbacks. */
      handler.post(thread::quitSafely);

    super.onDestroy();
  }

  private View buildContentView() {
    final FrameLayout root = new FrameLayout(this);
    root.setBackgroundColor(Color.BLACK);

    textureView = new TextureView(this);
    root.addView(textureView,
                 new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                              ViewGroup.LayoutParams.MATCH_PARENT));

    /* hint above the button, so a gloved thumb aiming for Cancel
       cannot cover the only feedback the pilot gets */
    final LinearLayout bottom = new LinearLayout(this);
    bottom.setOrientation(LinearLayout.VERTICAL);

    statusView = new TextView(this);
    statusView.setText("Point the camera at a task QR code");
    statusView.setTextColor(Color.WHITE);
    statusView.setBackgroundColor(Color.argb(160, 0, 0, 0));
    statusView.setPadding(24, 16, 24, 16);
    statusView.setGravity(Gravity.CENTER);
    bottom.addView(statusView,
                   new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                 ViewGroup.LayoutParams.WRAP_CONTENT));

    final Button cancelButton = new Button(this);
    cancelButton.setText("Cancel");
    cancelButton.setAllCaps(false);
    cancelButton.setOnClickListener(v -> cancel());

    /* tall enough to hit without looking, like the dialog buttons in
       the rest of the app */
    final LinearLayout.LayoutParams buttonParams =
      new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                    (int)(56 * getResources()
                                          .getDisplayMetrics().density));
    bottom.addView(cancelButton, buttonParams);

    final FrameLayout.LayoutParams bottomParams =
      new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                   ViewGroup.LayoutParams.WRAP_CONTENT);
    bottomParams.gravity = Gravity.BOTTOM;
    root.addView(bottom, bottomParams);

    return root;
  }

  /**
   * Give up on scanning and go back to XCSoar.
   *
   * Simply finishing is not enough: the main Activity is
   * "singleInstance", so it sits in a task of its own and Android
   * would drop the pilot on the home screen instead of back into the
   * map.  ReceiveTaskActivity does the same thing on its way out.
   */
  private void cancel() {
    startActivity(new Intent(this, XCSoar.class));
    finish();
  }

  @Override public void onBackPressed() {
    cancel();
  }

  @Override protected void onResume() {
    super.onResume();

    paused = false;

    if (textureView.isAvailable())
      openCamera();
    else
      textureView.setSurfaceTextureListener(surfaceTextureListener);
  }

  @Override protected void onPause() {
    paused = true;

    /* Hand the resources over as locals and clear the fields right
       away, so a teardown still in progress cannot close a camera, or
       release a surface, that a quick resume has meanwhile made. */
    final CameraCaptureSession session = captureSession;
    final CameraDevice device = camera;
    final ImageReader reader = imageReader;
    final Surface surface = previewSurface;
    captureSession = null;
    camera = null;
    imageReader = null;
    previewSurface = null;

    if (cameraHandler != null)
      /* Close on the camera thread, which is where onImageAvailable()
         runs: that lets a decode in flight finish instead of having the
         ImageReader closed from under it.  The thread outlives this,
         and is stopped in onDestroy(). */
      cameraHandler.post(() -> closeCamera(session, device, reader, surface));
    else
      closeCamera(session, device, reader, surface);

    super.onPause();
  }

  private final TextureView.SurfaceTextureListener surfaceTextureListener =
    new TextureView.SurfaceTextureListener() {
      @Override
      public void onSurfaceTextureAvailable(SurfaceTexture surface,
                                            int width, int height) {
        openCamera();
      }

      @Override
      public void onSurfaceTextureSizeChanged(SurfaceTexture surface,
                                              int width, int height) {
        configureTransform();
      }

      @Override
      public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        return true;
      }

      @Override
      public void onSurfaceTextureUpdated(SurfaceTexture surface) {
      }
    };

  /**
   * Pick the rear camera, or any camera if the device has no rear one.
   */
  private String selectCamera() throws CameraAccessException {
    String fallback = null;

    for (String id : cameraManager.getCameraIdList()) {
      final Integer facing = cameraManager.getCameraCharacteristics(id)
        .get(CameraCharacteristics.LENS_FACING);

      if (facing != null && facing == CameraCharacteristics.LENS_FACING_BACK)
        return id;

      if (fallback == null)
        fallback = id;
    }

    return fallback;
  }

  /**
   * Choose the largest supported YUV size that does not exceed
   * {@link #TARGET_WIDTH} x {@link #TARGET_HEIGHT}, falling back to
   * the smallest size the camera offers if none of them fit.
   */
  private static long area(Size size) {
    return (long)size.getWidth() * size.getHeight();
  }

  private static Size selectPreviewSize(Size[] choices) {
    /* the largest size that still fits the analysis budget: more
       pixels resolve a denser code */
    Size best = null;
    for (Size size : choices)
      if (size.getWidth() <= TARGET_WIDTH && size.getHeight() <= TARGET_HEIGHT &&
          (best == null || area(size) > area(best)))
        best = size;

    if (best != null)
      return best;

    /* nothing fits, so take the smallest on offer rather than
       binarising full resolution frames for every preview frame */
    for (Size size : choices)
      if (best == null || area(size) < area(best))
        best = size;

    return best;
  }

  private void openCamera() {
    try {
      cameraId = selectCamera();
      if (cameraId == null) {
        fail("No camera found");
        return;
      }

      final StreamConfigurationMap map = cameraManager
        .getCameraCharacteristics(cameraId)
        .get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
      if (map == null) {
        fail("Camera not usable");
        return;
      }

      final Size[] sizes = map.getOutputSizes(ImageFormat.YUV_420_888);
      if (sizes == null || sizes.length == 0) {
        fail("Camera not usable");
        return;
      }

      previewSize = selectPreviewSize(sizes);

      imageReader = ImageReader.newInstance(previewSize.getWidth(),
                                            previewSize.getHeight(),
                                            ImageFormat.YUV_420_888, 2);
      imageReader.setOnImageAvailableListener(imageAvailableListener,
                                              cameraHandler);

      configureTransform();

      cameraManager.openCamera(cameraId, cameraStateCallback, cameraHandler);
    } catch (CameraAccessException e) {
      Log.e(TAG, "Failed to access camera", e);
      fail("Cannot access the camera");
    } catch (SecurityException e) {
      Log.e(TAG, "No camera permission", e);
      fail("No camera permission");
    } catch (Exception e) {
      Log.e(TAG, "Failed to open camera", e);
      fail("Cannot open the camera");
    }
  }

  /**
   * Release the camera resources.  Takes them as parameters rather
   * than reading the fields, because it runs on the camera thread
   * after onPause() has already handed ownership over.
   */
  private static void closeCamera(CameraCaptureSession session,
                                  CameraDevice device,
                                  ImageReader reader,
                                  Surface surface) {
    if (session != null) {
      try {
        /* stop first, so the camera is not still filling the reader we
           are about to close */
        session.stopRepeating();
      } catch (Exception e) {
        /* ignore - we are tearing down anyway */
      }

      try {
        session.close();
      } catch (Exception e) {
        /* ignore */
      }
    }

    if (device != null)
      device.close();

    if (reader != null) {
      /* drop the listener first so no further frame can arrive while
         the reader is going away */
      try {
        reader.setOnImageAvailableListener(null, null);
      } catch (Exception e) {
        /* ignore */
      }
      reader.close();
    }

    if (surface != null)
      surface.release();
  }

  private final CameraDevice.StateCallback cameraStateCallback =
    new CameraDevice.StateCallback() {
      @Override public void onOpened(CameraDevice cameraDevice) {
        if (paused) {
          /* onPause() has already been and gone; it cannot have seen
             this device, so give it back here */
          cameraDevice.close();
          return;
        }

        camera = cameraDevice;
        startPreview();
      }

      @Override public void onDisconnected(CameraDevice cameraDevice) {
        cameraDevice.close();
        camera = null;
      }

      @Override public void onError(CameraDevice cameraDevice, int error) {
        Log.e(TAG, "Camera error " + error);
        cameraDevice.close();
        camera = null;
        fail("Camera error");
      }
    };

  private void startPreview() {
    try {
      final SurfaceTexture texture = textureView.getSurfaceTexture();
      /* read the reader once: onPause() clears the field from the UI
         thread while this runs on the camera thread */
      final ImageReader reader = imageReader;
      if (texture == null || camera == null || reader == null || paused)
        return;

      texture.setDefaultBufferSize(previewSize.getWidth(),
                                    previewSize.getHeight());

      final Surface surface = new Surface(texture);
      if (paused) {
        /* onPause() has already taken ownership of everything it could
           see, so nothing would ever release this one */
        surface.release();
        return;
      }

      /* kept in a field so it can be released in closeCamera(): a
         Surface holds a native reference that finalization would
         otherwise be left to clean up.  The ImageReader's surface is
         owned by the reader and must not be released here. */
      previewSurface = surface;
      final Surface analysisSurface = reader.getSurface();

      final CaptureRequest.Builder builder =
        camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
      builder.addTarget(surface);
      builder.addTarget(analysisSurface);
      builder.set(CaptureRequest.CONTROL_AF_MODE,
                  CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);

      camera.createCaptureSession(Arrays.asList(surface,
                                                 analysisSurface),
        new CameraCaptureSession.StateCallback() {
          @Override
          public void onConfigured(CameraCaptureSession session) {
            if (paused || camera == null) {
              /* paused while the session was being configured */
              try {
                session.close();
              } catch (Exception e) {
                /* ignore */
              }
              return;
            }

            captureSession = session;

            try {
              session.setRepeatingRequest(builder.build(), null,
                                          cameraHandler);
            } catch (Exception e) {
              Log.e(TAG, "Failed to start preview", e);
              fail("Cannot start the camera preview");
            }
          }

          @Override
          public void onConfigureFailed(CameraCaptureSession session) {
            Log.e(TAG, "Failed to configure capture session");
            fail("Cannot start the camera preview");
          }
        }, cameraHandler);
    } catch (Exception e) {
      Log.e(TAG, "Failed to create capture session", e);
      fail("Cannot start the camera preview");
    }
  }

  /**
   * Scale the preview to fill the view without distorting it, taking
   * the display rotation into account.
   */
  private void configureTransform() {
    if (previewSize == null || textureView == null)
      return;

    final int viewWidth = textureView.getWidth();
    final int viewHeight = textureView.getHeight();
    if (viewWidth == 0 || viewHeight == 0)
      return;

    final int rotation = getWindowManager().getDefaultDisplay().getRotation();

    /* the camera delivers landscape buffers; swap them when the
       display is portrait */
    final boolean swapped = rotation == Surface.ROTATION_0 ||
      rotation == Surface.ROTATION_180;
    final float bufferWidth = swapped
      ? previewSize.getHeight() : previewSize.getWidth();
    final float bufferHeight = swapped
      ? previewSize.getWidth() : previewSize.getHeight();

    final float scale = Math.max(viewWidth / bufferWidth,
                                  viewHeight / bufferHeight);

    final Matrix matrix = new Matrix();
    matrix.setScale(bufferWidth * scale / viewWidth,
                    bufferHeight * scale / viewHeight,
                    viewWidth / 2f, viewHeight / 2f);

    if (rotation == Surface.ROTATION_90)
      matrix.postRotate(270, viewWidth / 2f, viewHeight / 2f);
    else if (rotation == Surface.ROTATION_270)
      matrix.postRotate(90, viewWidth / 2f, viewHeight / 2f);
    else if (rotation == Surface.ROTATION_180)
      matrix.postRotate(180, viewWidth / 2f, viewHeight / 2f);

    textureView.setTransform(matrix);
  }

  private final ImageReader.OnImageAvailableListener imageAvailableListener =
    new ImageReader.OnImageAvailableListener() {
      @Override public void onImageAvailable(ImageReader imageReader) {
        /* acquireLatestImage() drops the frames that piled up while
           the previous decode was running */
        final Image image = imageReader.acquireLatestImage();
        if (image == null)
          return;

        try {
          if (!delivered.get())
            decode(image);
        } finally {
          image.close();
        }
      }
    };

  /**
   * Try to decode one camera frame, and deliver the result if it
   * contains a QR code.
   */
  private void decode(Image image) {
    final String text = decodeQRCode(image);
    if (text == null)
      return;

    if (!delivered.compareAndSet(false, true))
      /* another frame won the race */
      return;

    runOnUiThread(() -> deliver(text));
  }

  /**
   * @return the decoded QR text, or null if this frame holds no
   * readable QR code
   */
  private String decodeQRCode(Image image) {
    final Image.Plane[] planes = image.getPlanes();
    if (planes == null || planes.length == 0)
      return null;

    final Image.Plane plane = planes[0];
    final ByteBuffer buffer = plane.getBuffer();

    if (!buffer.isDirect())
      /* decodeQRCode() reads the buffer through
         GetDirectBufferAddress(); Camera2 always hands out direct
         buffers, so this is a guard rather than a real case */
      return null;

    return decodeQRCode(buffer, image.getWidth(), image.getHeight(),
                        plane.getRowStride(), plane.getPixelStride());
  }

  /**
   * Decode a QR code from the Y plane of a camera frame, using
   * zxing-cpp through src/Task/QRDecoder.cpp.
   *
   * The buffer must be direct: native code reads the camera's memory
   * in place rather than copying the plane, and the strides are passed
   * through so a padded plane needs no repacking.
   *
   * @return the decoded text, or null if the frame holds no readable
   * QR code
   */
  private static native String decodeQRCode(ByteBuffer plane,
                                            int width, int height,
                                            int rowStride, int pixelStride);

  /**
   * Hand the decoded text to native code and return to the map.
   */
  private void deliver(String text) {
    if (!Loader.loaded) {
      fail("Error");
      return;
    }

    Log.d(TAG, "Scanned QR code");

    final String msg = NativeView.onReceiveTaskQRCode(text);
    if (msg != null) {
      /* not a task, or a broken one - stay open so the pilot can
         simply aim at another code */
      delivered.set(false);
      statusView.setText(msg);
      return;
    }

    /* the task was accepted; the main activity shows it in the task
       manager */
    startActivity(new Intent(this, XCSoar.class));
    finish();
  }

  private void fail(String message) {
    Log.e(TAG, "QR scanner failed: " + message);
    runOnUiThread(() -> {
      Toast.makeText(this, message, Toast.LENGTH_LONG).show();
      cancel();
    });
  }
}
