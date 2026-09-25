#pragma once

#ifndef NGHTTP2_NO_SSIZE_T
#define NGHTTP2_NO_SSIZE_T
#endif

#include <nghttp2/nghttp2.h>
#include <nlohmann/json.hpp>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <topgg/util.h>
#include <functional>
#include <optional>
#include <atomic>
#include <thread>
#include <deque>
#include <mutex>
#include <uv.h>


namespace topgg {
  class http_frontend;
  class http_request;

  struct http_backend_pending_write {
    std::vector<uint8_t> data{};
    size_t offset{};
  };

  class http_backend {
    http_frontend* m_frontend{};
    waker m_waker{};
    std::atomic<bool> m_shutdown{};
#ifdef _WIN32
    wsastartup_guard m_wsastartup_guard{};
#endif
    uv_loop_t* m_loop{nullptr};
    SSL_CTX* m_ssl_context{nullptr};
    SSL* m_ssl{nullptr};
    BIO* m_ssl_read_bio{nullptr};
    BIO* m_ssl_write_bio{nullptr};
    struct addrinfo* m_addrs{nullptr};
    uv_tcp_t m_socket{};
    uv_connect_t m_socket_connection{};
    bool m_shook_hand{false};
    nghttp2_session* m_nghttp2{nullptr};
    std::deque<struct http_backend_pending_write> m_pending_writes{};
    std::vector<http_request*> m_ongoing_requests{};
    std::optional<exception> m_error{std::nullopt};

    inline http_backend(http_frontend* frontend): m_frontend(frontend) {};

    http_backend() = delete;

    inline void socket_throw(const topgg::exception& error) {
      m_error = error;

      shutdown();
    }
    
    void init();

    void connect();

    static void on_read(uv_stream_t* stream, ssize_t read_length, const uv_buf_t* buf);

    static void on_connect(uv_connect_t* connection, int status);

    void flush();

    void flush_requests();

    static nghttp2_ssize on_send(nghttp2_session* http2, const uint8_t* data, size_t length, int flags, void* ptr);

    static int on_header(nghttp2_session* http2, const nghttp2_frame* frame, const uint8_t* name, size_t name_length, const uint8_t* value, size_t value_length, uint8_t flags, void* ptr);

    static int on_data_chunk(nghttp2_session* http2, uint8_t flags, int32_t stream_id, const uint8_t* data, size_t length, void* ptr);

    void dispatch(topgg::http_request* request, uv_work_cb work_callback);

    static int on_stream_close(nghttp2_session* http2, int32_t stream_id, uint32_t error, void* ptr);

    static void on_close(uv_handle_t* handle);

    void loop();

    void shutdown(const bool blocking = false);

    ~http_backend();

    friend class http_frontend;
  };

  class http_request {
    nghttp2_nv m_headers[9];
    std::string m_path{};
    std::string m_content_length{};
    std::string m_body{};
    size_t m_body_remaining{};
    std::function<void(const uint16_t, const std::string_view&)> m_callback{};
    std::function<void(const exception&)> m_error_callback{};
    uint16_t m_status_code{};
    std::string m_response{};

    http_request(const std::string_view& authorization, const std::string_view& method, const std::string& path, const std::string& body, const std::function<void(const uint16_t, const std::string_view&)>& callback, const std::function<void(const exception&)> error_callback);

    void set_header(const size_t index, const std::string_view& name, const std::string_view& value);

    static ssize_t on_body_read(nghttp2_session* session, int32_t stream_id, uint8_t* data, size_t length, uint32_t* flags, nghttp2_data_source* source, void* ptr);

    inline void dispatch() {
      m_callback(m_status_code, m_response);
    }

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
    ~http_request();
#endif

  public:
    http_request() = delete;

    friend class http_backend;
    friend class http_frontend;
  };

  class http_frontend {
    http_backend* m_backend{nullptr};
    std::thread m_backend_thread{};
    std::vector<http_request*> m_requests{};
    std::mutex m_requests_mutex{};
    std::string m_authorization{};
  
  public:
    http_frontend(const std::string& token);

    http_frontend() = delete;

    ~http_frontend();

    void fetch(const std::string_view& method, const std::string& path, const std::string& body, const std::function<void(const uint16_t, const std::string_view&)>& callback, const std::function<void(const exception&)> error_callback, const bool defer = false);

    friend class http_backend;
  };
};
