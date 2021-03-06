// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'mojo:application';
import 'mojo:bindings';
import 'mojo:core';

import 'package:services/dart/test/pingpong_service.mojom.dart';

class PingPongServiceImpl implements PingPongService {
  PingPongServiceStub _stub;
  Application _application;
  PingPongClientProxy _pingPongClient;

  PingPongServiceImpl(Application application, MojoMessagePipeEndpoint endpoint)
      : _application = application {
    _stub = new PingPongServiceStub.fromEndpoint(endpoint)
            ..delegate = this
            ..listen();
  }

  void setClient(ProxyBase proxyBase) {
    assert(_pingPongClient == null);
    _pingPongClient = proxyBase;
  }

  void ping(int pingValue) => _pingPongClient.ptr.pong(pingValue + 1);

  void quit() {
    if (_pingPongClient != null) {
      _pingPongClient.close();
      _pingPongClient = null;
    }
    _stub.close();
    if (_application != null) {
      _application.close();
    }
  }
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void acceptConnection(String requestorUrl, ApplicationConnection connection) {
    connection.provideService(PingPongServiceName,
        (endpoint) => new PingPongServiceImpl(this, endpoint));
    connection.listen();
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  new PingPongApplication.fromHandle(appHandle);
}
