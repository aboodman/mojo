// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/base_jni_onload.h"
#include "base/android/base_jni_registrar.h"
#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "base/bind.h"
#include "services/native_viewport/platform_viewport_android.h"
#include "shell/android/android_handler.h"
#include "shell/android/keyboard_impl.h"
#include "shell/android/mojo_main.h"

namespace {

base::android::RegistrationMethod kMojoRegisteredMethods[] = {
    {"AndroidHandler", mojo::RegisterAndroidHandlerJni},
    {"Keyboard", mojo::shell::RegisterKeyboardJni},
    {"MojoMain", mojo::shell::RegisterMojoMain},
    {"PlatformViewportAndroid",
     native_viewport::PlatformViewportAndroid::Register},
};

bool RegisterJNI(JNIEnv* env) {
  if (!base::android::RegisterJni(env))
    return false;

  return RegisterNativeMethods(env, kMojoRegisteredMethods,
                               arraysize(kMojoRegisteredMethods));
}

}  // namespace

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  std::vector<base::android::RegisterCallback> register_callbacks;
  register_callbacks.push_back(base::Bind(&RegisterJNI));
  if (!base::android::OnJNIOnLoadRegisterJNI(vm, register_callbacks) ||
      !base::android::OnJNIOnLoadInit(
          std::vector<base::android::InitCallback>())) {
    return -1;
  }

  return JNI_VERSION_1_4;
}
