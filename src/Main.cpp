#include <cstdio>
#include <cstring>
#include "api/TcpUtil.h"

int main() {
    TcpUtil server(8080, make_http_task);

    tcp::Result<void> result = server.run();
    if (!result) {
        fprintf(stderr, "server failed: %s\n", strerror(result.error()));
        return 1;
    }

    return 0;
}
