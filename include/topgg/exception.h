#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>


namespace topgg {
  class http_backend;
  class http_exception;
  class http_frontend;
  class waker;
#ifdef _WIN32
  class wsastartup_guard;
#endif

  class exception: public std::runtime_error {
    inline exception(const char* message): std::runtime_error(message) {}

    static exception ssl(const char* message);

    static exception uv(const char* message, const int status);

    static exception system(const char* message);

    static exception nghttp2(const char* message, const int status);

  public:
    std::string cause{};

    exception() = delete;

    friend class http_backend;
    friend class http_exception;
    friend class http_frontend;
    friend class waker;
#ifdef _WIN32
    friend class wsastartup_guard;
#endif
  };
  
  class http_exception: public exception {
    http_exception(const std::pair<uint16_t, std::string_view>& response_pair);

  public:
    uint16_t status{};
    std::string type{};
    std::string detail{};

    http_exception() = delete;

    friend class client;
    friend class http_backend;
  };
};