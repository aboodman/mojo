// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_DOMAIN_SOCKET_COMPLETION_CALLBACK_H__
#define SHELL_DOMAIN_SOCKET_COMPLETION_CALLBACK_H__

#include "base/callback.h"
#include "base/cancelable_callback.h"

namespace mojo {
namespace shell {

// A callback specialization that takes a single int parameter. Usually this is
// used to report a byte count or network error code.
typedef base::Callback<void(int)> CompletionCallback;

// 64bit version of callback specialization that takes a single int64 parameter.
// Usually this is used to report a file offset, size or network error code.
typedef base::Callback<void(int64)> Int64CompletionCallback;

typedef base::CancelableCallback<void(int)> CancelableCompletionCallback;

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_DOMAIN_SOCKET_COMPLETION_CALLBACK_H__