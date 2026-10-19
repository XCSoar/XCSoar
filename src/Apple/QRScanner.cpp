// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/QRScanner.hpp"

#include <TargetConditionals.h>

/* macOS has no camera scanner (yet), and the decoder needs zxing-cpp */
#if TARGET_OS_IPHONE && defined(HAVE_ZXING)

#include "QR/QRCode.hpp"
#include "Task/ReceiveTask.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "util/Exception.hxx"

#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

/**
 * The height of the Cancel button, in points; tall enough to hit
 * without looking, like the dialog buttons in the rest of the app.
 */
static constexpr CGFloat CANCEL_BUTTON_HEIGHT = 56;

static NSString *
ToNSString(const char *s) noexcept
{
  NSString *result = [NSString stringWithUTF8String:s];

  /* stringWithUTF8String: returns nil for anything that is not valid
     UTF-8, and a nil label text would silently blank the hint */
  return result != nil ? result : @"";
}

/**
 * Pick the rear camera, or any camera if the device has no rear one.
 */
static AVCaptureDevice *
SelectCamera() noexcept
{
  AVCaptureDevice *device = [AVCaptureDevice
    defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera
                      mediaType:AVMediaTypeVideo
                       position:AVCaptureDevicePositionBack];
  if (device != nil)
    return device;

  return [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
}

/**
 * Decode one camera frame.
 *
 * Plane 0 of a bi-planar 4:2:0 buffer is the luminance plane with one
 * byte per pixel, which is exactly what DecodeQRCode() wants, so this
 * borrows the capture buffer's memory instead of copying a megabyte per
 * frame.  The row stride is passed through as well, so a padded plane
 * needs no repacking either.
 *
 * @return the decoded text, or an empty string if the frame holds no
 * readable QR code
 */
static std::string
DecodeSampleBuffer(CMSampleBufferRef sample_buffer) noexcept
{
  CVImageBufferRef image_buffer = CMSampleBufferGetImageBuffer(sample_buffer);
  if (image_buffer == nullptr || !CVPixelBufferIsPlanar(image_buffer))
    /* we asked for a bi-planar format, so this is a guard rather than
       a real case */
    return {};

  if (CVPixelBufferLockBaseAddress(image_buffer,
                                   kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess)
    return {};

  const auto *luminance = (const uint8_t *)
    CVPixelBufferGetBaseAddressOfPlane(image_buffer, 0);
  const std::size_t width = CVPixelBufferGetWidthOfPlane(image_buffer, 0);
  const std::size_t height = CVPixelBufferGetHeightOfPlane(image_buffer, 0);
  const std::size_t row_stride =
    CVPixelBufferGetBytesPerRowOfPlane(image_buffer, 0);

  std::string text;
  if (luminance != nullptr && row_stride > 0)
    /* DecodeQRCode() lets zxing-cpp reject a buffer too small for the
       geometry, so there is no bounds arithmetic to reproduce here */
    text = DecodeQRCode(luminance, row_stride * height,
                        width, height, row_stride, 1);

  CVPixelBufferUnlockBaseAddress(image_buffer, kCVPixelBufferLock_ReadOnly);

  return text;
}

/**
 * Pick a capture format DecodeSampleBuffer() can read.
 *
 * Both of these are bi-planar 4:2:0, i.e. plane 0 is the 8 bit
 * luminance plane; full range is what iOS offers by default and video
 * range is the fallback.  The difference between the two is the
 * luminance range, which does not matter to a binarising decoder.
 *
 * Asking for a format the output does not list raises an
 * NSInvalidArgumentException, so this must not simply assume one.
 *
 * @return 0 if the camera offers neither
 */
static OSType
SelectPixelFormat(AVCaptureVideoDataOutput *output) noexcept
{
  static constexpr OSType wanted[] = {
    kCVPixelFormatType_420YpCbCr8BiPlanarFullRange,
    kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
  };

  for (const OSType format : wanted)
    if ([output.availableVideoCVPixelFormatTypes containsObject:@(format)])
      return format;

  return 0;
}

/**
 * A full-screen camera preview that scans one task QR code.
 *
 * This is the iOS counterpart of Android's QRScannerActivity: it is
 * presented on top of the XCSoar window and hands the decoded text to
 * ReceiveTaskQRCode(), i.e. both ports end up in the same native
 * task-receive path.
 *
 * XCSoar's own event loop keeps running behind this - SDL pumps the
 * UIKit run loop on every iteration - so the task manager that started
 * the scan is still there to receive the task.
 */
@interface XCSoarQRScannerViewController
  : UIViewController<AVCaptureVideoDataOutputSampleBufferDelegate>
@end

@implementation XCSoarQRScannerViewController
{
  AVCaptureSession *session;
  AVCaptureVideoPreviewLayer *preview_layer;

  /**
   * The serial queue AVFoundation delivers frames on.  The capture
   * session is started and stopped there too, so that a decode in
   * flight finishes before the session goes away.
   */
  dispatch_queue_t video_queue;

  UILabel *status_label;

  /**
   * Set once a code has been decoded, so that the frames still in
   * flight cannot deliver a second result.  Read on #video_queue and
   * written on both that and the main thread, hence atomic.
   */
  std::atomic<bool> delivered;
}

- (instancetype)init
{
  self = [super init];
  if (self != nil) {
    delivered.store(false, std::memory_order_relaxed);
    video_queue = dispatch_queue_create("org.xcsoar.QRScanner",
                                        DISPATCH_QUEUE_SERIAL);
  }

  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.view.backgroundColor = UIColor.blackColor;

  /* hint above the button, so a gloved thumb aiming for Cancel cannot
     cover the only feedback the pilot gets */
  status_label = [[UILabel alloc] init];
  status_label.text = ToNSString(_("Point the camera at a task QR code"));
  status_label.textColor = UIColor.whiteColor;
  status_label.backgroundColor = [UIColor colorWithWhite:0 alpha:0.63];
  status_label.textAlignment = NSTextAlignmentCenter;
  status_label.numberOfLines = 0;
  status_label.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:status_label];

  UIButton *cancel_button = [UIButton buttonWithType:UIButtonTypeSystem];
  [cancel_button setTitle:ToNSString(_("Cancel"))
                 forState:UIControlStateNormal];
  [cancel_button setTitleColor:UIColor.whiteColor
                      forState:UIControlStateNormal];
  cancel_button.backgroundColor = [UIColor colorWithWhite:0 alpha:0.8];
  cancel_button.translatesAutoresizingMaskIntoConstraints = NO;
  [cancel_button addTarget:self
                    action:@selector(onCancel)
          forControlEvents:UIControlEventTouchUpInside];
  [self.view addSubview:cancel_button];

  /* keep the button clear of the home indicator where there is one */
  NSLayoutYAxisAnchor *bottom_anchor;
  if (@available(iOS 11.0, *))
    bottom_anchor = self.view.safeAreaLayoutGuide.bottomAnchor;
  else
    bottom_anchor = self.view.bottomAnchor;

  [NSLayoutConstraint activateConstraints:@[
    [status_label.leadingAnchor
      constraintEqualToAnchor:self.view.leadingAnchor],
    [status_label.trailingAnchor
      constraintEqualToAnchor:self.view.trailingAnchor],
    [status_label.bottomAnchor
      constraintEqualToAnchor:cancel_button.topAnchor],

    [cancel_button.leadingAnchor
      constraintEqualToAnchor:self.view.leadingAnchor],
    [cancel_button.trailingAnchor
      constraintEqualToAnchor:self.view.trailingAnchor],
    [cancel_button.bottomAnchor constraintEqualToAnchor:bottom_anchor],
    [cancel_button.heightAnchor
      constraintEqualToConstant:CANCEL_BUTTON_HEIGHT],
  ]];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];

  [self requestCameraAccess];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];

  [self stopCapture];
}

- (void)viewDidLayoutSubviews
{
  [super viewDidLayoutSubviews];

  preview_layer.frame = self.view.bounds;
  [self updateVideoOrientation];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];

  [coordinator animateAlongsideTransition:nil
                               completion:^([[maybe_unused]] id<UIViewControllerTransitionCoordinatorContext> context) {
    [self updateVideoOrientation];
  }];
}

/**
 * Ask for the camera permission, then start scanning.  Unlike Android,
 * iOS answers straight away for every state but "not determined".
 */
- (void)requestCameraAccess
{
  switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]) {
  case AVAuthorizationStatusAuthorized:
    [self startCapture];
    break;

  case AVAuthorizationStatusNotDetermined:
    [self askForCameraAccess];
    break;

  case AVAuthorizationStatusDenied:
  case AVAuthorizationStatusRestricted:
    /* say so rather than staring at a black screen with no explanation
       for why nothing happens */
    [self failWithMessage:N_("No camera permission")];
    break;
  }
}

/**
 * The first scan ever: iOS puts up its own permission sheet and answers
 * asynchronously.
 */
- (void)askForCameraAccess
{
  [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                           completionHandler:^(BOOL granted) {
    /* the completion handler runs on an arbitrary queue, and all of
       UIKit needs the main one */
    dispatch_async(dispatch_get_main_queue(), ^{
      if (granted)
        [self startCapture];
      else
        [self failWithMessage:N_("No camera permission")];
    });
  }];
}

- (void)startCapture
{
  if (session != nil)
    /* -viewDidAppear: runs again whenever something presented on top
       of us goes away */
    return;

  AVCaptureDevice *device = SelectCamera();
  if (device == nil) {
    [self failWithMessage:N_("No camera found")];
    return;
  }

  NSError *error = nil;
  AVCaptureDeviceInput *input =
    [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
  if (input == nil) {
    LogFormat("QR scanner: cannot open the camera: %s",
              error.localizedDescription.UTF8String);
    [self failWithMessage:N_("Cannot open the camera")];
    return;
  }

  AVCaptureVideoDataOutput *output = [[AVCaptureVideoDataOutput alloc] init];

  /* drop the frames that pile up while a decode is running, like
     Android's acquireLatestImage() */
  output.alwaysDiscardsLateVideoFrames = YES;

  AVCaptureSession *new_session = [[AVCaptureSession alloc] init];
  [new_session beginConfiguration];

  /* big enough to resolve a dense XCTrack task code, small enough to
     decode at frame rate: the same budget as the Android scanner */
  if ([new_session canSetSessionPreset:AVCaptureSessionPreset1280x720])
    new_session.sessionPreset = AVCaptureSessionPreset1280x720;

  if (![new_session canAddInput:input] || ![new_session canAddOutput:output]) {
    [new_session commitConfiguration];
    [self failWithMessage:N_("Cannot start the camera preview")];
    return;
  }

  [new_session addInput:input];
  [new_session addOutput:output];

  /* only now does the output know what the camera can deliver */
  const OSType pixel_format = SelectPixelFormat(output);
  if (pixel_format == 0) {
    [new_session commitConfiguration];
    [self failWithMessage:N_("Camera not usable")];
    return;
  }

  output.videoSettings = @{
    (id)kCVPixelBufferPixelFormatTypeKey: @(pixel_format),
  };

  [output setSampleBufferDelegate:self queue:video_queue];

  [new_session commitConfiguration];

  if ([device lockForConfiguration:nil]) {
    if ([device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus])
      device.focusMode = AVCaptureFocusModeContinuousAutoFocus;
    [device unlockForConfiguration];
  }

  preview_layer = [[AVCaptureVideoPreviewLayer alloc]
                    initWithSession:new_session];
  preview_layer.videoGravity = AVLayerVideoGravityResizeAspectFill;
  preview_layer.frame = self.view.bounds;

  /* below the hint and the button, which were added in -viewDidLoad */
  [self.view.layer insertSublayer:preview_layer atIndex:0];

  session = new_session;

  [self updateVideoOrientation];

  /* -startRunning blocks for a moment, and this is the thread XCSoar's
     event loop runs on */
  dispatch_async(video_queue, ^{
    [new_session startRunning];
  });
}

- (void)stopCapture
{
  AVCaptureSession *old_session = session;
  session = nil;

  [preview_layer removeFromSuperlayer];
  preview_layer = nil;

  if (old_session != nil)
    /* on the video queue, which is where the frame callback runs: that
       lets a decode in flight finish instead of having the session
       stopped from under it */
    dispatch_async(video_queue, ^{
      [old_session stopRunning];
    });
}

/**
 * Turn the preview the right way up.  The decoder does not care - QR
 * detection finds the finder patterns at any angle - but the pilot
 * aiming the camera does.
 */
- (void)updateVideoOrientation
{
  AVCaptureConnection *connection = preview_layer.connection;
  if (connection == nil || !connection.supportsVideoOrientation)
    return;

  connection.videoOrientation = [self videoOrientation];
}

- (AVCaptureVideoOrientation)videoOrientation
{
  UIInterfaceOrientation orientation = UIInterfaceOrientationPortrait;

  if (@available(iOS 13.0, *)) {
    UIWindowScene *scene = self.view.window.windowScene;
    if (scene != nil)
      orientation = scene.interfaceOrientation;
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    /* the scene API is iOS 13+, and XCSoar still supports iOS 11 (and
       iOS 10 on armv7), where this is the only way to ask */
    orientation = UIApplication.sharedApplication.statusBarOrientation;
#pragma clang diagnostic pop
  }

  switch (orientation) {
  case UIInterfaceOrientationPortraitUpsideDown:
    return AVCaptureVideoOrientationPortraitUpsideDown;

  case UIInterfaceOrientationLandscapeLeft:
    return AVCaptureVideoOrientationLandscapeLeft;

  case UIInterfaceOrientationLandscapeRight:
    return AVCaptureVideoOrientationLandscapeRight;

  case UIInterfaceOrientationPortrait:
  case UIInterfaceOrientationUnknown:
    break;
  }

  return AVCaptureVideoOrientationPortrait;
}

/* method from protocol AVCaptureVideoDataOutputSampleBufferDelegate */
- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sample_buffer
           fromConnection:(AVCaptureConnection *)connection
{
  if (delivered.load(std::memory_order_relaxed))
    return;

  const auto text = DecodeSampleBuffer(sample_buffer);
  if (text.empty())
    return;

  if (delivered.exchange(true, std::memory_order_relaxed))
    /* the main thread was still deciding what to do with an earlier
       result */
    return;

  NSString *ns_text = ToNSString(text.c_str());
  dispatch_async(dispatch_get_main_queue(), ^{
    [self deliver:ns_text];
  });
}

/**
 * Hand the decoded text to the task-receive path and return to the map.
 */
- (void)deliver:(NSString *)text
{
  try {
    ReceiveTaskQRCode(text.UTF8String);
  } catch (...) {
    /* not a task, or a broken one - stay open so the pilot can simply
       aim at another code */
    status_label.text =
      ToNSString(GetFullMessage(std::current_exception()).c_str());
    delivered.store(false, std::memory_order_relaxed);
    return;
  }

  LogFormat("Scanned QR code");

  /* the task was accepted; MainWindow::OnTaskReceived() shows it in the
     task manager that is still open behind us */
  [self dismiss];
}

- (void)onCancel
{
  [self dismiss];
}

- (void)dismiss
{
  /* a frame still in flight must not deliver a task after this */
  delivered.store(true, std::memory_order_relaxed);

  [self stopCapture];
  [self dismissViewControllerAnimated:YES completion:nil];
}

/**
 * Give up on scanning, but stay on screen: the message is all the pilot
 * gets to explain why, and Cancel is right below it.
 *
 * @param message an untranslated English reason, as marked up with
 * N_() by the callers: it is logged as it is, because log output stays
 * English, and translated for the status line
 */
- (void)failWithMessage:(const char *)message
{
  LogFormat("QR scanner failed: %s", message);

  [self stopCapture];
  status_label.text = ToNSString(gettext(message));
}

@end

/**
 * Find the view controller to present the scanner on top of.
 *
 * SDL owns the one and only UIWindow and we have no handle on it here,
 * so ask UIApplication.  Its -windows is deprecated in favour of the
 * scene API, which XCSoar cannot rely on while it still supports
 * iOS 11.
 */
static UIViewController *
FindTopViewController() noexcept
{
  UIWindow *key_window = nil;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  for (UIWindow *window in UIApplication.sharedApplication.windows) {
    if (window.rootViewController == nil)
      continue;

    if (window.isKeyWindow) {
      key_window = window;
      break;
    }

    if (key_window == nil)
      /* fall back to the first usable window, in case none of them is
         key just yet */
      key_window = window;
  }
#pragma clang diagnostic pop

  if (key_window == nil)
    return nil;

  UIViewController *controller = key_window.rootViewController;
  while (controller.presentedViewController != nil)
    controller = controller.presentedViewController;

  return controller;
}

bool
HaveQRScanner() noexcept
{
  return FindTopViewController() != nil && SelectCamera() != nil;
}

void
ScanTaskQRCode() noexcept
{
  UIViewController *presenter = FindTopViewController();
  if (presenter == nil)
    return;

  XCSoarQRScannerViewController *scanner =
    [[XCSoarQRScannerViewController alloc] init];
  scanner.modalPresentationStyle = UIModalPresentationFullScreen;

  [presenter presentViewController:scanner animated:YES completion:nil];
}

#else /* !TARGET_OS_IPHONE || !HAVE_ZXING */

bool
HaveQRScanner() noexcept
{
  return false;
}

void
ScanTaskQRCode() noexcept
{
}

#endif /* !TARGET_OS_IPHONE || !HAVE_ZXING */
