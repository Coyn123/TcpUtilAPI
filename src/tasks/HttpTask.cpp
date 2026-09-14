#include "HttpTask.h"
#include "http/HttpParser.h"
#include "util/Sha1.h"
#include "util/Base64.h"
#include "tasks/WSTask.h"
#include <unordered_map>
#include <utility>

HttpTask::HttpTask(Connection conn) : connection_(std::move(conn)) {}

void HttpTask::run_task() {
    printf("client connected\n");
    BufferedReader reader(connection_);

    tcp::Result<HttpRequest> request = Http::build_request(reader);
    if (!request) {
        HttpResponse bad_build;
        bad_build.status_code = 400;
        bad_build.headers = {{"Content-Length","0"}};
        bad_build.body = "";
        std::string serial = Http::serialize_response(bad_build);

        tcp::Result<void> try_write_build = connection_.write_all(serial.data(), serial.size());

        if(!try_write_build) {
            fprintf(stderr, "write http build response failed: code %d\n", try_write_build.error());
            return;
        }

        fprintf(stderr, "build_request failed: code %d\n", request.error());
        return;
    }
    const HttpRequest& req = request.value();

    //Websocket upgrade path start
    if(req.is_ws_upgrade) {
        std::string socket_key = req.headers.at("sec-websocket-key");
        socket_key += kRFCguid;

        auto digest = sha1(reinterpret_cast<const unsigned char*>(socket_key.data()), socket_key.size());

        std::string accept_value = base64_encode(digest.data(), digest.size());
        std::unordered_map<std::string, std::string> ws_headers;

        ws_headers["Upgrade"] = "websocket";
        ws_headers["Connection"] = "upgrade";
        ws_headers["Sec-WebSocket-Accept"] = accept_value;

        HttpResponse resp;
        resp.status_code = 101;
        resp.headers = ws_headers;
        resp.body = "";
        std::string bytes = Http::serialize_response(resp);

        tcp::Result<void> try_write = connection_.write_all(bytes.data(), bytes.size());

        if(!try_write) {
            fprintf(stderr, "serialize web-socket upgrade failed: code %d\n", try_write.error());
            return;
        }
        WSTask(std::move(connection_)).run_task(); return;
    } //Websocket upgrade path end

    printf("%s %s %s\n",req.method.c_str(), req.url.c_str(), req.version.c_str());

    tcp::Result<HttpResponse> response = Http::build_response(req);
    if (!response) {
        HttpResponse bad_response;
        bad_response.status_code = 500;
        bad_response.headers = {{"Content-Length","0"}};
        bad_response.body = "";
        std::string serial_resp = Http::serialize_response(bad_response);

        tcp::Result<void> try_write_resp = connection_.write_all(serial_resp.data(), serial_resp.size());

        if(!try_write_resp) {
            fprintf(stderr, "write http build response failed: code %d\n", try_write_resp.error());
            return;
        }

        fprintf(stderr, "http build_response failed: code %d\n", response.error());
        return;
    }

    std::string bytes = Http::serialize_response(response.value());

    tcp::Result<void> sent = connection_.write_all(bytes.data(), bytes.size());
    if (!sent) {
        fprintf(stderr, "http write_all failed: %s\n", strerror(sent.error()));
        return;
    }
    printf("sent %zu bytes (status %d)\n", bytes.size(), response.value().status_code);
}
