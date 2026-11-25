#include <openssl/buffer.h>
#include <topgg/topgg.h>

#include <charconv>

#define TOPGG_HTTP_REQUEST_ASSERT(stmt) \
  topgg::exception::assert_curle(stmt, m_curl_error_details)
#define TOPGG_HTTP_CLIENT_ASSERT(stmt) topgg::exception::assert_curlm(stmt)

using topgg::base_application_client;
using topgg::base_client;
using topgg::direct_http_provider;
using topgg::http_client;
using topgg::http_request;
using topgg::http_response;
using topgg::oauth2;
using topgg::oauth2_client;
using topgg::oauth2_http_provider;
using topgg::oauth2_session;
using topgg::oauth2_url;
using topgg::project_client;

http_request::http_request(const char* method, const std::string& path,
                           const http_request::callback& callback,
                           const char* content_type, const std::string& body)
    : m_curl(curl_easy_init()),
      m_url(TOPGG_BASE_URL + path),
      m_callback(callback),
      m_request_body(body) {
  if (m_curl == nullptr) {
    throw std::bad_alloc{};
  }

  m_curl_error_details[0] = 0;

  curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER,
                   reinterpret_cast<void*>(m_curl_error_details));

  auto shared_this{
      new std::shared_ptr<http_request>{std::move(shared_from_this())}};

  try {
    TOPGG_HTTP_REQUEST_ASSERT(
        curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str()));
    TOPGG_HTTP_REQUEST_ASSERT(
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, method));
    TOPGG_HTTP_REQUEST_ASSERT(
        curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS));
    TOPGG_HTTP_REQUEST_ASSERT(curl_easy_setopt(m_curl, CURLOPT_PIPEWAIT, 1L));
    TOPGG_HTTP_REQUEST_ASSERT(curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L));
    TOPGG_HTTP_REQUEST_ASSERT(curl_easy_setopt(
        m_curl, CURLOPT_HEADERFUNCTION, http_request::write_response_headers));
    TOPGG_HTTP_REQUEST_ASSERT(curl_easy_setopt(
        m_curl, CURLOPT_HEADERDATA, reinterpret_cast<void*>(shared_this)));
    TOPGG_HTTP_REQUEST_ASSERT(curl_easy_setopt(
        m_curl, CURLOPT_WRITEFUNCTION, http_request::write_response_body));
    TOPGG_HTTP_REQUEST_ASSERT(curl_easy_setopt(
        m_curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(shared_this)));
    TOPGG_HTTP_REQUEST_ASSERT(curl_easy_setopt(
        m_curl, CURLOPT_PRIVATE, reinterpret_cast<void*>(shared_this)));

    m_headers = curl_slist_append(m_headers, content_type);
    m_headers =
        curl_slist_append(m_headers,
                          "User-Agent: topgg-cpp-sdk "
                          "(https://github.com/top-gg-community/cpp-sdk) "
                          "C++/");  // TODO: add version here

    TOPGG_HTTP_REQUEST_ASSERT(
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers));

    TOPGG_HTTP_REQUEST_ASSERT(
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, m_request_body.c_str()));
    TOPGG_HTTP_REQUEST_ASSERT(
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE,
                         static_cast<long>(m_request_body.size())));
  } catch (const topgg::exception& error) {
    delete shared_this;

    unref();

    throw error;
  }
}

void http_request::unref() {
  std::shared_ptr<http_request>* stored_shared_this{nullptr};

  if (curl_easy_getinfo(m_curl, CURLINFO_PRIVATE, &stored_shared_this) ==
          CURLE_OK &&
      stored_shared_this != nullptr) {
    delete stored_shared_this;

    curl_easy_setopt(m_curl, CURLOPT_PRIVATE, nullptr);
  }
}

void http_request::set_token(const std::string& token) {
  m_authorization = "Authorization: Bearer " + token;

  m_headers = curl_slist_append(m_headers, m_authorization.c_str());
}

size_t http_request::write_response_headers(char* ptr, size_t size,
                                            size_t nmemb, void* userdata) {
  const auto total_size{size * nmemb};
  auto self{reinterpret_cast<std::shared_ptr<http_request>*>(userdata)};

  std::string_view line{ptr, total_size};

  const auto trailing_newline_position{line.find_last_not_of("\r\n")};

  if (trailing_newline_position != std::string_view::npos) {
    line = line.substr(0, trailing_newline_position + 1);
  }

  const auto trimmed_total_size{line.size()};

  if (trimmed_total_size >= 10 && line.compare(0, 7, "HTTP/2 ") == 0) {
    uint32_t status{};

    auto [_, error]{std::from_chars(line.data() + 7,
                                    line.data() + trimmed_total_size, status)};

    (*self)->m_response_status = error == std::errc() ? status : 500;
  } else if (trimmed_total_size > 13 &&
             line.compare(0, 13, "retry-after: ") == 0) {
    uint32_t retry_after{};

    auto [_, error]{std::from_chars(
        line.data() + 13, line.data() + trimmed_total_size, retry_after)};

    if (error == std::errc()) {
      (*self)->m_retry_after = retry_after;
    }
  }

  return total_size;
}

size_t http_request::write_response_body(char* ptr, size_t size, size_t nmemb,
                                         void* userdata) {
  const auto total_size{size * nmemb};

  (*reinterpret_cast<std::shared_ptr<http_request>*>(userdata))
      ->m_response_body.append(ptr, total_size);

  return total_size;
}

void http_request::dispatch() {
  dispatch_variant(http_response{m_response_status, m_retry_after,
                                 std::string_view{m_response_body}});
}

http_request::~http_request() {
  if (m_headers != nullptr) {
    curl_slist_free_all(m_headers);

    m_headers = nullptr;
  }

  if (m_curl != nullptr) {
    curl_easy_cleanup(m_curl);

    m_curl = nullptr;
  }
}

http_client::http_client() : m_running(true) {
  curl_global_init(CURL_GLOBAL_ALL);

  if ((m_client = curl_multi_init()) == nullptr) {
    throw std::bad_alloc{};
  }

  TOPGG_HTTP_CLIENT_ASSERT(
      curl_multi_setopt(m_client, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX));

  m_event_loop_thread = std::thread([this]() { event_loop(); });
}

void http_client::event_loop() {
  int still_running{};
  int remaining_messages{};
  CURLMsg* message{nullptr};

HTTP_CLIENT_EVENT_LOOP:
  m_queued_requests_mutex.lock();

  while (!m_queued_requests.empty()) {
    auto request{m_queued_requests.front()};

    curl_multi_add_handle(m_client, request->m_curl);

    m_pending_requests.push_back(request);
    m_queued_requests.pop_back();
  }

  m_queued_requests_mutex.unlock();

  curl_multi_perform(m_client, &still_running);

  while ((message = curl_multi_info_read(m_client, &remaining_messages)) !=
         nullptr) {
    if (message->msg == CURLMSG_DONE) {
      std::shared_ptr<http_request>* request{nullptr};
      auto request_curl{(*request)->m_curl};

      if (curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &request) ==
              CURLE_OK &&
          request != nullptr) {
        m_callback_thread_pool.detach_task(
            [request = std::shared_ptr{*request}]() { request->dispatch(); });
      }

      curl_multi_remove_handle(m_client, message->easy_handle);

      (*request).get()->unref();

      auto request_pos{std::find_if(
          m_pending_requests.begin(), m_pending_requests.end(),
          [request_curl](const std::shared_ptr<http_request>& pending_request) {
            return pending_request->m_curl == request_curl;
          })};

      if (request_pos != m_pending_requests.end()) {
        m_pending_requests.erase(request_pos);
      }
    }
  }

  if (m_running.load(std::memory_order_relaxed) || still_running > 0) {
    curl_multi_poll(m_client, nullptr, 0, 5000, nullptr);

    goto HTTP_CLIENT_EVENT_LOOP;
  }
}

void http_client::submit(const std::shared_ptr<http_request>& request) {
  m_queued_requests_mutex.lock();
  m_queued_requests.push_back(request);
  m_queued_requests_mutex.unlock();

  TOPGG_HTTP_CLIENT_ASSERT(curl_multi_wakeup(m_client));
}

void http_client::safe_submit(const std::shared_ptr<http_request>& request) {
  try {
    submit(request);
  } catch (const topgg::exception& error) {
    m_callback_thread_pool.detach_task(
        [request, error]() { request->dispatch_variant(error); });
  }
}

http_client::~http_client() {
  if (m_client != nullptr) {
    m_running.store(false, std::memory_order_relaxed);

    curl_multi_wakeup(m_client);

    if (m_event_loop_thread.joinable()) {
      m_event_loop_thread.join();
    }

    curl_multi_cleanup(m_client);

    for (auto& request : m_pending_requests) {
      request->unref();
    }

    curl_global_cleanup();

    m_client = nullptr;
  }
}

void direct_http_provider::ensure_session(
    const topgg::empty_callback& callback_) {
  m_http.dispatch<std::monostate>(callback_, std::monostate{});
}

void base_client::fetch_json(const std::shared_ptr<http_request>& request) {
  m_provider->ensure_session(
      [this, request](const topgg::empty_result& result) {
        try {
          result.check();
          request->set_token(m_provider->get_token());

          m_provider->get_http()->submit(request);
        } catch (const topgg::request_exception& error) {
          request->dispatch_variant(error);
        } catch (const topgg::exception& error) {
          request->dispatch_variant(error);
        } catch (const nlohmann::json::exception& error) {
          request->dispatch_variant(error);
        }
      });
}

void base_client::fetch_empty(const char* method, const std::string& path,
                              const topgg::empty_callback& callback,
                              const std::string& body) {
  fetch_json(std::shared_ptr<http_request>{new http_request{
      method, path,
      [callback](const topgg::http_response_variant& response_variant) {
        _TOPGG_UNWRAP_VARIANT(callback, response_variant, {
          try {
            const auto& response{std::get<http_response>(response_variant)};

            if (response.status >= 400) {
              callback(topgg::request_exception::parse(response));
            } else {
              callback(std::monostate{});
            }
          } catch (const nlohmann::json::exception& error) {
            callback(error);
          }
        });
      },
      TOPGG_CT_APPLICATION_JSON, body}});
}

void project_client::get_project(
    const topgg::callback<topgg::project>& callback) {
  fetch_simple("GET", "/projects/@me", callback);
}

void project_client::edit_project(const topgg::locale_map& headline,
                                  const topgg::locale_map& page_content,
                                  const topgg::empty_callback& callback) {
  if (headline.m_json.empty() && page_content.m_json.empty()) {
    m_http.dispatch<topgg::exception>(
        callback, "Either headline or page_content must be specified");

    return;
  }

  if (enforce_constraints(headline, 3, 140, callback) &&
      enforce_constraints(page_content, 300, 50000, callback)) {
    nlohmann::json body{};

    body["headline"] = headline.m_json;
    body["page_content"] = page_content.m_json;

    fetch_empty("PATCH", "/projects/@me", callback, body.dump());
  }
}

void project_client::post_announcement(
    const std::string& title, const std::string& content,
    const topgg::announcement_category category,
    const topgg::empty_callback& callback) {
  if (title.length() < 3 || title.length() > 100) {
    m_http.dispatch<topgg::exception>(
        callback, "Title length is outside of the accepted threshold");

    return;
  } else if (content.length() < 100 || content.length() > 2000) {
    m_http.dispatch<topgg::exception>(
        callback, "Content length is outside of the accepted threshold");

    return;
  }

  nlohmann::json body{};

  body["title"] = title;
  body["content"] = content;

  std::string category_string{};

  switch (category) {
    case topgg::announcement_category::ac_announcement:
      category_string = "announcement";
      break;
    case topgg::announcement_category::ac_event:
      category_string = "event";
      break;
    case topgg::announcement_category::ac_new_feature:
      category_string = "new_feature";
      break;
  }

  body["category"] = category_string;

  fetch_empty("POST", "/projects/@me/announcements", callback, body.dump());
}

void project_client::post_metrics(const topgg::metrics& metrics,
                                  const topgg::empty_callback& callback) {
  fetch_empty("PATCH", "/projects/@me/metrics", callback,
              metrics.to_json().dump());
}

void project_client::post_commands(const std::string& commands,
                                   const topgg::empty_callback& callback) {
  fetch_empty("PUT", "/projects/@me/commands", callback, commands);
}

void project_client::get_votes(
    const time_t since,
    const topgg::paginated_callback<topgg::vote>& callback) {
  const auto now{time(nullptr)};

  if (now < since || (now - since) > 31536000) {
    m_http.dispatch<topgg::exception>(callback, "Invalid since timestamp");

    return;
  }

  fetch_paginated("data", "GET",
                  "/projects/@me/votes?startDate=" +
                      topgg::_url_encode(topgg::_to_time_string(since)),
                  callback);
}

void project_client::get_votes(
    const topgg::paginated_result<topgg::vote>& cursor,
    const topgg::paginated_callback<topgg::vote>& callback) {
  fetch_paginated(cursor, "data", "GET", "/projects/@me/votes", callback);
}

void project_client::get_vote(
    const std::string& user_id, const topgg::user_source& source,
    const topgg::callback<std::optional<topgg::partial_vote>>& callback) {
  std::string source_string{};

  switch (source) {
    case topgg::user_source::us_topgg:
      source_string = "topgg";
      break;
    case topgg::user_source::us_discord:
      source_string = "discord";
      break;
  }

  fetch_json(std::shared_ptr<http_request>{new http_request{
      "GET", "/projects/@me/votes/" + user_id + "?source=" + source_string,
      [callback](const topgg::http_response_variant& response_variant) {
        _TOPGG_UNWRAP_VARIANT(callback, response_variant, {
          try {
            const auto& response{std::get<http_response>(response_variant)};

            if (response.status >= 400) {
              auto error{topgg::request_exception::parse(response)};

              if (error.status == 404 && error.code.has_value()) {
                const auto& code{error.code.value()};

                if (code == "VOTE_NOT_FOUND" || code == "VOTE_EXPIRED") {
                  callback(std::optional<topgg::partial_vote>{});

                  return;
                }
              }

              callback(error);
            } else {
              callback(std::optional{
                  topgg::partial_vote{nlohmann::json::parse(response.body)}});
            }
          } catch (const nlohmann::json::exception& error) {
            callback(error);
          }
        });
      }}});
}

void base_application_client::get_projects(
    const topgg::paginated_callback<topgg::partial_project>& callback) {
  fetch_paginated("projects", "GET", "/projects", callback);
}

void base_application_client::get_project(
    const uint64_t id, const topgg::callback<topgg::project>& callback) {
  fetch_simple("GET", "/projects/" + id, callback);
}

void base_application_client::edit_project(
    const uint64_t id, const topgg::locale_map& headline,
    const topgg::locale_map& page_content,
    const topgg::empty_callback& callback) {
  if (headline.m_json.empty() && page_content.m_json.empty()) {
    m_provider->get_http()->dispatch<topgg::exception>(
        callback, "Either headline or page_content must be specified");

    return;
  }

  if (enforce_constraints(headline, 3, 140, callback) &&
      enforce_constraints(page_content, 300, 50000, callback)) {
    nlohmann::json body{};

    body["headline"] = headline.m_json;
    body["page_content"] = page_content.m_json;

    fetch_empty("PATCH", "/projects/" + id, callback, body.dump());
  }
}

void base_application_client::post_announcement(
    const uint64_t id, const std::string& title, const std::string& content,
    const topgg::announcement_category category,
    const topgg::empty_callback& callback) {
  if (title.length() < 3 || title.length() > 100) {
    m_provider->get_http()->dispatch<topgg::exception>(
        callback, "Title length is outside of the accepted threshold");

    return;
  } else if (content.length() < 100 || content.length() > 2000) {
    m_provider->get_http()->dispatch<topgg::exception>(
        callback, "Content length is outside of the accepted threshold");

    return;
  }

  nlohmann::json body{};

  body["title"] = title;
  body["content"] = content;

  std::string category_string{};

  switch (category) {
    case topgg::announcement_category::ac_announcement:
      category_string = "announcement";
      break;
    case topgg::announcement_category::ac_event:
      category_string = "event";
      break;
    case topgg::announcement_category::ac_new_feature:
      category_string = "new_feature";
      break;
  }

  body["category"] = category_string;

  fetch_empty("POST", "/projects/" + std::to_string(id) + "/announcements",
              callback, body.dump());
}

void base_application_client::post_metrics(
    const uint64_t id, const topgg::metrics& metrics,
    const topgg::empty_callback& callback) {
  fetch_empty("PATCH", "/projects/" + std::to_string(id) + "/metrics", callback,
              metrics.to_json().dump());
}

void base_application_client::post_commands(
    const uint64_t id, const std::string& commands,
    const topgg::empty_callback& callback) {
  fetch_empty("PUT", "/projects/" + std::to_string(id) + "/commands", callback,
              commands);
}

void base_application_client::get_votes(
    const uint64_t id, const time_t since,
    const topgg::paginated_callback<topgg::vote>& callback) {
  const auto now{time(nullptr)};

  if (now < since || (now - since) > 31536000) {
    m_provider->get_http()->dispatch<topgg::exception>(
        callback, "Invalid since timestamp");

    return;
  }

  fetch_paginated("data", "GET",
                  "/projects/" + std::to_string(id) + "/votes?startDate=" +
                      topgg::_url_encode(topgg::_to_time_string(since)),
                  callback);
}

void base_application_client::get_votes(
    const uint64_t id, const topgg::paginated_result<topgg::vote>& cursor,
    const topgg::paginated_callback<topgg::vote>& callback) {
  fetch_paginated(cursor, "data", "GET",
                  "/projects/" + std::to_string(id) + "/votes", callback);
}

void base_application_client::get_vote(
    const uint64_t id, const std::string& user_id,
    const topgg::user_source& source,
    const topgg::callback<std::optional<topgg::partial_vote>>& callback) {
  std::string source_string{};

  switch (source) {
    case topgg::user_source::us_topgg:
      source_string = "topgg";
      break;
    case topgg::user_source::us_discord:
      source_string = "discord";
      break;
  }

  fetch_json(std::shared_ptr<http_request>{new http_request{
      "GET",
      "/projects/" + std::to_string(id) + "/votes/" + user_id +
          "?source=" + source_string,
      [callback](const topgg::http_response_variant& response_variant) {
        _TOPGG_UNWRAP_VARIANT(callback, response_variant, {
          try {
            const auto& response{std::get<http_response>(response_variant)};

            if (response.status >= 400) {
              auto error{topgg::request_exception::parse(response)};

              if (error.status == 404 && error.code.has_value()) {
                const auto& code{error.code.value()};

                if (code == "VOTE_NOT_FOUND" || code == "VOTE_EXPIRED") {
                  callback(std::optional<topgg::partial_vote>{});

                  return;
                }
              }

              callback(error);
            } else {
              callback(std::optional{
                  topgg::partial_vote{nlohmann::json::parse(response.body)}});
            }
          } catch (const nlohmann::json::exception& error) {
            callback(error);
          }
        });
      }}});
}

void base_application_client::get_integrations(
    const uint64_t id,
    const topgg::callback<std::vector<topgg::integration>>& callback) {
  fetch_vector("GET", "/projects/" + std::to_string(id) + "/integrations",
               callback);
}

void base_application_client::connect_integration(
    const uint64_t project_id, const uint64_t integration_id,
    const topgg::empty_callback& callback) {
  fetch_empty("PUT",
              "/projects/" + std::to_string(project_id) + "/integrations/" +
                  std::to_string(integration_id),
              callback);
}

void base_application_client::disconnect_integration(
    const uint64_t project_id, const uint64_t integration_id,
    const topgg::empty_callback& callback) {
  fetch_empty("DELETE",
              "/projects/" + std::to_string(project_id) + "/integrations/" +
                  std::to_string(integration_id),
              callback);
}

void base_application_client::get_webhooks(
    const uint64_t id,
    const topgg::callback<std::vector<topgg::webhook>>& callback) {
  fetch_vector("GET", "/projects/" + std::to_string(id) + "/webhooks",
               callback);
}

void base_application_client::create_webhook(
    const uint64_t id, const topgg::base_webhook& webhook,
    const topgg::callback<topgg::webhook>& callback) {
  nlohmann::json body{};

  body["label"] = webhook.label;
  body["url"] = webhook.url;

  fetch_simple("POST", "/projects/" + std::to_string(id) + "/webhooks",
               callback, body.dump());
}

void base_application_client::delete_webhook(
    const uint64_t project_id, const uint64_t webhook_id,
    const topgg::empty_callback& callback) {
  fetch_empty("DELETE",
              "/projects/" + std::to_string(project_id) + "/webhooks/" +
                  std::to_string(webhook_id),
              callback);
}

void base_application_client::rotate_webhook_secret(
    const uint64_t project_id, const uint64_t webhook_id,
    const topgg::callback<std::string>& callback) {
  fetch_json(std::shared_ptr<http_request>{new http_request{
      "POST",
      "/projects/" + std::to_string(project_id) + "/webhooks/" +
          std::to_string(webhook_id) + "/rotate",
      [callback](const topgg::http_response_variant& response_variant) {
        _TOPGG_UNWRAP_VARIANT(callback, response_variant, {
          try {
            const auto& response{std::get<http_response>(response_variant)};

            if (response.status >= 400) {
              callback(topgg::request_exception::parse(response));
            } else {
              const auto json{nlohmann::json::parse(response.body)};

              callback(json["secret"].template get<std::string>());
            }
          } catch (const nlohmann::json::exception& error) {
            callback(error);
          }
        });
      }}});
}

void base_application_client::test_webhook(
    const uint64_t project_id, const uint64_t webhook_id,
    const topgg::empty_callback& callback) {
  fetch_empty("POST",
              "/projects/" + std::to_string(project_id) + "/webhooks/" +
                  std::to_string(webhook_id) + "/test",
              callback);
}

void oauth2_url::exchange(const std::string& code,
                          const topgg::callback<oauth2_session>& callback_) {
  std::string body{"grant_type=authorization_code&client_id="};

  body += m_oauth2->m_client_id +
          "&client_secret=" + m_oauth2->m_client_secret +
          "&code=" + topgg::_url_encode(code) +
          "&redirect_uri=" + topgg::_url_encode(m_oauth2->m_redirect_uri) +
          "&code_verifier=" + topgg::_url_encode(code_verifier);

  m_oauth2->m_http.safe_submit(
      std::shared_ptr<http_request>{new topgg::http_request{
          "POST", "/oauth2/token",
          [this,
           callback_](const topgg::http_response_variant& response_variant) {
            _TOPGG_UNWRAP_VARIANT(callback_, response_variant, {
              try {
                const auto& response{
                    std::get<topgg::http_response>(response_variant)};

                if (response.status >= 400) {
                  callback_(topgg::request_exception::parse(response));
                } else {
                  const auto json{nlohmann::json::parse(response.body)};

                  callback_(topgg::oauth2_session{
                      json["access_token"].template get<std::string>(),
                      json["refresh_token"].template get<std::string>(),
                      time(nullptr) +
                          json["expires_in"].template get<time_t>()});
                }
              } catch (const nlohmann::json::exception& error) {
                callback_(error);
              }
            });
          },
          TOPGG_CT_APPLICATION_X_WWW_FORM_URLENCODED, body}});
}

oauth2::oauth2(const uint64_t client_id, const std::string& client_secret,
               const std::string& redirect_uri,
               const std::initializer_list<std::string_view>& scopes)
    : m_client_id(client_id),
      m_client_secret(client_secret),
      m_redirect_uri(redirect_uri) {
  for (const auto& scope : scopes) {
    m_scopes += '+';
    m_scopes += scope;
  }

  if ((m_bio = BIO_new(BIO_s_mem())) == nullptr ||
      (m_base64_bio = BIO_new(BIO_f_base64())) == nullptr) {
    throw topgg::exception::ssl("Unable to create BIO");
  } else if ((m_md = EVP_MD_fetch(nullptr, "SHA256", nullptr)) == nullptr ||
             (m_md_context = EVP_MD_CTX_new()) == nullptr ||
             EVP_DigestInit_ex(m_md_context, m_md, nullptr) != 1) {
    throw topgg::exception::ssl("Unable to create SHA256 digest context");
  }

  BIO_set_flags(m_base64_bio, BIO_FLAGS_BASE64_NO_NL);
  m_base64_bio = BIO_push(m_base64_bio, m_bio);
}

std::shared_ptr<topgg::oauth2_client> oauth2::new_client(
    const topgg::oauth2_session& session) {
  return std::shared_ptr<topgg::oauth2_client>{
      new topgg::oauth2_client{this, session}};
}

std::shared_ptr<topgg::oauth2_url> oauth2::new_url(
    const std::string& state, const std::string& code_verifier) {
  std::lock_guard guard{m_url_mutex};

  if (EVP_DigestUpdate(m_md_context, code_verifier.data(),
                       code_verifier.length()) != 1) {
    throw topgg::exception{"Unable to update sha256 hash of code verifier"};
  }

  std::vector<uint8_t> code_verifier_hash(
      static_cast<size_t>(EVP_MD_get_size(m_md)));
  BUF_MEM* code_challenge_hash_base64{};
  unsigned int digest_size{};

  if (EVP_DigestFinal_ex(m_md_context, code_verifier_hash.data(),
                         &digest_size) != 1 ||
      BIO_write(m_base64_bio, code_verifier_hash.data(),
                static_cast<int>(code_verifier_hash.size())) < 1 ||
      BIO_flush(m_base64_bio) != 1) {
    throw topgg::exception{"Unable to compute sha256 digest of code verifier"};
  } else if (EVP_MD_CTX_reset(m_md_context) != 1 ||
             EVP_DigestInit_ex(m_md_context, m_md, nullptr) != 1) {
    throw topgg::exception{"Unable to reuse sha256 digest context"};
  }

  BIO_get_mem_ptr(m_bio, &code_challenge_hash_base64);

  auto code_challenge{std::string{code_challenge_hash_base64->data,
                                  code_challenge_hash_base64->length}};

  std::replace(code_challenge.begin(), code_challenge.end(), '+', '-');
  std::replace(code_challenge.begin(), code_challenge.end(), '/', '_');

  code_challenge.erase(
      std::remove(code_challenge.begin(), code_challenge.end(), '='),
      code_challenge.end());

  std::string_view scopes{m_scopes};

  scopes = scopes.empty() ? scopes : scopes.substr(1);

  auto url{std::string{"https://top.gg/oauth2/"
                       "authorize?response_type=code&code_challenge_method="
                       "S256&client_id="} +
           std::to_string(m_client_id) +
           "&redirect_uri=" + topgg::_url_encode(m_redirect_uri) +
           "&state=" + topgg::_url_encode(state) +
           "&code_challenge=" + topgg::_url_encode(code_challenge) + "&scope="};

  url += scopes;

  return std::shared_ptr<topgg::oauth2_url>{
      new topgg::oauth2_url{this, state, code_verifier, url}};
}

std::shared_ptr<topgg::oauth2_url> oauth2::new_url() {
  const auto state{topgg::_random_string()};
  auto code_verifier{topgg::_random_string()};

  return new_url(state, code_verifier);
}

oauth2::~oauth2() {
  if (m_md_context != nullptr) {
    EVP_MD_CTX_free(m_md_context);

    m_md_context = nullptr;
  }

  if (m_md != nullptr) {
    EVP_MD_free(m_md);

    m_md = nullptr;
  }

  if (m_base64_bio != nullptr) {
    BIO_free(m_base64_bio);

    m_base64_bio = nullptr;
  }

  if (m_bio != nullptr) {
    BIO_free(m_bio);

    m_bio = nullptr;
  }
}

void oauth2_http_provider::ensure_session(
    const topgg::empty_callback& callback_) {
  std::string body{"grant_type=refresh_token&client_id="};

  {
    std::shared_lock guard{m_token_mutex};

    body += m_oauth2->m_client_id +
            "&client_secret=" + m_oauth2->m_client_secret +
            "&refresh_token=" + m_session.refresh_token;
  };

  m_oauth2->m_http.safe_submit(
      std::shared_ptr<http_request>{new topgg::http_request{
          "POST", "/oauth2/token",
          [this,
           callback_](const topgg::http_response_variant& response_variant) {
            _TOPGG_UNWRAP_VARIANT(callback_, response_variant, {
              try {
                const auto& response{std::get<http_response>(response_variant)};
                const auto json{nlohmann::json::parse(response.body)};

                std::unique_lock guard{m_token_mutex};

                m_session = topgg::oauth2_session{
                    json["access_token"].template get<std::string>(),
                    json["refresh_token"].template get<std::string>(),
                    time(nullptr) + json["expires_in"].template get<time_t>()};
              } catch (const nlohmann::json::exception& error) {
                callback_(error);
              }
            });
          },
          TOPGG_CT_APPLICATION_X_WWW_FORM_URLENCODED, body}});
}

std::string oauth2_http_provider::get_token() {
  std::shared_lock guard{m_token_mutex};

  return m_session.token;
}

oauth2_session oauth2_client::get_session() {
  std::shared_lock guard{m_token_mutex};

  return m_session;
}

void oauth2_client::get_authorized_user(
    const topgg::callback<topgg::user>& callback) {
  fetch_simple("GET", "/users/@me", callback);
}

void oauth2_client::get_authorized_user_projects(
    const topgg::callback<std::vector<topgg::user_project>>& callback) {
  fetch_vector("GET", "/users/@me/projects", callback);
}

void oauth2_client::submit_project(const topgg::project_submission& submission,
                                   const topgg::empty_callback& callback) {
  nlohmann::json body{};

  body["platform"] =
      std::string{topgg::_platform_to_string(submission.platform)};
  body["type"] = std::string{topgg::_project_type_to_string(submission.type)};
  body["platform_id"] = submission.platform_id;

  if (submission.headline.length() < 3 || submission.headline.length() > 140) {
    m_oauth2->m_http.dispatch<topgg::exception>(
        callback, "Headline length is outside of the accepted threshold");

    return;
  } else if (submission.page_content.length() < 300 ||
             submission.page_content.length() > 50000) {
    m_oauth2->m_http.dispatch<topgg::exception>(
        callback, "Page content length is outside of the accepted threshold");

    return;
  }

  body["headline"] = submission.headline;
  body["page_content"] = submission.page_content;

  fetch_empty("POST", "/users/@me/projects", callback, body.dump());
}

void oauth2_client::revoke_token(const topgg::empty_callback& callback) {
  std::string body{"client_id="};

  {
    std::shared_lock guard{m_token_mutex};

    body += m_oauth2->m_client_id +
            "&client_secret=" + m_oauth2->m_client_secret +
            "&token=" + m_session.refresh_token;
  };

  m_oauth2->m_http.safe_submit(
      std::shared_ptr<http_request>{new topgg::http_request{
          "POST", "/oauth2/revoke",
          [callback](const topgg::http_response_variant& response_variant) {
            _TOPGG_UNWRAP_VARIANT(callback, response_variant, {
              try {
                const auto& response{
                    std::get<topgg::http_response>(response_variant)};

                if (response.status >= 400) {
                  callback(topgg::request_exception::parse(response));
                } else {
                  callback(std::monostate{});
                }
              } catch (const nlohmann::json::exception& error) {
                callback(error);
              }
            });
          },
          TOPGG_CT_APPLICATION_X_WWW_FORM_URLENCODED, body}});
}

topgg::webhooks::payload topgg::webhooks::parse(const std::string& body) {
  const auto json{nlohmann::json::parse(body)};
  const auto type{json["type"].template get<std::string>()};
  const auto data{json["data"]};

  if (type == "vote.create") {
    return topgg::webhooks::vote_create{data};
  } else if (type == "webhook.test") {
    return topgg::webhooks::test{data};
  } else if (type == "integration.create") {
    return topgg::webhooks::integration_create{data};
  } else if (type == "integration.delete") {
    return topgg::webhooks::integration_delete{data};
  }

  throw topgg::exception{"Got an unrecognized event type"};
}

topgg::webhooks::verifier::verifier() {
  if ((m_mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr)) == nullptr ||
      (m_mac_context = EVP_MAC_CTX_new(m_mac)) == nullptr) {
    throw topgg::exception::ssl("Unable to create HMAC context");
  }

  char sha256[] = "SHA256";

  m_params[0] = OSSL_PARAM_construct_utf8_string("digest", sha256, 0);
  m_params[1] = OSSL_PARAM_construct_end();
}

static const char g_hex_digits[] = "0123456789abcdef";

bool topgg::webhooks::verifier::verify(const std::string& secret,
                                       const std::string& signature,
                                       const std::string& body) {
  try {
    const auto signature_comma_position{signature.find(',')};

    if (signature_comma_position == std::string::npos) {
      return false;
    }

    const auto signature_timestamp_pair{
        signature.substr(0, signature_comma_position)};
    const auto signature_timestamp_equal_position{
        signature_timestamp_pair.find('=')};

    if (signature_timestamp_equal_position == std::string::npos) {
      return false;
    }

    const auto signature_timestamp_text{
        signature_timestamp_pair.substr(signature_timestamp_equal_position + 1,
                                        signature_timestamp_pair.length())};
    size_t signature_timestamp_pos{};
    int64_t signature_timestamp{
        std::stoll(signature_timestamp_text, &signature_timestamp_pos)};

    if (signature_timestamp_pos != signature_timestamp_text.size() ||
        (abs(static_cast<int64_t>(time(nullptr)) - signature_timestamp) >
         300)) {
      return false;
    }

    const auto hmac_update_string{signature_timestamp_text + '.' + body};

    const auto signature_signature_pair{
        signature.substr(signature_comma_position + 1, signature.length())};
    const auto signature_signature_equal_position{
        signature_signature_pair.find('=')};

    if (signature_signature_equal_position == std::string::npos) {
      return false;
    }

    const auto signature_signature{
        signature_signature_pair.substr(signature_signature_equal_position + 1,
                                        signature_signature_pair.length())};

    size_t hmac_length{};
    std::vector<uint8_t> hmac(hmac_length);

    {
      std::lock_guard guard_{m_mutex};

      if (EVP_MAC_init(m_mac_context,
                       reinterpret_cast<const unsigned char*>(secret.c_str()),
                       secret.length(), m_params) != 1 ||
          EVP_MAC_update(m_mac_context,
                         reinterpret_cast<const unsigned char*>(
                             hmac_update_string.c_str()),
                         hmac_update_string.length()) != 1 ||
          EVP_MAC_final(m_mac_context, nullptr, &hmac_length, 0) != 1 ||
          (hmac_length * 2) != signature_signature.length() ||
          EVP_MAC_final(m_mac_context, hmac.data(), &hmac_length,
                        hmac_length) != 1) {
        return false;
      }
    }

    uint8_t result{};
    size_t index{};

    for (const auto byte : hmac) {
      result |= (signature_signature[index++] ^ g_hex_digits[byte >> 4]);
      result |= (signature_signature[index++] ^ g_hex_digits[byte & 0x0f]);
    }

    return result == 0;
  } catch (const std::exception&) {
    return false;
  }
}

topgg::webhooks::verifier::~verifier() {
  if (m_mac_context != nullptr) {
    EVP_MAC_CTX_free(m_mac_context);

    m_mac_context = nullptr;
  }

  if (m_mac != nullptr) {
    EVP_MAC_free(m_mac);

    m_mac = nullptr;
  }
}