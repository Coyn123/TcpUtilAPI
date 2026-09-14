#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "transport/BufferedReader.h"

constexpr size_t kMaxRequestLineBytes = 8192;
constexpr std::string_view kRFCguid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

struct HttpResponse {
    int status_code = 0;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpRequest {
    std::string url;
    std::string method;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string version;
    bool is_ws_upgrade = false;

};

namespace Http {
    enum class ParseError : int {
        RequestLineTooLong, // hit byte_max before finding \r\n on the request line
        RequestLineMalformed, // request line didn't split into exactly 3 tokens
        HeaderLineTooLong, // hit byte_max before finding \r\n on a header line
        HeaderMissingColon, // header line has no ':' to split on
        DuplicateHeader, // same header key seen twice
        ContentLengthEmpty, // Content-Length value is an empty string
        ContentLengthTooLong, // Content-Length value has more digits than can fit unsigned long
        ContentLengthInvalid, // Content-Length value contains a non-digit character
        BodyTruncated, // connection closed before Content-Length bytes arrived
    };

    tcp::Result<HttpRequest> build_request(BufferedReader& reader);
    std::string serialize_response(const HttpResponse& response);
    tcp::Result<HttpResponse> build_response(const HttpRequest&);
    tcp::Result<std::string> route(const std::string& url);

};
