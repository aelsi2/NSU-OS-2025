
#include "Connection.h"
#include "Socket.h"
#include <unordered_map>

using namespace proxy;

class Proxy : SocketManager, ConnectionManager {
  public:
    void setPollEvents(Socket &socket, SocketEvents actions) override;
    void close(Socket &socket) override;
    std::shared_ptr<ReadBuffer>
    connect(std::string hostname, uint16_t port,
            std::shared_ptr<ReadBuffer> requestBuffer) override;

  private:
};

void Proxy::setPollEvents(Socket &socket, SocketEvents actions) {
}

void Proxy::close(Socket &socket) {
}

std::shared_ptr<ReadBuffer> connect(std::string hostname, uint16_t port,
                                    std::shared_ptr<ReadBuffer> requestBuffer) {
}

int main(int argc, char *argv[]) {
    return 0;
}
