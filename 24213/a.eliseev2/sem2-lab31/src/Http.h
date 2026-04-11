#pragma once

#include "MessageBuffer.h"
#include <cstdint>
#include <string>
#include <vector>

namespace proxy {
namespace http {

enum class ResponseCode {
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NoContent = 204,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    NotModified = 304,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
};

enum class Version {
    Http10,
    Unknown,
};

enum class RequestMethod {
    Get,
    Head,
    Post,
};

struct Uri {
    static bool parse(MessageBuffer::Reader &reader, Uri &uri);

    std::string raw;
    std::string host;
    uint16_t port;
};

struct RequestLine {
    static bool parse(MessageBuffer::Reader &reader, RequestLine &line);

    RequestMethod method;
    Uri uri;
    Version version;
};

struct StatusLine {
    static bool parse(MessageBuffer::Reader &reader, StatusLine &line);

    Version version;
    ResponseCode code;
};

} // namespace http
} // namespace proxy
