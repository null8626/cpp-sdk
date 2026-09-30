#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <ctime>


namespace topgg {
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
  class client;
#endif
  class http_backend;
  class http_exception;
  class http_frontend;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  class oauth2_client;
#endif
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

    friend time_t _from_time_string(const std::string& timestamp);
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend std::string _random_string();
#endif
    friend class base_client;
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
    friend class client;
#endif
    friend class http_backend;
    friend class http_exception;
    friend class http_frontend;
    friend class localized_string;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
    friend class oauth2;
#endif
    friend class waker;
#ifdef _WIN32
    friend class wsastartup_guard;
#endif
  };

  class http_exception: public exception {
    http_exception(const std::pair<uint16_t, std::string_view>& response_pair);

  public:
    uint16_t status{};
    std::string detail{};

    http_exception() = delete;

    friend class base_client;
    friend class http_backend;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
    friend class oauth2;
#endif
  };
};