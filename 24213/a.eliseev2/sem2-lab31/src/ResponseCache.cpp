#include "ResponseCache.h"

namespace proxy {

ResponseCache::ResponseCache() : map() {
}

std::shared_ptr<MessageBuffer> ResponseCache::getEntry(http::Uri &uri) {
    auto it = map.find(uri.raw);
    if (it == map.end()) {
        return nullptr;
    }
    time_t time = ::time(nullptr);
    return it->second.buffer;
}

bool ResponseCache::addEntry(http::Uri &uri,
                             std::shared_ptr<MessageBuffer> buffer) {
    if (map.find(uri.raw) != map.end()) {
        return false;
    }
    map[uri.raw] = {
        .buffer = std::move(buffer),
    };
    return true;
}

void ResponseCache::removeEntry(http::Uri &uri) {
    map.erase(uri.raw);
}

} // namespace proxy
