// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MacOSMainMenu.hpp"
#include "Language/Language.hpp"
#include "ResourceLoader.hpp"
#include "Resources.hpp"
#include "Version.hpp"

#import <AppKit/AppKit.h>

static NSString *
ToNSString(const char *value)
{
  if (value == nullptr || *value == '\0')
    return @"";

  return [NSString stringWithUTF8String:value];
}

@interface XCSoarAboutPanelController : NSObject
- (void)ShowAboutPanel:(id)sender;
@end

@implementation XCSoarAboutPanelController
- (void)ShowAboutPanel:(id)sender
{
  NSBundle *const bundle = [NSBundle mainBundle];
  NSString *app_name = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
  if (app_name.length == 0)
    app_name = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
  if (app_name.length == 0)
    app_name = @"XCSoar";

  NSString *version =
    [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
  if (version.length == 0)
    version = [NSString stringWithUTF8String:XCSoar_VersionString];
#ifdef GIT_COMMIT_ID
  version = [version stringByAppendingFormat:ToNSString(_(" · git %@")), @GIT_COMMIT_ID];
#endif
  NSString *const build = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
  NSAttributedString *const credits = [[NSAttributedString alloc]
    initWithString:ToNSString(_("© The XCSoar Project\nGPL-2.0-or-later"))];
  NSImage *icon = [[NSImage alloc] initWithContentsOfURL:
    [bundle URLForResource:@"logo_1024" withExtension:@"icns"]];
  if (icon == nil) {
    const auto icon_data = ResourceLoader::Load(IDB_LOGO_HD_RGBA);
    if (!icon_data.empty()) {
      NSData *const data = [NSData dataWithBytes:icon_data.data()
                                          length:icon_data.size_bytes()];
      icon = [[NSImage alloc] initWithData:data];
    }
  }

  NSMutableDictionary *const options = [@{
    NSAboutPanelOptionApplicationName: app_name,
    NSAboutPanelOptionApplicationVersion: version ?: @"",
    NSAboutPanelOptionVersion: build ?: @"",
    NSAboutPanelOptionCredits: credits,
  } mutableCopy];
  if (icon != nil)
    options[NSAboutPanelOptionApplicationIcon] = icon;
  [NSApp orderFrontStandardAboutPanelWithOptions:options];
}
@end

static XCSoarAboutPanelController *macos_about_panel_controller;

void
InitialiseMacOSMainMenu()
{
  NSBundle *const bundle = [NSBundle mainBundle];
  NSString *app_name = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
  if (app_name.length == 0)
    app_name = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
  if (app_name.length == 0)
    app_name = @"XCSoar";

  macos_about_panel_controller = [[XCSoarAboutPanelController alloc] init];

  NSMenu *main_menu = [[NSMenu alloc] init];
  NSMenuItem *application_menu_item = [[NSMenuItem alloc] init];
  [main_menu addItem:application_menu_item];

  NSMenu *application_menu = [[NSMenu alloc] initWithTitle:app_name];

  NSMenuItem *const about_menu_item =
    [application_menu addItemWithTitle:[NSString stringWithFormat:ToNSString(_("About %@")), app_name]
                                 action:@selector(ShowAboutPanel:)
                          keyEquivalent:@""];
  [about_menu_item setTarget:macos_about_panel_controller];
  [application_menu addItem:[NSMenuItem separatorItem]];

  NSMenu *services_menu = [[NSMenu alloc] init];
  NSMenuItem *services_menu_item = [[NSMenuItem alloc] initWithTitle:ToNSString(_("Services"))
                                                                action:nil
                                                         keyEquivalent:@""];
  [services_menu_item setSubmenu:services_menu];
  [application_menu addItem:services_menu_item];
  [NSApp setServicesMenu:services_menu];
  [application_menu addItem:[NSMenuItem separatorItem]];

  [application_menu addItemWithTitle:[NSString stringWithFormat:ToNSString(_("Hide %@")), app_name]
                              action:@selector(hide:)
                       keyEquivalent:@"h"];

  NSMenuItem *hide_others_item =
    [application_menu addItemWithTitle:ToNSString(_("Hide Others"))
                                 action:@selector(hideOtherApplications:)
                          keyEquivalent:@"h"];
  [hide_others_item setKeyEquivalentModifierMask:NSEventModifierFlagCommand |
                                                 NSEventModifierFlagOption];

  [application_menu addItemWithTitle:ToNSString(_("Show All"))
                              action:@selector(unhideAllApplications:)
                       keyEquivalent:@""];
  [application_menu addItem:[NSMenuItem separatorItem]];

  [application_menu addItemWithTitle:[NSString stringWithFormat:ToNSString(_("Quit %@")), app_name]
                              action:@selector(terminate:)
                       keyEquivalent:@"q"];

  [application_menu_item setSubmenu:application_menu];
  [NSApp setMainMenu:main_menu];
}