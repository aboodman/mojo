// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_APPLICATION_MANAGER_H_
#define SHELL_APPLICATION_MANAGER_APPLICATION_MANAGER_H_

#include <map>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "shell/application_manager/application_loader.h"
#include "shell/application_manager/application_manager_export.h"
#include "shell/application_manager/fetcher.h"
#include "shell/application_manager/shell_impl.h"
#include "url/gurl.h"

namespace base {
class FilePath;
class SequencedWorkerPool;
}

namespace mojo {

// ApplicationManager requires implementations of NativeRunner and
// NativeRunnerFactory to run native applications.
class MOJO_APPLICATION_MANAGER_EXPORT NativeRunner {
 public:
  // Parameter for |Start| to specify its cleanup behavior.
  enum CleanupBehavior { DeleteAppPath, DontDeleteAppPath };
  virtual ~NativeRunner() {}

  // Loads the app in the file at |app_path| and runs it on some other
  // thread/process. If |cleanup_behavior| is |true|, takes ownership of the
  // file. |app_completed_callback| is posted (to the thread on which |Start()|
  // was called) after |MojoMain()| completes.
  virtual void Start(const base::FilePath& app_path,
                     CleanupBehavior cleanup_behavior,
                     InterfaceRequest<Application> application_request,
                     const base::Closure& app_completed_callback) = 0;
};

class MOJO_APPLICATION_MANAGER_EXPORT NativeRunnerFactory {
 public:
  virtual ~NativeRunnerFactory() {}
  virtual scoped_ptr<NativeRunner> Create() = 0;
};

class MOJO_APPLICATION_MANAGER_EXPORT ApplicationManager {
 public:
  class MOJO_APPLICATION_MANAGER_EXPORT Delegate {
   public:
    virtual ~Delegate();
    // Send when the Application holding the handle on the other end of the
    // Shell pipe goes away.
    virtual void OnApplicationError(const GURL& url);
    virtual GURL ResolveURL(const GURL& url);
    virtual GURL ResolveMappings(const GURL& url);
  };

  // API for testing.
  class MOJO_APPLICATION_MANAGER_EXPORT TestAPI {
   public:
    explicit TestAPI(ApplicationManager* manager);
    ~TestAPI();

    // Returns true if the shared instance has been created.
    static bool HasCreatedInstance();
    // Returns true if there is a ShellImpl for this URL.
    bool HasFactoryForURL(const GURL& url) const;

   private:
    ApplicationManager* manager_;

    DISALLOW_COPY_AND_ASSIGN(TestAPI);
  };

  explicit ApplicationManager(Delegate* delegate);
  ~ApplicationManager();

  // Loads a service if necessary and establishes a new client connection.
  void ConnectToApplication(const GURL& application_url,
                            const GURL& requestor_url,
                            InterfaceRequest<ServiceProvider> services,
                            ServiceProviderPtr exposed_services);

  template <typename Interface>
  inline void ConnectToService(const GURL& application_url,
                               InterfacePtr<Interface>* ptr) {
    ScopedMessagePipeHandle service_handle =
        ConnectToServiceByName(application_url, Interface::Name_);
    ptr->Bind(service_handle.Pass());
  }

  ScopedMessagePipeHandle ConnectToServiceByName(
      const GURL& application_url,
      const std::string& interface_name);

  void RegisterContentHandler(const std::string& mime_type,
                              const GURL& content_handler_url);

  void RegisterExternalApplication(const GURL& application_url,
                                   const std::vector<std::string>& args,
                                   ApplicationPtr application);

  // Sets the default Loader to be used if not overridden by SetLoaderForURL()
  // or SetLoaderForScheme().
  void set_default_loader(scoped_ptr<ApplicationLoader> loader) {
    default_loader_ = loader.Pass();
  }
  void set_native_runner_factory(
      scoped_ptr<NativeRunnerFactory> runner_factory) {
    native_runner_factory_ = runner_factory.Pass();
  }
  void set_blocking_pool(base::SequencedWorkerPool* blocking_pool) {
    blocking_pool_ = blocking_pool;
  }
  void set_disable_cache(bool disable_cache) { disable_cache_ = disable_cache; }
  // Sets a Loader to be used for a specific url.
  void SetLoaderForURL(scoped_ptr<ApplicationLoader> loader, const GURL& url);
  // Sets a Loader to be used for a specific url scheme.
  void SetLoaderForScheme(scoped_ptr<ApplicationLoader> loader,
                          const std::string& scheme);
  // These strings will be passed to the Initialize() method when an
  // Application is instantiated.
  void SetArgsForURL(const std::vector<std::string>& args, const GURL& url);

  // Destroys all Shell-ends of connections established with Applications.
  // Applications connected by this ApplicationManager will observe pipe errors
  // and have a chance to shutdown.
  void TerminateShellConnections();

  // Removes a ShellImpl when it encounters an error.
  void OnShellImplError(ShellImpl* shell_impl);

 private:
  class ContentHandlerConnection;

  typedef std::map<std::string, ApplicationLoader*> SchemeToLoaderMap;
  typedef std::map<GURL, ApplicationLoader*> URLToLoaderMap;
  typedef std::map<GURL, ShellImpl*> URLToShellImplMap;
  typedef std::map<GURL, ContentHandlerConnection*> URLToContentHandlerMap;
  typedef std::map<GURL, std::vector<std::string>> URLToArgsMap;
  typedef std::map<std::string, GURL> MimeTypeToURLMap;

  bool ConnectToRunningApplication(const GURL& resolved_url,
                                   const GURL& requestor_url,
                                   InterfaceRequest<ServiceProvider>* services,
                                   ServiceProviderPtr* exposed_services);

  bool ConnectToApplicationWithLoader(
      const GURL& requested_url,
      const GURL& resolved_url,
      const GURL& requestor_url,
      InterfaceRequest<ServiceProvider>* services,
      ServiceProviderPtr* exposed_services,
      ApplicationLoader* loader);

  InterfaceRequest<Application> RegisterShell(
      // The original URL requested by client, before any resolution or
      // redirects.
      // This is mostly useless and should be removed.
      const GURL& original_url,
      // The URL after resolution and redirects, including the querystring.
      const GURL& resolved_url,
      const GURL& requestor_url,
      InterfaceRequest<ServiceProvider> services,
      ServiceProviderPtr exposed_services);

  ShellImpl* GetShellImpl(const GURL& url);

  void ConnectToClient(ShellImpl* shell_impl,
                       const GURL& resolved_url,
                       const GURL& requestor_url,
                       InterfaceRequest<ServiceProvider> services,
                       ServiceProviderPtr exposed_services);

  void HandleFetchCallback(const GURL& requested_url,
                           const GURL& requestor_url,
                           InterfaceRequest<ServiceProvider> services,
                           ServiceProviderPtr exposed_services,
                           NativeRunner::CleanupBehavior cleanup_behavior,
                           scoped_ptr<Fetcher> fetcher);

  void RunNativeApplication(InterfaceRequest<Application> application_request,
                            NativeRunner::CleanupBehavior cleanup_behavior,
                            scoped_ptr<Fetcher> fetcher,
                            const base::FilePath& file_path,
                            bool path_exists);

  void LoadWithContentHandler(const GURL& content_handler_url,
                              InterfaceRequest<Application> application_request,
                              URLResponsePtr url_response);

  // Return the appropriate loader for |url|. This can return NULL if there is
  // no loader configured for the URL.
  ApplicationLoader* GetLoaderForURL(const GURL& url);

  // Removes a ContentHandler when it encounters an error.
  void OnContentHandlerError(ContentHandlerConnection* content_handler);

  // Returns the arguments for the given url.
  Array<String> GetArgsForURL(const GURL& url);

  void CleanupRunner(NativeRunner* runner);

  Delegate* delegate_;
  // Loader management.
  // Loaders are chosen in the order they are listed here.
  URLToLoaderMap url_to_loader_;
  SchemeToLoaderMap scheme_to_loader_;
  scoped_ptr<ApplicationLoader> default_loader_;
  scoped_ptr<NativeRunnerFactory> native_runner_factory_;

  URLToShellImplMap url_to_shell_impl_;
  URLToContentHandlerMap url_to_content_handler_;
  URLToArgsMap url_to_args_;

  base::WeakPtrFactory<ApplicationManager> weak_ptr_factory_;

  base::SequencedWorkerPool* blocking_pool_;
  NetworkServicePtr network_service_;
  MimeTypeToURLMap mime_type_to_url_;
  ScopedVector<NativeRunner> native_runners_;
  bool disable_cache_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationManager);
};

}  // namespace mojo

#endif  // SHELL_APPLICATION_MANAGER_APPLICATION_MANAGER_H_
