#include "HttpParser.h"
#include "core/Platform.h"
#include "transport/Connection.h"
#include <functional>
#include <sstream>
#include <cctype>
#include <limits>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <iterator>
#include <algorithm>

tcp::Result<std::string> Http::route(const std::string& url) {
    std::error_code ec;

    std::string stripped = url.substr(1);
    std::string candidate = std::filesystem::path("templates").string() + "/" + stripped;
    std::filesystem::path canon_path = std::filesystem::weakly_canonical(candidate, ec);

    if(ec) {
        return tcp::Result<std::string>::err(ec.value());
    }

    std::filesystem::path canon_root = std::filesystem::weakly_canonical("templates", ec);
    if(ec) {
        return tcp::Result<std::string>::err(ec.value());
    }

    std::filesystem::path is_relative = std::filesystem::relative(canon_path, canon_root);
    if(is_relative.string().substr(0, 2) == ".." || is_relative.string().size() == 0) {
        //Not relative.
        return tcp::Result<std::string>::err(-1);
    }

    return is_relative.string() == "."
        ? tcp::Result<std::string>::ok("templates/index.html")
            : tcp::Result<std::string>::ok("templates/" + is_relative.string());
}

tcp::Result<HttpResponse> Http::build_response(const HttpRequest& in) {

    tcp::Result<std::string> path = route(in.url);

    static const std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".txt", "text/plain"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".webp", "image/webp"},
        {".pdf", "application/pdf"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"}
    };

    if(!path) {
        return tcp::Result<HttpResponse>::ok(HttpResponse{404, {{"Content-Length", std::to_string(0)}}, ""});
    };

    std::string extension = std::filesystem::path(path.value()).extension().string();

    //Lowercase normalize the extension (JIC)
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    auto it = mime_types.find(extension);
    std::string mime = (it != mime_types.end()) ? it->second : "application/octet-stream";

    if( std::filesystem::is_regular_file(path.value()) ) {
        std::ifstream file(path.value(), std::ios::binary);
        if(!file) return tcp::Result<HttpResponse>::err(last_file_error());
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string body = buffer.str();
        return tcp::Result<HttpResponse>::ok(HttpResponse{200, {{"Content-Length", std::to_string(body.size())}, {"Content-Type", mime}}, body});

    } else {
        return tcp::Result<HttpResponse>::ok(HttpResponse{404, {{"Content-Length", std::to_string(0)}}, ""});
    }

}

std::string Http::serialize_response(const HttpResponse& response) {
    static const std::unordered_map<int, std::string> reasons = {
        {200, "OK"},
        {404, "Not Found"},
        {101, "Switching Protocols"},
        {400, "Bad Request"},
        {500, "Internal Server Error"}
    };
    auto it = reasons.find(response.status_code);
    std::string reason = (it != reasons.end()) ? it->second : "Unknown";

    std::string ret = "HTTP/1.1 " + std::to_string(response.status_code) + " " + reason + "\r\n";
    for (const auto& [key,value] : response.headers) {
        ret += key + ": ";
        ret += value + "\r\n";
    }
    ret += "\r\n" + response.body;
    return ret;
}

tcp::Result<HttpRequest> Http::build_request(BufferedReader& reader) {

    HttpRequest ret;
    tcp::Result<tcp::BufferedResult> try_read = reader.read_until("\r\n", kMaxRequestLineBytes);

    if (!try_read) {
        return tcp::Result<HttpRequest>::err(try_read.error());
    } else if (!try_read.value().complete) {
        return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::RequestLineTooLong));
    }
    std::string entire_request = try_read.value().bytes;
    std::stringstream ss(entire_request);

    std::vector<std::string> split_request((std::istream_iterator<std::string>(ss)), std::istream_iterator<std::string>());
    if (split_request.size() != 3) {
        return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::RequestLineMalformed));
    }

    ret.method = split_request[0];
    ret.url = split_request[1];
    ret.version = split_request[2];

    //Headers loop start
    for(;;) {
        tcp::Result<tcp::BufferedResult> try_headers = reader.read_until("\r\n", kMaxRequestLineBytes);
        if(!try_headers) {
            return tcp::Result<HttpRequest>::err(try_headers.error());
        } else if (!try_headers.value().complete) {
            return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::HeaderLineTooLong));
        }
        if (try_headers.value().bytes == "\r\n") break;

        //Find
        std::string cur = try_headers.value().bytes;
        size_t splitter = cur.find(":");
        if (splitter == std::string::npos) return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::HeaderMissingColon));
        size_t start = cur.find_first_not_of(" ", splitter + 1);
        std::string value = cur.substr(start, (cur.size()-2) - start);
        std::string key = cur.substr(0, splitter);

        //Lowercase normalize the key
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
            return std::tolower(c);
        });


        //Place the keys
        auto existing = ret.headers.find(key);
        if (existing != ret.headers.end()) {
            //Content-Length / Host duplicates are dangerous, every other header can be merged
            if (key == "content-length" || key == "host")
                return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::DuplicateHeader));
            existing->second += ", " + value;
        } else {
            ret.headers[key] = value;
        }

    } //Headers loop end

    auto ws1 = ret.headers.find("upgrade");
    auto ws2 = ret.headers.find("connection");
    auto ws3 = ret.headers.find("sec-websocket-key");
    auto wsCheck = ret.headers.end();
    if(ws1 != wsCheck && ws2 != wsCheck && ws3 != wsCheck) {
        if (ws1->second == "websocket" && ws2->second.find("Upgrade") != std::string::npos) {
            ret.is_ws_upgrade = true;
        }
    }


    if(ret.is_ws_upgrade) return tcp::Result<HttpRequest>::ok(ret);

    auto check_length = ret.headers.find("content-length");

    //If we have a Content-Length at all (start)
    if (check_length != ret.headers.end() ) {

        std::string clen = check_length->second;

        if(clen.size() == 0) return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::ContentLengthEmpty));
        if(clen.size() > std::numeric_limits<unsigned long>::digits10) {
            return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::ContentLengthTooLong));
        }

        for (const char& c : clen) {
            if(!std::isdigit(static_cast<unsigned char>(c))) {
                return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::ContentLengthInvalid));
            }
        }
        unsigned long rclen = std::stoul(clen, nullptr, 10);

        //Is the real content length still 0?
        if(rclen != 0) {
            tcp::Result<tcp::BufferedResult> try_body = reader.read_exact(rclen);

            if(!try_body) {
                return tcp::Result<HttpRequest>::err(try_body.error());
            } else if (!try_body.value().complete) {
                return tcp::Result<HttpRequest>::err(static_cast<int>(Http::ParseError::BodyTruncated));
            }
            ret.body = try_body.value().bytes;
        }

    } // If we have a Content-Length at all (end)

    //Finally, a finished request!
    return tcp::Result<HttpRequest>::ok(ret);

}
