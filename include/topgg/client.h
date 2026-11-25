#pragma once

#include <topgg/topgg.h>

#include <BS_thread_pool.hpp>
#include <memory>
#include <shared_mutex>

#ifndef TOPGG_BASE_URL
#define TOPGG_BASE_URL "https://top.gg/api/v1"
#endif

#define TOPGG_CT_APPLICATION_JSON "Content-Type: application/json"
#define TOPGG_CT_APPLICATION_X_WWW_FORM_URLENCODED \
  "Content-Type: application/x-www-form-urlencoded"

#define _TOPGG_UNWRAP_VARIANT(callback, response_variant, ...)              \
  if (std::holds_alternative<topgg::request_exception>(response_variant)) { \
    callback(std::get<topgg::request_exception>(response_variant));         \
  } else if (std::holds_alternative<topgg::exception>(response_variant)) {  \
    callback(std::get<topgg::exception>(response_variant));                 \
  } else if (std::holds_alternative<nlohmann::json::exception>(             \
                 response_variant)) {                                       \
    callback(std::get<nlohmann::json::exception>(response_variant));        \
  } else                                                                    \
    __VA_ARGS__

namespace topgg {
/**
 * @brief An API call callback function that receives a topgg::result<T>.
 *
 * @since 2.0.0
 */
template <class T>
using callback = std::function<void(const result<T>&)>;

/**
 * @brief An API call callback function that receives a topgg::empty_result.
 *
 * @since 2.0.0
 */
using empty_callback = std::function<void(const empty_result&)>;

/**
 * @brief An API call callback function that receives a
 * topgg::paginated_result<T>.
 *
 * @since 2.0.0
 */
template <class T>
using paginated_callback = std::function<void(const paginated_result<T>&)>;

struct http_response {
  uint16_t status{};
  std::optional<uint32_t> retry_after{std::nullopt};
  std::string_view body{};
};

using http_response_variant =
    std::variant<exception, request_exception, nlohmann::json::exception,
                 http_response>;

class http_request : private std::enable_shared_from_this<http_request> {
  using callback = std::function<void(const http_response_variant&)>;

  CURL* m_curl{nullptr};
  std::string m_url{};
  http_request::callback m_callback{};
  std::string m_authorization{};
  std::string m_request_body{};
  struct curl_slist* m_headers{nullptr};

  char m_curl_error_details[CURL_ERROR_SIZE];
  uint16_t m_response_status{};
  std::optional<uint32_t> m_retry_after{};
  std::string m_response_body{};

  http_request(const char* method, const std::string& path,
               const http_request::callback& callback,
               const char* content_type = TOPGG_CT_APPLICATION_JSON,
               const std::string& body = "");

  void unref();

  void set_token(const std::string& token);

  static size_t write_response_headers(char* ptr, size_t size, size_t nmemb,
                                       void* userdata);

  static size_t write_response_body(char* ptr, size_t size, size_t nmemb,
                                    void* userdata);

  void dispatch();

  inline void dispatch_variant(const http_response_variant& variant) {
    m_callback(variant);
  }

 public:
  http_request() = delete;

  ~http_request();

  friend class base_client;
  friend class base_application_client;
  friend class http_client;  // OK
  friend class oauth2_client;
  friend class oauth2_http_provider;
  friend class oauth2_url;
  friend struct project_client;
};

class http_client {
  std::atomic<bool> m_running{};
  CURLM* m_client{nullptr};
  std::thread m_event_loop_thread{};
  std::mutex m_pending_requests_mutex{};
  std::vector<std::shared_ptr<http_request>> m_pending_requests{};
  std::vector<std::shared_ptr<http_request>> m_completed_requests{};
  BS::thread_pool<> m_callback_thread_pool{};

  http_client();

  void event_loop();

  void submit(const std::shared_ptr<http_request>& request);

  void safe_submit(const std::shared_ptr<http_request>& request);

  template <class T, class C, class A>
  inline void dispatch(const C& callback, const A& args) {
    m_callback_thread_pool.detach_task(
        [callback, args]() { callback(T{args}); });
  }

 public:
  ~http_client();

  friend class base_client;
  friend class base_application_client;
  friend class direct_http_provider;  // OK
  friend class oauth2;
  friend class oauth2_client;
  friend class oauth2_http_provider;
  friend class oauth2_url;
  friend struct project_client;
};

class http_provider {
 protected:
  virtual void ensure_session(const empty_callback& callback) = 0;

  virtual std::string get_token() = 0;

  virtual http_client* get_http() = 0;

  friend class base_client;
  friend class base_application_client;
};

class direct_http_provider : protected http_provider {
  http_client m_http{};
  std::string m_token{};

  void ensure_session(const empty_callback& callback_) override;

  inline std::string get_token() override { return m_token; }

  inline http_client* get_http() override { return &m_http; }

 protected:
  inline direct_http_provider(const std::string& token) : m_token(token) {}

 public:
  direct_http_provider() = delete;

  friend struct project_client;
};

class base_client {
  http_provider* m_provider{nullptr};

  void fetch_json(const std::shared_ptr<http_request>& request);

 protected:
  inline base_client(http_provider* provider) : m_provider(provider) {}

  template <class T>
  bool enforce_constraints(const locale_map& map, const uint16_t minimum,
                           const uint16_t maximum,
                           const std::function<T>& callback) {
    const auto result{map.enforce_constraints(minimum, maximum)};

    if (result < 0) {
      m_provider->get_http()->dispatch<topgg::exception>(
          callback, "A locale_map text length is below the minimum threshold");

      return false;
    } else if (result > 0) {
      m_provider->get_http()->dispatch<topgg::exception>(
          callback, "A locale_map text length is above the maximum threshold");

      return false;
    }

    return true;
  }

  template <class T>
  void fetch_simple(const char* method, const std::string& path,
                    const callback<T>& callback_,
                    const std::string& body = "") {
    fetch_json(std::shared_ptr<http_request>{new http_request{
        method, path,
        [callback_](const http_response_variant& response_variant) {
          _TOPGG_UNWRAP_VARIANT(callback_, response_variant, {
            try {
              const auto& response{std::get<http_response>(response_variant)};

              if (response.status >= 400) {
                callback_(request_exception::parse(response));
              } else {
                callback_(T{nlohmann::json::parse(response.body)});
              }
            } catch (const nlohmann::json::exception& error) {
              callback_(error);
            }
          });
        },
        TOPGG_CT_APPLICATION_JSON, body}});
  }

  template <class T>
  void fetch_paginated(const char* key, const char* method,
                       const std::string& path,
                       const paginated_callback<T>& callback) {
    fetch_json(std::shared_ptr<http_request>{new http_request{
        method, path,
        [key, callback](const http_response_variant& response_variant) {
          _TOPGG_UNWRAP_VARIANT(callback, response_variant, {
            try {
              const auto& response{std::get<http_response>(response_variant)};

              if (response.status >= 400) {
                callback(request_exception::parse(response));
              } else {
                callback(paginated_result<T>::from_array(
                    key, nlohmann::json::parse(response.body)));
              }
            } catch (const nlohmann::json::exception& error) {
              callback(error);
            }
          });
        }}});
  }

  template <class T>
  void fetch_paginated(const paginated_result<T>& cursor_, const char* key,
                       const char* method, const std::string& path,
                       const paginated_callback<T>& callback) {
    if (cursor_.is_last()) {
      fetch_paginated(key, method, path + "?cursor=" + cursor_.cursor(),
                      callback);
    } else {
      callback(paginated_result<T>::empty());
    }
  }

  void fetch_empty(const char* method, const std::string& path,
                   const empty_callback& callback,
                   const std::string& body = "");

  template <class T>
  void fetch_vector(const char* method, const std::string& path,
                    const callback<std::vector<T>>& callback,
                    const std::string& body = "") {
    fetch_json(std::shared_ptr<http_request>{new http_request{
        method, path,
        [callback](const http_response_variant& response_variant) {
          _TOPGG_UNWRAP_VARIANT(callback, response_variant, {
            try {
              const auto& response{std::get<http_response>(response_variant)};

              if (response.status >= 400) {
                callback(request_exception::parse(response));
              } else {
                const auto json{nlohmann::json::parse(response.body)};
                std::vector<T> output{};

                for (const auto& project : json) {
                  output.push_back(T{project});
                }

                callback(output);
              }
            } catch (const nlohmann::json::exception& error) {
              callback(error);
            }
          });
        },
        TOPGG_CT_APPLICATION_JSON, body}});
  }

 public:
  base_client() = delete;

  friend class base_application_client;
  friend class oauth2_client;
  friend struct project_client;
};

/**
 * @brief An API client using a project token.
 *
 * @since 2.0.0
 */
struct project_client : private direct_http_provider, private base_client {
  project_client() = delete;

  /**
   * @brief Creates the client.
   *
   * @param token The project token to use.
   * @since 2.0.0
   */
  inline project_client(const std::string& token)
      : direct_http_provider(token),
        base_client(static_cast<http_provider*>(this)) {}

  /**
   * @brief Fetches the project associated with the current token.
   *
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_project(const callback<project>& callback);

  /**
   * @brief Updates the headline and/or page content for the current project.
   * Both fields are locale-keyed, so you can set content for multiple languages
   * in a single request.
   *
   * @param headline A map of locales to headline strings. Each headline must be
   * between 3 and 140 characters.
   * @param page_content A map of locales to page content strings (Markdown
   * supported.) Each entry must be between 300 and 50,000 characters.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void edit_project(const locale_map& headline, const locale_map& page_content,
                    const empty_callback& callback);

  /**
   * @brief Creates a new announcement for the current project. Announcements
   * appear on a project’s page and can be used to notify users about updates,
   * new features, or other news.
   *
   * @param title The announcement's title.
   * @param content The announcement's body.
   * @param category The category to publish the announcement under.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void post_announcement(const std::string& title, const std::string& content,
                         const announcement_category category,
                         const empty_callback& callback);

  /**
   * @brief Submits a single metrics payload for the current project. Use this
   * to push fresh numbers after an event such as joining or leaving a guild or
   * a player connecting.
   *
   * @param metrics The metrics payload.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void post_metrics(const metrics& metrics, const empty_callback& callback);

  /**
   * @brief Submits up to 100 metrics entries in a single request.
   *
   * @param metrics_ The metrics payloads.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  template <class T>
  void post_metrics(const timestamped_metrics<T>& metrics_,
                    const empty_callback& callback) {
    if (metrics_.m_json.size() < 1 || metrics_.m_json.size() > 100) {
      m_http.dispatch<topgg::exception>(
          callback, "Batch length is outside of the accepted threshold");

      return;
    }

    nlohmann::json body{};

    body["data"] = metrics_.m_json;

    fetch_empty("POST", "/projects/@me/metrics/batch", callback, body.dump());
  }

  /**
   * @brief Overwrites the list of slash command definitions for the current bot
   * project on Top.gg. This is only applicable to bot-type projects on the
   * discord platform.
   *
   * @param commands The array of slash commands in a JSON-stringified Discord
   * API's application command structure format.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void post_commands(const std::string& commands,
                     const empty_callback& callback);

  /**
   * @brief Fetches a cursor-paginated list of votes for the current project,
   * ordered by creation date (oldest first within each page.)
   *
   * @param since The creation date frame of reference.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_votes(const time_t since, const paginated_callback<vote>& callback);

  /**
   * @brief Fetches a cursor-paginated list of votes for the current project
   * from a cursor.
   *
   * @param cursor The cursor frame of reference.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_votes(const paginated_result<vote>& cursor,
                 const paginated_callback<vote>& callback);

  /**
   * @brief Fetches the most recent vote status for a specific user. Use this to
   * check whether a user has voted before granting in-app rewards or unlocking
   * features.
   *
   * @param user_id The ID of the user to look up. The expected format depends
   * on the source parameter.
   * @param source The ID type being provided.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_vote(const std::string& user_id, const user_source& source,
                const callback<std::optional<partial_vote>>& callback);
};

/**
 * @brief A base client for application and oauth2 access tokens.
 *
 * @since 2.0.0
 */
class base_application_client : private base_client {
 protected:
  inline base_application_client(http_provider* provider)
      : base_client(provider) {}

 public:
  base_application_client() = delete;

  /**
   * @brief Fetches the projects the current credential covers.
   *
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_projects(const paginated_callback<partial_project>& callback);

  /**
   * @brief Fetches the project associated with the current token.
   *
   * @param id The project's Top.gg ID.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_project(const uint64_t id, const callback<project>& callback);

  /**
   * @brief Updates the headline and/or page content for the current project.
   * Both fields are locale-keyed, so you can set content for multiple languages
   * in a single request.
   *
   * @param id The project's Top.gg ID.
   * @param headline A map of locales to headline strings. Each headline must be
   * between 3 and 140 characters.
   * @param page_content A map of locales to page content strings (Markdown
   * supported.) Each entry must be between 300 and 50,000 characters.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void edit_project(const uint64_t id, const locale_map& headline,
                    const locale_map& page_content,
                    const empty_callback& callback);

  /**
   * @brief Creates a new announcement for the current project. Announcements
   * appear on a project’s page and can be used to notify users about updates,
   * new features, or other news.
   *
   * @param id The project's Top.gg ID.
   * @param title The announcement's title.
   * @param content The announcement's body.
   * @param category The category to publish the announcement under.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void post_announcement(const uint64_t id, const std::string& title,
                         const std::string& content,
                         const announcement_category category,
                         const empty_callback& callback);

  /**
   * @brief Submits a single metrics payload for the current project. Use this
   * to push fresh numbers after an event such as joining or leaving a guild or
   * a player connecting.
   *
   * @param id The project's Top.gg ID.
   * @param metrics The metrics payload.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void post_metrics(const uint64_t id, const metrics& metrics,
                    const empty_callback& callback);

  /**
   * @brief Submits up to 100 metrics entries in a single request.
   *
   * @param id The project's Top.gg ID.
   * @param metrics_ The metrics payloads.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  template <class T>
  inline void post_metrics(const uint64_t id,
                           const timestamped_metrics<T>& metrics_,
                           const empty_callback& callback) {
    if (metrics_.m_json.size() < 1 || metrics_.m_json.size() > 100) {
      m_provider->get_http()->dispatch<topgg::exception>(
          callback, "Batch length is outside of the accepted threshold");

      return;
    }

    nlohmann::json body{};

    body["data"] = metrics_.m_json;

    fetch_empty("POST", "/projects/" + std::to_string(id) + "/metrics/batch",
                callback, body.dump());
  }

  /**
   * @brief Overwrites the list of slash command definitions for the current bot
   * project on Top.gg. This is only applicable to bot-type projects on the
   * discord platform.
   *
   * @param id The project's Top.gg ID.
   * @param commands The array of slash commands in a JSON-stringified Discord
   * API's application command structure format.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void post_commands(const uint64_t id, const std::string& commands,
                     const empty_callback& callback);

  /**
   * @brief Fetches a cursor-paginated list of votes for the current project,
   * ordered by creation date (oldest first within each page.)
   *
   * @param id The project's Top.gg ID.
   * @param since The creation date frame of reference.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_votes(const uint64_t id, const time_t since,
                 const paginated_callback<vote>& callback);

  /**
   * @brief Fetches a cursor-paginated list of votes for the current project
   * from a cursor.
   *
   * @param id The project's Top.gg ID.
   * @param cursor The cursor frame of reference.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_votes(const uint64_t id, const paginated_result<vote>& cursor,
                 const paginated_callback<vote>& callback);

  /**
   * @brief Fetches the most recent vote status for a specific user. Use this to
   * check whether a user has voted before granting in-app rewards or unlocking
   * features.
   *
   * @param project_id The project's Top.gg ID.
   * @param user_id The ID of the user to look up. The expected format depends
   * on the source parameter.
   * @param source The ID type being provided.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_vote(const uint64_t project_id, const std::string& user_id,
                const user_source& source,
                const callback<std::optional<partial_vote>>& callback);

  /**
   * @brief Fetches the integrations available for a project and whether each
   * one is connected.
   *
   * @param id The project's Top.gg ID.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_integrations(const uint64_t id,
                        const callback<std::vector<integration>>& callback);

  /**
   * @brief Connects an integration to the project. Top.gg runs the integration
   * handshake and starts delivering events to it.
   *
   * @param project_id The project's Top.gg ID.
   * @param integration_id The integration's Top.gg ID.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void connect_integration(const uint64_t project_id,
                           const uint64_t integration_id,
                           const empty_callback& callback);

  /**
   * @brief Disconnects an integration the application connected. The
   * integration receives an integration.delete event.
   *
   * @param project_id The project's Top.gg ID.
   * @param integration_id The integration's Top.gg ID.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void disconnect_integration(const uint64_t project_id,
                              const uint64_t integration_id,
                              const empty_callback& callback);

  /**
   * @brief Fetches the webhooks the application created on the project.
   *
   * @param id The project's Top.gg ID.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_webhooks(const uint64_t id,
                    const callback<std::vector<webhook>>& callback);

  /**
   * @brief Creates a vote webhook on the project.
   *
   * @param id The project's Top.gg ID.
   * @param webhook The vote webhook.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void create_webhook(const uint64_t id, const base_webhook& webhook,
                      const callback<topgg::webhook>& callback);

  /**
   * @brief Deletes a webhook the application created.
   *
   * @param project_id The project's Top.gg ID.
   * @param webhook_id The webhook's Top.gg ID.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void delete_webhook(const uint64_t project_id, const uint64_t webhook_id,
                      const empty_callback& callback);

  /**
   * @brief Replaces the signing secret of a webhook the application created.
   * The previous secret stops working immediately.
   *
   * @param project_id The project's Top.gg ID.
   * @param webhook_id The webhook's Top.gg ID.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void rotate_webhook_secret(const uint64_t project_id,
                             const uint64_t webhook_id,
                             const callback<std::string>& callback);

  /**
   * @brief Sends a webhook.test event to a webhook the application created.
   *
   * @param project_id The project's Top.gg ID.
   * @param webhook_id The webhook's Top.gg ID.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void test_webhook(const uint64_t project_id, const uint64_t webhook_id,
                    const empty_callback& callback);

  friend class oauth2_client;
};

/**
 * @brief An API client using an application token.
 *
 * @since 2.0.0
 */
struct application_client : private direct_http_provider,
                            public base_application_client {
  application_client() = delete;

  /**
   * @brief Creates the client.
   *
   * @param token The application token to use.
   * @since 2.0.0
   */
  inline application_client(const std::string& token)
      : direct_http_provider(token),
        base_application_client(static_cast<http_provider*>(this)) {}
};

/**
 * @brief An oauth2 session.
 *
 * @since 2.0.0
 */
struct oauth2_session {
  /**
   * @brief The session's access token.
   *
   * @since 2.0.0
   */
  std::string token{};

  /**
   * @brief The session's refresh token.
   *
   * @since 2.0.0
   */
  std::string refresh_token{};

  /**
   * @brief When the session's access token expires.
   *
   * @since 2.0.0
   */
  time_t expires_at{};

  /**
   * @brief Whether the session's access token has expired.
   *
   * @return bool Whether the session's access token has expired.
   * @since 2.0.0
   */
  inline bool has_expired() const noexcept {
    return time(nullptr) >= expires_at;
  }
};

/**
 * @brief An oauth2 URL.
 *
 * @since 2.0.0
 */
class oauth2_url {
  oauth2* m_oauth2{nullptr};

  inline oauth2_url(oauth2* oauth2_, const std::string& state_,
                    const std::string& code_verifier_, const std::string& url_)
      : m_oauth2(oauth2_),
        state(state_),
        code_verifier(code_verifier_),
        url(url_) {}

 public:
  oauth2_url() = delete;

  /**
   * @brief The state.
   *
   * @since 2.0.0
   */
  std::string state{};

  /**
   * @brief The code verifier.
   *
   * @since 2.0.0
   */
  std::string code_verifier{};

  /**
   * @brief The URL.
   *
   * @since 2.0.0
   */
  std::string url{};

  /**
   * @brief Exchanges an authorization code for an oauth2 session.
   *
   * @param code The request's code query parameter.
   * @param callback_ The API call's callback function.
   * @since 2.0.0
   */
  void exchange(const std::string& code,
                const callback<oauth2_session>& callback_);

  friend class oauth2;
};

class oauth2_client;

/**
 * @brief A thread-safe oauth2 manager.
 *
 * @since 2.0.0
 */
class oauth2 {
  http_client m_http{};
  uint64_t m_client_id{};
  std::string m_client_secret{};
  std::string m_scopes{};
  std::string m_redirect_uri{};
  std::mutex m_url_mutex{};
  EVP_MD* m_md{nullptr};
  EVP_MD_CTX* m_md_context{nullptr};
  BIO* m_bio{nullptr};
  BIO* m_base64_bio{nullptr};

 public:
  oauth2() = delete;

  /**
   * @brief Creates an oauth2 manager instance.
   *
   * @param client_id The client ID.
   * @param client_secret The client secret.
   * @param redirect_uri The redirect URI.
   * @param scopes An array of scope strings.
   * @throws topgg::exception Unable to create OpenSSL objects.
   * @since 2.0.0
   */
  oauth2(const uint64_t client_id, const std::string& client_secret,
         const std::string& redirect_uri,
         const std::initializer_list<std::string_view>& scopes);

  /**
   * @brief Creates an oauth2 API client from an oauth2 session.
   *
   * @param session The session.
   * @return std::shared_ptr<topgg::oauth2_client> The oauth2 API client.
   * @since 2.0.0
   */
  std::shared_ptr<oauth2_client> new_client(const oauth2_session& session);

  /**
   * @brief Creates an oauth2 URL from an existing state and code verifier.
   *
   * @param state The existing state.
   * @param code_verifier The existing code verifier.
   * @return std::shared_ptr<topgg::oauth2_url> The oauth2 URL.
   * @throws topgg::exception Code verifier computation failure.
   * @since 2.0.0
   */
  std::shared_ptr<oauth2_url> new_url(const std::string& state,
                                      const std::string& code_verifier);

  /**
   * @brief Creates an oauth2 URL.
   *
   * @return std::shared_ptr<topgg::oauth2_url> The oauth2 URL.
   * @throws topgg::exception Code verifier computation failure.
   * @since 2.0.0
   */
  std::shared_ptr<oauth2_url> new_url();

  ~oauth2();

  friend class oauth2_client;
  friend class oauth2_http_provider;  // OK
  friend class oauth2_url;
};

class oauth2_http_provider : protected http_provider {
 protected:
  oauth2* m_oauth2{nullptr};
  oauth2_session m_session{};
  mutable std::shared_mutex m_token_mutex{};

  void ensure_session(const empty_callback& callback_) override;

  std::string get_token() override;

  inline http_client* get_http() override { return &m_oauth2->m_http; }

  inline oauth2_http_provider(oauth2* oauth2_, const oauth2_session& session)
      : m_oauth2(oauth2_), m_session(session) {}

 public:
  oauth2_http_provider() = delete;
};

/**
 * @brief An oauth2 API client.
 *
 * @since 2.0.0
 */
class oauth2_client : private oauth2_http_provider,
                      public base_application_client {
  inline oauth2_client(oauth2* oauth2, const oauth2_session& session)
      : oauth2_http_provider(oauth2, session),
        base_application_client(static_cast<http_provider*>(this)) {}

 public:
  oauth2_client() = delete;

  /**
   * @brief Retrieves the client's oauth2 session.
   *
   * @return topgg::oauth2_session The oauth2 session.
   * @since 2.0.0
   */
  oauth2_session get_session();

  /**
   * @brief Fetches the user who authorized the application.
   *
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_authorized_user(const callback<user>& callback);

  /**
   * @brief Fetches the projects owned by the user who authorized the
   * application.
   *
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void get_authorized_user_projects(
      const callback<std::vector<user_project>>& callback);

  /**
   * @brief Creates a draft project for the user who authorized the application.
   * Name, icon, and missing descriptions are fetched from the platform. The
   * project starts as a draft; the user completes the listing and submits it
   * for review in their Top.gg dashboard.
   *
   * @param submission The project submission.
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void submit_project(const project_submission& submission,
                      const empty_callback& callback);

  /**
   * @brief Revokes the client's access token.
   *
   * @param callback The API call's callback function.
   * @since 2.0.0
   */
  void revoke_token(const empty_callback& callback);

  friend class oauth2;
};
};  // namespace topgg