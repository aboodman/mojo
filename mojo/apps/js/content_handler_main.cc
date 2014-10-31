// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/icu_util.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/apps/js/js_app.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"

namespace mojo {
namespace apps {

class JsContentHandler : public ApplicationDelegate,
                         public ContentHandlerFactory::Delegate {
 public:
  JsContentHandler() : content_handler_factory_(this) {}

 private:
  // Overridden from ApplicationDelegate:
  void Initialize(ApplicationImpl* app) override {
    base::i18n::InitializeICU();
    gin::IsolateHolder::Initialize(gin::IsolateHolder::kStrictMode,
                                   gin::ArrayBufferAllocator::SharedInstance());
  }

  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Overridden from ContentHandlerFactory::Delegate:
  scoped_ptr<ContentHandlerFactory::HandledApplicationHolder> CreateApplication(
      ShellPtr shell,
      URLResponsePtr response) override {
    return make_scoped_ptr(new JSApp(shell.Pass(), response.Pass()));
  }

  ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(JsContentHandler);
};

}  // namespace apps
}  // namespace mojo

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new mojo::apps::JsContentHandler);
  return runner.Run(shell_handle);
}
