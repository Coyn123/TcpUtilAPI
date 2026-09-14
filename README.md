# TcpUtilAPI

A cross-platform C++20 TCP sockets library — RAII ownership, `Result<T>` error handling instead of exceptions, no dependencies. Includes a full HTTP + WebSocket server built on top of it.

## What's in here

- **Transport core**: `Listener` → `Connection` → `IStream`/`Stream` → `BufferedReader`. RAII-wrapped sockets, cross-platform (BSD sockets / Winsock) behind a single `Platform.h` shim.
- **HTTP**: request/response parsing, MIME-typed static file serving, path-traversal-safe routing (canonicalize + prefix-check against the served root, not a string blacklist), 400/500 error responses.
- **WebSocket**: full RFC 6455 handshake — hand-written SHA-1 and Base64, no OpenSSL or Boost — verified against the RFC's own test vector. Full frame parsing/serialization (masking, all three payload-length encodings, fragmentation) and an echo loop.
- **`TcpUtil`**: the harness tying it together. `TcpUtil(port, factory).run()` — pass a factory function, get a server. Thread-per-connection concurrency gated by a configurable semaphore cap (default 6), backoff-and-retry on transient `accept()` failures (`EMFILE`/`ENFILE`) instead of busy-looping, and a clean shutdown path when the listening socket is genuinely dead (`EBADF`/`ENOTSOCK`/`EOPNOTSUPP`/`EINVAL`).

## Example

```cpp
#include "api/TcpUtil.h"

int main() {
    TcpUtil server(8080, make_http_task);
    return server.run() ? 0 : 1;
}
```

Swap `make_http_task` for your own `Connection -> unique_ptr<TaskBase>` factory to serve anything else over the same harness.

## Building

No build system yet — compile directly:

```bash
g++ -std=c++20 -Isrc src/Main.cpp src/transport/*.cpp src/http/*.cpp src/tasks/*.cpp src/websocket/*.cpp src/util/*.cpp src/api/*.cpp -o server -pthread
```

On Windows (MinGW), swap `-pthread` for `-lws2_32`.

## Status

HTTP + WebSocket serving is complete and tested end-to-end (verified against real clients, not just unit-level). Next: an HTTPS layer via a `TlsStream : IStream` — drops in without touching HTTP/WS code, since everything above the transport layer is composed over `IStream` rather than a concrete socket. Longer-term: FTP and SSH support through the same `TcpUtil` + task-factory pattern.
