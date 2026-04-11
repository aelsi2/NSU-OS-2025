#include "Socket.h"
#include <arpa/inet.h>
#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace proxy {

constexpr int CONN_QUEUE_SIZE = 10;

SocketManager::SocketManager(uint16_t listenPort) {
    int sockFd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (sockFd == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not open listening socket");
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = ::htons(listenPort);

    if (bind(sockFd, (struct sockaddr *)&addr, sizeof(addr))) {
        int error = errno;
        ::close(sockFd);
        throw std::system_error(error, std::generic_category(),
                                "Could not bind listening socket");
    }
    if (listen(sockFd, CONN_QUEUE_SIZE)) {
        int error = errno;
        ::close(sockFd);
        throw std::system_error(error, std::generic_category(),
                                "Could not listen on socket");
    }
    pollFds.push_back({
        .fd = sockFd,
        .events = POLLIN,
    });
    sockets.push_back(nullptr);
}

std::unique_ptr<Socket> SocketManager::poll() {
    if (::poll(pollFds.data(), pollFds.size(), -1) == -1) {
        throw std::system_error(errno, std::generic_category(), "Poll failed");
    }
    for (int i = pollFds.size() - 1; i >= 0; i--) {
        pollfd &pollFd = pollFds[i];
        Socket &socket = *sockets[i];
        if (!pollFd.revents) {
            continue;
        }
        SocketEvents events;
        if (!socket.connected) {
            events = SocketEvents::None;
        } else {
            events = static_cast<SocketEvents>(pollFd.revents);
        }
        pollFd.events = 0;
        socket.updateActualEvents(events);
    }
    pollfd &serverPollFd = pollFds[0];
    if (serverPollFd.revents & POLLIN) {
        sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        int clientFd = accept4(serverPollFd.fd, (sockaddr *)&addr, &addr_len,
                               SOCK_NONBLOCK);
        if (clientFd == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    "Could not accept client");
        }
        std::unique_ptr<Socket> clientSocket =
            std::make_unique<Socket>(clientFd, *this, true);
        pollFds.push_back({.fd = clientFd});
        sockets.push_back(&*clientSocket);
        fdIndexMap[clientFd] = pollFds.size() - 1;
        return clientSocket;
    }
    return nullptr;
}

static sockaddr resolveAddress(const char *hostname, uint16_t port,
                               socklen_t *addrLen) {
    std::string strPort = std::to_string(port);

    addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int gaiError = ::getaddrinfo(hostname, strPort.c_str(), &hints, &result);
    if (gaiError) {
        throw std::runtime_error("Could not resolve address");
    }

    sockaddr addr = *result->ai_addr;
    *addrLen = result->ai_addrlen;
    ::freeaddrinfo(result);
    return addr;
}

std::unique_ptr<Socket> SocketManager::connect(const char *hostname,
                                               uint16_t port) {
    socklen_t addrLen;
    sockaddr sockAddr = resolveAddress(hostname, port, &addrLen);

    int sockFd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockFd == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not open target socket");
    }

    bool pending = false;
    if (::connect(sockFd, (sockaddr *)&sockAddr, addrLen)) {
        if (errno == EINPROGRESS) {
            pending = true;
        } else {
            int error = errno;
            ::close(sockFd);
            throw std::system_error(error, std::generic_category(),
                                    "Could connect socket");
        }
    }
    std::unique_ptr<Socket> sock =
        std::make_unique<Socket>(sockFd, *this, !pending);
    pollFds.push_back({
        .fd = sockFd,
        .events = static_cast<short>(pending ? POLLOUT : 0),
    });
    sockets.push_back(&*sock);
    fdIndexMap[sockFd] = pollFds.size() - 1;
    return sock;
}

void SocketManager::updateRequestedEvents(Socket &socket, SocketEvents events) {
    if (!socket.connected) {
        return;
    }
    auto it = fdIndexMap.find(socket.fd);
    if (it == fdIndexMap.end()) {
        return;
    }
    size_t index = it->second;
    pollFds[index].events |= static_cast<short>(events);
}

void SocketManager::close(Socket &socket) {
    int fd = socket.fd;

    ::close(fd);

    auto it = fdIndexMap.find(fd);
    if (it == fdIndexMap.end()) {
        return;
    }
    size_t index = it->second;

    pollFds.erase(pollFds.begin() + index);
    sockets.erase(sockets.begin() + index);
    fdIndexMap.erase(fd);
    for (auto &kvp : fdIndexMap) {
        if (kvp.second > index) {
            kvp.second -= 1;
        }
    }
}

Socket::Socket(int fd, SocketManager &manager, bool connected)
    : fd(fd), manager(&manager), connected(connected),
      actualEvents(SocketEvents::None) {
}

Socket::~Socket() {
    manager->close(*this);
}

SocketEvents Socket::handleEvents() {
    SocketEvents events = actualEvents;
    actualEvents = SocketEvents::None;
    return events;
}

void Socket::await(Waker waker, SocketEvents events) {
    awaiter = waker;
    manager->updateRequestedEvents(*this, events);
}

size_t Socket::read(char *buffer, size_t size) {
    ssize_t read_count = ::read(fd, buffer, size);
    if (read_count < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "Error while reading from socket");
    }
    return read_count;
}

size_t Socket::write(const char *buffer, size_t size) {
    ssize_t write_count = ::write(fd, buffer, size);
    if (write_count < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "Error while writing to socket");
    }
    return write_count;
}

void Socket::updateActualEvents(SocketEvents events) {
    actualEvents = actualEvents | events;
    connected = true;
    awaiter.schedule();
    awaiter = {};
}

} // namespace proxy
