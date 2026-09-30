#pragma once

#include <topgg/models.h>
#include <topgg/result.h>
#include <topgg/http.h>
#include <functional>
#include <string>

#ifndef TOPGG_PROJECT_TOKENS_ONLY
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <ctime>
#include <uv.h>
#endif


namespace topgg {
  template<class T>
  using callback = std::function<void(const result<T>&)>;
  using empty_callback = std::function<void(const empty_result&)>;
  template<class T>
  using paginated_callback = std::function<void(const paginated_result<T>&)>;

#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
  class client;
#endif=
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  class oauth2_client;
#endif

  class base_client {
    virtual http_frontend* get_http() = 0;

    virtual std::string get_token() = 0;

    template<class T>
    void fetch_simple(const std::string_view& method, const std::string& path, const callback<T>& callback_, const bool defer, const std::string& body = "") {
      get_http()->fetch(new http_request{get_token(), method, path, [callback_](const http_response& response) {
        if (std::holds_alternative<exception>(response)) {
          callback_(std::get<exception>(response));
        } else {
          try {
            const auto& response_pair{std::get<std::pair<uint16_t, std::string_view>>(response)};

            if (response_pair.first >= 400) {
              callback_(http_exception{response_pair});
            } else {
              callback_(T{nlohmann::json::parse(response_pair.second)});
            }
          } catch (const nlohmann::json::exception& error) {
            callback_(error);
          }
        }
      }, body}, defer);
    }

    template<class T>
    void fetch_paginated(const char* key, const std::string_view& method, const std::string& path, const paginated_callback<T>& callback, const bool defer) {
      get_http()->fetch(new http_request{get_token(), method, path, [key, callback](const http_response& response) {
        if (std::holds_alternative<exception>(response)) {
          callback(std::get<exception>(response));
        } else {
          try {
            const auto& response_pair{std::get<std::pair<uint16_t, std::string_view>>(response)};

            if (response_pair.first >= 400) {
              callback(http_exception{response_pair});
            } else {
              callback(paginated_result<T>::from_array(key, nlohmann::json::parse(response_pair.second)));
            }
          } catch (const nlohmann::json::exception& error) {
            callback(error);
          }
        }
      }}, defer);
    }

    template<class T>
    void fetch_paginated(const paginated_result<T>& cursor_, const char* key, const std::string_view& method, const std::string& path, const paginated_callback<T>& callback, const bool defer) {
      const auto cursor{cursor_.cursor()};

      if (cursor.has_value()) {
        fetch_paginated(key, method, path + "?cursor=" + cursor.value(), callback, defer);
      } else {
        callback(paginated_result<T>::empty());
      }
    }

    void fetch_empty(const std::string_view& method, const std::string& path, const empty_callback& callback, const bool defer, const std::string& body = "");

#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
    friend class client;
#endif
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
  };

#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
  class client: private base_client {
    http_frontend m_http{};
    std::string m_token{};

    inline http_frontend* get_http() override {
      return &m_http;
    }

    inline std::string get_token() override {
      return m_token;
    }

  public:
    client() = delete;

    inline client(const std::string& token): m_token(token) {}

    void get_own_project(const callback<project>& callback, const bool defer = false);

    void edit_own_project(const localized_string& headline, const localized_string& page_content, const empty_callback& callback, const bool defer = false);

    void post_own_announcement(const std::string& title, const std::string& content, const announcement_category category, const empty_callback& callback, const bool defer = false);

    void post_own_metrics(const metrics& metrics, const empty_callback& callback, const bool defer = false);

    template<class T>
    void post_own_metrics(const timestamped_metrics<T>& metrics_, const empty_callback& callback, const bool defer = false) {
      if (metrics_.m_json.size() < 1 || metrics_.m_json.size() > 100) {
        return callback(exception{"Batch length is outside of the accepted threshold"});
      }

      nlohmann::json body{};

      body["data"] = metrics_.m_json;

      fetch_empty("POST", "/projects/@me/metrics/batch", callback, defer, body.dump());
    }

    void post_own_commands(const std::string& commands, const empty_callback& callback, const bool defer = false);

    void get_own_votes(const time_t since, const paginated_callback<vote>& callback, const bool defer = false);

    void get_own_votes(const paginated_result<vote>& cursor, const paginated_callback<vote>& callback, const bool defer = false);

    void get_own_votes(const std::string& user_id, const user_source& source, const callback<partial_vote>& callback, const bool defer = false);
  };
#endif

#ifndef TOPGG_PROJECT_TOKENS_ONLY
  struct oauth2_session {
    std::string token{};
    std::string refresh_token{};
    time_t token_expires_at{};
  };

  class oauth2 {
    http_frontend m_http{};
    std::string m_client_id{};
    std::string m_client_secret{};
    std::string m_scopes{};
    std::string m_redirect_uri{};
    std::mutex m_regenerate_mutex{};
    std::string m_state{};
    std::string m_code_verifier{};
    std::string m_code_challenge{};
    EVP_MD* m_md{nullptr};
    EVP_MD_CTX* m_md_context{nullptr};
    BIO* m_bio{nullptr};
    BIO* m_base64_bio{nullptr};

  public:
    oauth2() = delete;

    oauth2(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri, const std::initializer_list<std::string_view>& scopes);

    std::shared_ptr<oauth2_client> new_client(const oauth2_session& session);

    void regenerate();

    std::string get_url();

    void exchange(const std::string& code, const std::string& state, const callback<oauth2_session>& callback);

    ~oauth2();

    friend class oauth2_client;
  };

  class oauth2_client: private base_client, private std::enable_shared_from_this<oauth2_client> {
    oauth2* m_oauth2{nullptr};
    oauth2_session m_session{};
    std::mutex m_token_mutex{};
    uv_timer_t m_oauth2_refresh_timer{};

    inline oauth2_client(oauth2* oauth2, const oauth2_session& session): m_oauth2(oauth2), m_session(session) {
      m_oauth2_refresh_timer.data = nullptr;
    }

    inline http_frontend* get_http() override {
      return &m_oauth2->m_http;
    }

    std::string get_token() override;

    void refresh_token();

    void stop_refresh_token();

  public:
    oauth2_client() = delete;

    oauth2_session get_session();

    void get_projects(const paginated_callback<partial_project>& callback, const bool defer = false);

    void get_projects(const paginated_result<partial_project>& cursor, const paginated_callback<partial_project>& callback, const bool defer = false);

    void get_project(const std::string& id, const callback<project>& callback, const bool defer = false);

    void edit_project(const std::string& id, const localized_string& headline, const localized_string& page_content, const empty_callback& callback, const bool defer = false);

    void post_announcement(const std::string& id, const std::string& title, const std::string& content, const announcement_category category, const empty_callback& callback, const bool defer = false);

    void post_metrics(const std::string& id, const metrics& metrics, const empty_callback& callback, const bool defer = false);

    template<class T>
    void post_metrics(const std::string& id, const timestamped_metrics<T>& metrics_, const empty_callback& callback, const bool defer = false) {
      if (metrics_.m_json.size() < 1 || metrics_.m_json.size() > 100) {
        return callback(exception{"Batch length is outside of the accepted threshold"});
      }

      nlohmann::json body{};

      body["data"] = metrics_.m_json;

      fetch_empty("POST", "/projects/" + id + "/metrics/batch", callback, defer, body.dump());
    }

    void post_commands(const std::string& id, const std::string& commands, const empty_callback& callback, const bool defer = false);

    void get_votes(const std::string& id, const time_t since, const paginated_callback<vote>& callback, const bool defer = false);

    void get_votes(const std::string& id, const paginated_result<vote>& cursor, const paginated_callback<vote>& callback, const bool defer = false);

    void get_votes(const std::string& id, const std::string& user_id, const user_source& source, const callback<partial_vote>& callback, const bool defer = false);

    void get_authorized_user(const callback<user>& callback, const bool defer = false);

    void get_authorized_user_project(const callback<std::vector<user_project>>& callback, const bool defer = false);

    void submit_project(const project_submission& submission, const empty_callback& callback, const bool defer = false);

    void revoke_token(const empty_callback& callback, const bool defer = false);

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
    ~oauth2_client();
#endif

    friend class http_backend;
    friend class oauth2;
  };
#endif
};