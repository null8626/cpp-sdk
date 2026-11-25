#pragma once

#include <curl/curl.h>

#include <stdexcept>

namespace topgg {
/**
 * @brief The base error class.
 *
 * @since 2.0.0
 */
class exception : public std::runtime_error {
 protected:
  inline exception(const char* message) : std::runtime_error(message) {}

  inline exception(const std::string& message) : std::runtime_error(message) {}

  static void assert_curle(const CURLcode code, char* details);

  static void assert_curlm(const CURLMcode code);

  static exception ssl(const std::string& message);

 public:
  exception() = delete;

  friend time_t _from_time_string(const std::string& timestamp);
  friend std::string _random_string();
  friend webhooks::payload webhooks::parse(const std::string& body);
  friend class http_client;  // OK
  friend class http_request;
  friend class locale_map;
  friend class oauth2;
  friend class webhooks::verifier;
};

struct http_response;

/**
 * @brief Thrown upon HTTP request failure.
 *
 * @since 2.0.0
 */
class request_exception : public exception {
  inline request_exception(const uint16_t status_, const std::string& message)
      : exception(message), status(status_) {}

  static request_exception parse(const http_response& response);

 public:
  /**
   * @brief The URI identifying the error type.
   *
   * @since 2.0.0
   */
  std::optional<std::string> type{};

  /**
   * @brief The error's short human-readable summary.
   *
   * @since 2.0.0
   */
  std::optional<std::string> title{};

  /**
   * @brief The error's HTTP status code.
   *
   * @since 2.0.0
   */
  uint16_t status{};

  /**
   * @brief The human-readable explanation of this specific occurrence.
   *
   * @since 2.0.0
   */
  std::optional<std::string> detail{};

  /**
   * @brief The error's code (e.g. "PROJECT_NOT_FOUND".)
   *
   * @since 2.0.0
   */
  std::optional<std::string> code{};

  request_exception() = delete;

  friend class base_client;  // OK
  friend class base_application_client;
  friend class oauth2_client;
  friend class oauth2_http_provider;
  friend class oauth2_url;
  friend struct project_client;
};
};  // namespace topgg