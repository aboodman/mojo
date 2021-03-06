// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/network/public/interfaces/net_address.mojom.h"
#include "services/http_server/public/http_server.mojom.h"
#include "services/http_server/public/http_server_factory.mojom.h"
#include "services/http_server/public/http_server_util.h"
#include "shell/test/pingable.mojom.h"

namespace mojo {

namespace {

std::string GetURL(uint16_t port, const std::string& path) {
  return base::StringPrintf("http://127.0.0.1:%u/%s", port, path.c_str());
}

}  // namespace

class GetHandler : public http_server::HttpHandler {
 public:
  GetHandler(InterfaceRequest<http_server::HttpHandler> request, uint16_t port)
      : binding_(this, request.Pass()), port_(port) {
    CHECK(PathService::Get(base::FILE_MODULE, &app_path_));
    app_path_ = app_path_.DirName().Append("pingable_app.mojo");
    CHECK(base::PathExists(app_path_));
  }
  ~GetHandler() override {}

 private:
  // http_server::HttpHandler:
  void HandleRequest(
      http_server::HttpRequestPtr request,
      const Callback<void(http_server::HttpResponsePtr)>& callback) override {
    http_server::HttpResponsePtr response;
    if (StartsWithASCII(request->relative_url, "/app", true)) {
      // Super inefficient, but meh.
      std::string data;
      base::ReadFileToString(app_path_, &data);
      response = http_server::CreateHttpResponse(200, data);
      response->content_type = "application/octet-stream";
    } else if (request->relative_url == "/redirect") {
      response = http_server::HttpResponse::New();
      response->status_code = 302;
      response->custom_headers.insert("Location", GetURL(port_, "app"));
    } else {
      NOTREACHED();
    }

    callback.Run(response.Pass());
  }

  Binding<http_server::HttpHandler> binding_;
  base::FilePath app_path_;
  uint16_t port_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(GetHandler);
};

typedef test::ApplicationTestBase ShellAppTest;

class ShellHTTPAppTest : public test::ApplicationTestBase {
 public:
  ShellHTTPAppTest() : ApplicationTestBase() {}
  ~ShellHTTPAppTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();

    application_impl()->ConnectToService("mojo:http_server",
                                         &http_server_factory_);

    mojo::NetAddressPtr local_address(mojo::NetAddress::New());
    local_address->family = mojo::NET_ADDRESS_FAMILY_IPV4;
    local_address->ipv4 = mojo::NetAddressIPv4::New();
    local_address->ipv4->addr.resize(4);
    local_address->ipv4->addr[0] = 127;
    local_address->ipv4->addr[1] = 0;
    local_address->ipv4->addr[2] = 0;
    local_address->ipv4->addr[3] = 1;
    local_address->ipv4->port = 0;
    http_server_factory_->CreateHttpServer(GetProxy(&http_server_),
                                           local_address.Pass());

    http_server_->GetPort([this](uint16_t p) { port_ = p; });
    EXPECT_TRUE(http_server_.WaitForIncomingMethodCall());

    InterfacePtr<http_server::HttpHandler> http_handler;
    handler_.reset(new GetHandler(GetProxy(&http_handler).Pass(), port_));
    http_server_->SetHandler(".*", http_handler.Pass(),
                             [](bool result) { EXPECT_TRUE(result); });
    EXPECT_TRUE(http_server_.WaitForIncomingMethodCall());
  }

  std::string GetURL(const std::string& path) {
    return ::mojo::GetURL(port_, path);
  }

  http_server::HttpServerFactoryPtr http_server_factory_;
  http_server::HttpServerPtr http_server_;
  scoped_ptr<GetHandler> handler_;
  uint16_t port_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(ShellHTTPAppTest);
};

#if defined(OS_ANDROID)
// These tests rely on data that needs to be bundled into the apptest binary in
// order to work on Android.
#else
// Test that we can load apps over http.
TEST_F(ShellHTTPAppTest, Http) {
  InterfacePtr<Pingable> pingable;
  application_impl()->ConnectToService(GetURL("app"), &pingable);
  pingable->Ping("hello",
                 [this](const String& app_url, const String& connection_url,
                        const String& message) {
                   EXPECT_EQ(GetURL("app"), app_url);
                   EXPECT_EQ(GetURL("app"), connection_url);
                   EXPECT_EQ("hello", message);
                   base::MessageLoop::current()->Quit();
                 });
  base::RunLoop().Run();
}

// Test that redirects work.
// TODO(aa): Test that apps receive the correct URL parameters.
TEST_F(ShellHTTPAppTest, Redirect) {
  InterfacePtr<Pingable> pingable;
  application_impl()->ConnectToService(GetURL("redirect"), &pingable);
  pingable->Ping("hello",
                 [this](const String& app_url, const String& connection_url,
                        const String& message) {
                   EXPECT_EQ(GetURL("app"), app_url);
                   EXPECT_EQ(GetURL("app"), connection_url);
                   EXPECT_EQ("hello", message);
                   base::MessageLoop::current()->Quit();
                 });
  base::RunLoop().Run();
}

// Test that querystring is not considered when resolving http applications.
TEST_F(ShellHTTPAppTest, QueryHandling) {
  InterfacePtr<Pingable> pingable1;
  InterfacePtr<Pingable> pingable2;
  application_impl()->ConnectToService(GetURL("app?foo"), &pingable1);
  application_impl()->ConnectToService(GetURL("app?bar"), &pingable2);

  int num_responses = 0;
  auto callback = [this, &num_responses](const String& app_url,
                                         const String& connection_url,
                                         const String& message) {
    EXPECT_EQ(GetURL("app"), app_url);
    EXPECT_EQ("hello", message);
    ++num_responses;
    if (num_responses == 1) {
      EXPECT_EQ(GetURL("app?foo"), connection_url);
    } else if (num_responses == 2) {
      EXPECT_EQ(GetURL("app?bar"), connection_url);
      base::MessageLoop::current()->Quit();
    } else {
      CHECK(false);
    }
  };
  pingable1->Ping("hello", callback);
  pingable2->Ping("hello", callback);
  base::RunLoop().Run();
}
#endif

// mojo: URLs can have querystrings too
TEST_F(ShellAppTest, MojoURLQueryHandling) {
  InterfacePtr<Pingable> pingable;
  application_impl()->ConnectToService("mojo:pingable_app?foo", &pingable);
  auto callback = [this](const String& app_url, const String& connection_url,
                         const String& message) {
    EXPECT_TRUE(EndsWith(app_url, "/pingable_app.mojo", true));
    EXPECT_EQ(app_url.To<std::string>() + "?foo", connection_url);
    EXPECT_EQ("hello", message);
    base::MessageLoop::current()->Quit();
  };
  pingable->Ping("hello", callback);
  base::RunLoop().Run();
}

}  // namespace mojo
