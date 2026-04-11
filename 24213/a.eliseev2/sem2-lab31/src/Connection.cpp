#include "Connection.h"

using namespace proxy;

constexpr size_t READ_BUF_SIZE = 1024;

ClientConnection::ClientConnection(ConnectionManager &connMan,
                                   ResponseCache &cache,
                                   std::unique_ptr<Socket> clientSocket)
    : clientSocket(std::move(clientSocket)),
      request(std::make_shared<MessageBuffer>()), cache(cache),
      connMan(connMan) {
}

bool ClientConnection::service(Waker waker) {
    SocketEvents events = clientSocket->handleEvents();

    if ((events & SocketEvents::Error) != SocketEvents::None) {
        // We've got an error. Terminate the connection.
        return true;
    }
    if (events == SocketEvents::None) {
        // We're being initialized. Wait for read.
        clientSocket->await(waker, SocketEvents::Read);
        return false;
    }

    if ((events & SocketEvents::Read) != SocketEvents::None) {
        MessageBuffer::Writer writer = request->write();
        char *ptr = writer.reserve(READ_BUF_SIZE);
        size_t count = clientSocket->read(ptr, READ_BUF_SIZE);
        writer.commit(count);

        if (count == 0) {
            // Client has closed the connection.
            writer.end();
            return true;
        }
    }

    if (!response) {
        size_t readPos = 0;
        MessageBuffer::Reader reader = request->read(readPos);

        http::RequestLine line;
        if (!http::RequestLine::parse(reader, line)) {
            // Request line is not ready. Wait.
            clientSocket->await(waker, SocketEvents::Read);
            return false;
        }

        if (line.method == http::RequestMethod::Get) {
            response = cache.getEntry(line.uri);
        }
        if (!response) {
            response = connMan.connect(line.uri, request);
        }
    }

    MessageBuffer::Reader reader = response->read(responseReadPos);
    if (reader.length() > 0) {
        if ((events & SocketEvents::Write) != SocketEvents::None) {
            reader.advance(clientSocket->write(reader.data(), reader.length()));
        }
        clientSocket->await(waker, SocketEvents::Read | SocketEvents::Write);
    } else if (reader.isEnd()) {
        // Server has closed the connection and we have no more data to send.
        return true;
    } else {
        // We don't have any new response data. Wait.
        clientSocket->await(waker, SocketEvents::Read);
        reader.await(waker);
    }

    return false;
}

ServerConnection::ServerConnection(http::Uri &uri, bool shouldCache,
                                   ResponseCache &cache,
                                   std::unique_ptr<Socket> serverSocket,
                                   std::shared_ptr<MessageBuffer> request,
                                   std::shared_ptr<MessageBuffer> response)
    : uri(uri), shouldCache(shouldCache), cache(cache),
      serverSocket(std::move(serverSocket)), request(std::move(request)),
      response(std::move(response)), cached(false) {
}

bool ServerConnection::service(Waker waker) {
    SocketEvents events = serverSocket->handleEvents();

    if ((events & SocketEvents::Error) != SocketEvents::None) {
        // We've got an error. Remove the cache entry 
        // and terminate the connection.
        if (cached) {
            cache.removeEntry(uri);
        }
        return true;
    }

    if ((events & SocketEvents::Read) != SocketEvents::None) {
        MessageBuffer::Writer writer = response->write();
        char *ptr = writer.reserve(READ_BUF_SIZE);
        size_t count = serverSocket->read(ptr, READ_BUF_SIZE);
        writer.commit(count);

        if (count == 0) {
            // Server has closed the connection.
            writer.end();
            return true;
        }
    }

    if (shouldCache && !cached) {
        size_t pos = 0;
        MessageBuffer::Reader reader = response->read(pos);
        http::StatusLine line;

        if (http::StatusLine::parse(reader, line)) {
            shouldCache = false;
            if (line.code == http::ResponseCode::Ok) {
                cached = cache.addEntry(uri, response);
            }
        }
    }

    MessageBuffer::Reader reader = request->read(requestReadPos);
    if (reader.length() > 0) {
        if ((events & SocketEvents::Write) != SocketEvents::None) {
            reader.advance(serverSocket->write(reader.data(), reader.length()));
        }
        serverSocket->await(waker, SocketEvents::Read | SocketEvents::Write);
    } else {
        // We don't have any new request data. Wait.
        serverSocket->await(waker, SocketEvents::Read);
        reader.await(waker);
    }

    return false;
}
