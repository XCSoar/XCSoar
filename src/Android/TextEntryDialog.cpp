/*
Copyright_License {

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

#include "TextEntryDialog.hpp"
#include "Main.hpp"
#include "net/http/DownloadManager.hpp"
#include "Form/Form.hpp"
#include "Context.hpp"
#include "java/Class.hxx"
#include "java/Closeable.hxx"
#include "java/Env.hxx"
#include "java/String.hxx"
#include "org_xcsoar_TextEntryDialog.h"
#include "UIGlobals.hpp"

#include <cassert>

static Java::TrivialClass text_entry_dialog_class;
static jmethodID ctor;

static Java::LocalCloseable
NewTextEntryDialog(JNIEnv *env, AndroidTextEntryDialog &instance,
                   Context &context, jstring title, jstring value, jint type)
{
  return Java::LocalCloseable{
    Java::NewObjectRethrow(env, text_entry_dialog_class, ctor,
                           (jlong)(std::size_t)&instance,
                           context.Get(), title, value, type)};
}

AndroidTextEntryDialog::AndroidTextEntryDialog() noexcept
:fake_dialog(UIGlobals::GetDialogLook())
{
}

void
AndroidTextEntryDialog::Initialise(JNIEnv *env) noexcept
{
  assert(text_entry_dialog_class == nullptr);
  assert(env != nullptr);

  text_entry_dialog_class.Find(env, "org/xcsoar/TextEntryDialog");

  ctor = env->GetMethodID(text_entry_dialog_class, "<init>",
                          "(JLandroid/content/Context;Ljava/lang/String;Ljava/lang/String;I)V");
}

void
AndroidTextEntryDialog::Deinitialise(JNIEnv *env) noexcept
{
  text_entry_dialog_class.Clear(env);
}

JNIEXPORT void JNICALL
Java_org_xcsoar_TextEntryDialog_onResult(JNIEnv *env, jobject obj,
                                         jlong ptr, jstring value)
{
  auto &dialog = *(AndroidTextEntryDialog *)(std::size_t)ptr;
  dialog.OnResult(env, value);
}

std::optional<std::string>
AndroidTextEntryDialog::ShowModal(JNIEnv *env,
                                  Context &context,
                                  const char *title,
                                  const char *initial_value,
                                  Type type)
{
  auto obj = NewTextEntryDialog(env, *this, context,
                                Java::String(env, title).Get(),
                                Java::String(env, initial_value).Get(),
                                jint(type));

  /* this WndForm instance only exists so we can (ab)use its modal
     event loop in XCSoar native UI thread while the native Android
     dialog runs in the Android Java UI thread */
  fake_dialog.Create(UIGlobals::GetMainWindow(), {0, 0, 100, 100});
  fake_dialog.ShowModal();

  return std::move(new_value);
}

void
AndroidTextEntryDialog::OnResult(JNIEnv *env, jstring _value) noexcept
{
  if (_value != nullptr)
    new_value = Java::String::GetUTFChars(env, _value).c_str();

  /* this callback is invoked in the Android Java UI thread, so we use
     UI::Notify to move to the native UI thread */
  notify.SendNotification();
}
