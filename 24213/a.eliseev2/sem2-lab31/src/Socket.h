#pragma once

#include "Scheduler.h"
#include <cstdint>
#include <memory>
#include <poll.h>
#include <unordered_map>
#include <vector>

namespace proxy {

enum class SocketEvents : int {
    None = 0,
    Read = POLLRDNORM,
    Write = POLLOUT,

    // Can't be specified in await filter.
    Error = POLLERR | POLLHUP,
};

constexpr SocketEvents operator&(SocketEvents lhs, SocketEvents rhs) {
    return static_cast<SocketEvents>(static_cast<int>(lhs) &
                                     static_cast<int>(rhs));
}

constexpr SocketEvents operator|(SocketEvents lhs, SocketEvents rhs) {
    return static_cast<SocketEvents>(static_cast<int>(lhs) |
                                     static_cast<int>(rhs));
}

class Socket;

class SocketManager {
    friend class Socket;

  public:
    SocketManager(uint16_t listenPort);

    // SocketManager is non-copyable because it owns the server socket.
    SocketManager(const SocketManager &) = delete;
    SocketManager &operator=(const SocketManager &) = delete;

    // SocketManager is immovable because Sockets keep pointers to it.
    SocketManager(SocketManager &&) = delete;
    SocketManager &operator=(SocketManager &&) = delete;

    // Polls the existing sockets.
    // Accepts any incoming connection, if there is one, and returns the new
    // socket, otherwise returns null.
    std::unique_ptr<Socket> poll();

    // Establishes a new connection. Returns the socket.
    std::unique_ptr<Socket> connect(const char *hostname, uint16_t port);

  private:
    void close(Socket &socket);
    void updateRequestedEvents(Socket &socket, SocketEvents events);

    std::vector<pollfd> pollFds;
    std::vector<Socket *> sockets;
    std::unordered_map<int, size_t> fdIndexMap;
};

// A network socket that can be polled, read from and written to.
// Must not outlive the parent SocketManager.
class Socket {
    friend class SocketManager;

  public:
    Socket(int fd, SocketManager &manager, bool connected);
    ~Socket();

    // Sockets are non-copyable because they own FDs.
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    // Sockets are immovable because the parent SocketManager 
    // keeps pointers to them.
    Socket &operator=(Socket &&) = delete;
    Socket(Socket &&socket) = delete;

    // Gets the events that have occurred since the last call.
    SocketEvents handleEvents();

    // Waits for one or more events,
    // wakes the task up when at least one occurs.
    // Must be called on every wake-up.
    void await(Waker waker, SocketEvents events);

    // Reads data from the socket to a buffer.
    size_t read(char *buffer, size_t size);

    // Writes data from a buffer to the socket.
    size_t write(const char *buffer, size_t size);

  private:
    // Updates the actualEvents and schedules the awaiting tasks.
    void updateActualEvents(SocketEvents events);

    int fd;
    bool connected;
    SocketEvents actualEvents;
    Waker awaiter;
    SocketManager *manager;
};

} // namespace proxy
