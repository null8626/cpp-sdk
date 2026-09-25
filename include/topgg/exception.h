#pragma once

#include <stdexcept>
#include <string>


namespace topgg {
  class http_backend;
  class http_frontend;
  class waker;
#ifdef _WIN32
  class wsastartup_guard;
#endif

  class exception: public std::runtime_error {
    std::string m_cause{""};

    inline exception(const char* message): std::runtime_error(message) {}

    static exception ssl(const char* message);

    static exception uv(const char* message, const int status);

    static exception system(const char* message);

    static exception nghttp2(const char* message, const int status);

  public:
    exception() = delete;

    inline constexpr const std::string& cause() const noexcept {
      return m_cause;
    }

    friend class http_backend;
    friend class http_frontend;
    friend class waker;
#ifdef _WIN32
    friend class wsastartup_guard;
#endif
  };
};