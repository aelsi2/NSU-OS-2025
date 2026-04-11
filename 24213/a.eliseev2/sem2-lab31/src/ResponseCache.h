#pragma once

#include "Http.h"
#include "MessageBuffer.h"
#include <ctime>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace proxy {

class ResponseCache {
  public:
    ResponseCache();

    std::shared_ptr<MessageBuffer> getEntry(http::Uri &uri);
    bool addEntry(http::Uri &uri, std::shared_ptr<MessageBuffer> buffer);
    void removeEntry(http::Uri &uri);

  private:
    struct Response {
        std::shared_ptr<MessageBuffer> buffer;
    };

    std::unordered_map<std::string, Response> map;
};

} // namespace proxy
