//An attempt at making the utility platform agnostic
#pragma once
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
  inline constexpr socket_t kInvalidSocket = INVALID_SOCKET;
  inline constexpr int kFdExhausted = WSAEMFILE;
  inline constexpr int kSysExhausted = WSAENFILE;
  inline constexpr int kListenerBadFd = WSAEBADF;
  inline constexpr int kListenerNotSocket = WSAENOTSOCK;
  inline constexpr int kListenerOpNotSupported = WSAEOPNOTSUPP;
  inline constexpr int kListenerInvalid = WSAEINVAL;
  inline int close_socket(socket_t s) { return ::closesocket(s); }
  inline int last_error() { return ::WSAGetLastError(); }
  inline int last_file_error() { return static_cast<int>(::GetLastError()); }
  inline int set_reuseaddr(socket_t s) {
      char yes = 1;
      return ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  }
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <cerrno>
  using socket_t = int;
  inline constexpr socket_t kInvalidSocket = -1;
  inline constexpr int kFdExhausted = EMFILE;
  inline constexpr int kSysExhausted = ENFILE;
  inline constexpr int kListenerBadFd = EBADF;
  inline constexpr int kListenerNotSocket = ENOTSOCK;
  inline constexpr int kListenerOpNotSupported = EOPNOTSUPP;
  inline constexpr int kListenerInvalid = EINVAL;
  inline int close_socket(socket_t s) { return ::close(s); }
  inline int last_error() { return errno; }
  inline int last_file_error() { return errno; }
  inline int set_reuseaddr(socket_t s) {
      int yes = 1;
      return ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  }
#endif

#ifdef SO_NOSIGPIPE
  inline int set_macopt(socket_t s) {
      int yes = 1;
      return ::setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
  }
#else
  inline void set_macopt(socket_t s) {
      return;
  }
#endif

#ifdef MSG_NOSIGNAL
  inline constexpr int kNoSignalFlag = MSG_NOSIGNAL;
#else
  inline constexpr int kNoSignalFlag = 0;
#endif


namespace tcp {
    class PlatformInit {
        public:
            PlatformInit() {
                #ifdef _WIN32
                    WSADATA d;
                    WSAStartup(MAKEWORD(2, 2), &d);
                #endif
            }
            ~PlatformInit() {
                #ifdef _WIN32
                    WSACleanup();
                #endif
            }

            PlatformInit(const PlatformInit&) = delete;
            PlatformInit& operator=(const PlatformInit&) = delete;
    };

    inline void ensure_started() { static PlatformInit once; }
}
