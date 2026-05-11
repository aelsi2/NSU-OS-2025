#include "Http.h"
#include <arpa/inet.h>
#include <cassert>
#include <iostream>
#include <netdb.h>
#include <new>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

using namespace client;

static sockaddr resolveAddress(const char *hostname, in_port_t port,
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
    assert(addr.sa_family == AF_INET && "Resolved non-IPv4 address");

    sockaddr_in &ipAddr = reinterpret_cast<sockaddr_in &>(addr);
    char ipString[INET_ADDRSTRLEN];
    if (!::inet_ntop(AF_INET, &ipAddr.sin_addr, ipString, sizeof(ipString))) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not parse IPv4 address");
    }

    ::freeaddrinfo(result);
    return addr;
}

static void printHelp(std::ostream &stream, const char *progamName) {
    stream << "Usage: " << progamName << " URI\n"
           << " URI : The URI to request.\n";
}

static bool getUri(int argc, char *argv[], http::Uri uri) {
    if (argc != 2) {
        printHelp(std::cerr, argv[0]);
        return false;
    }
    const char *ptr = argv[1];
    const char *end = ptr + std::strlen(ptr) + 1;
    try {
        if (!http::Uri::parseClient(&ptr, end, uri)) {
            throw std::runtime_error("Incomplete URI");
        }
    } catch (std::runtime_error &error) {
        std::cerr << "Invalid URI: " << error.what() << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    http::Uri uri;
    try {
        if (!getUri(argc, argv, uri)) {
            return 1;
        }

    } catch (std::bad_alloc &error) {
        std::cerr << "Out of memory!\n";
        _exit(1);
    } catch (std::exception &error) {
        std::cerr << "Fatal error: " << error.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error: " << std::endl;
        return 1;
    }
    return 0;
}
