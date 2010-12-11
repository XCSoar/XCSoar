/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "Screen/Init.hpp"
#include "Screen/Fonts.hpp"
#include "Simulator.hpp"
#include "Asset.hpp"
#include "Profile/Profile.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "org_xcsoar_NativeView.h"

#include <assert.h>

JavaVM *jvm = NULL;
NativeView *native_view;

/**
 * This function is called by the Java glue code.  It replaces the
 * standard C function main().
 */
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_run(JNIEnv *env, jobject obj,
                               jint width, jint height)
{
  env->GetJavaVM(&jvm);

  assert(native_view == NULL);
  native_view = new NativeView(env, obj, width, height);

  /* force simulatior mode until GPS support has been implemented */
  global_simulator_flag = true;
  sim_set_in_cmd_line_flag = true;

  InitAsset();

  Profile::SetFiles(_T(""));

  ScreenGlobalInit screen_init;

  if (XCSoarInterface::Startup(NULL))
    CommonInterface::main_window.event_loop();

  CommonInterface::main_window.reset();

  Fonts::Deinitialize();

  delete native_view;
};
