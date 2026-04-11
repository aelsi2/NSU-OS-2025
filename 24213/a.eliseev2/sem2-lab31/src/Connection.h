#pragma once

#include "Http.h"
#include "MessageBuffer.h"
#include "ResponseCache.h"
#include "Scheduler.h"
#include "Socket.h"
#include <memory>
#include <string>

namespace proxy {

class Scheduler;

class ConnectionManager {
  public:
    virtual std::shared_ptr<MessageBuffer>
    connect(http::Uri uri, std::shared_ptr<MessageBuffer> request) = 0;
};

class ClientConnection final : public Task {
  public:
    ClientConnection(ConnectionManager &connMan, ResponseCache &cache,
                     std::unique_ptr<Socket> clientSocket);

    bool service(Waker waker) override;

  private:
    std::unique_ptr<Socket> clientSocket;
    std::shared_ptr<MessageBuffer> request;
    std::shared_ptr<MessageBuffer> response;
    size_t responseReadPos;
    ResponseCache &cache;
    ConnectionManager &connMan;
};

class ServerConnection final : public Task {
  public:
    ServerConnection(http::Uri &uri, bool shouldCache, ResponseCache &cache,
                     std::unique_ptr<Socket> serverSocket,
                     std::shared_ptr<MessageBuffer> request,
                     std::shared_ptr<MessageBuffer> response);

    bool service(Waker waker) override;

  private:
    http::Uri uri;
    std::unique_ptr<Socket> serverSocket;
    std::shared_ptr<MessageBuffer> response;
    std::shared_ptr<MessageBuffer> request;
    size_t requestReadPos;
    bool shouldCache;
    bool cached;
    ResponseCache &cache;
};

} // namespace proxy
