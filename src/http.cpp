#include <topgg/exception.h>
#include <openssl/err.h>
#include <topgg/debug.h>
#include <topgg/http.h>
#include <topgg/util.h>
#include <algorithm>
#include <cstring>
#include <vector>

#if (defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)) && defined(TOPGG_SSLKEYLOG)
#include <cstdio>
#endif


void topgg::http_backend::init() {
  m_async_close.data = nullptr;

  if ((m_loop = uv_default_loop()) == nullptr) {
    throw topgg::exception{"Unable to retrieve default backend loop"};
  }

#ifdef _WIN32
  m_wsastartup_guard.init();
#endif

  SSL_library_init();
  SSL_load_error_strings();

  if ((m_ssl_context = SSL_CTX_new(TLS_client_method())) == nullptr) {
    throw topgg::exception::ssl("Unable to create SSL context");
  }

  SSL_CTX_set_options(m_ssl_context, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

  if (SSL_CTX_set_alpn_protos(m_ssl_context, (const unsigned char*)"\x02h2", 3) != 0) {
    throw topgg::exception::ssl("Unable to configure SSL context to use HTTP/2");
  }
#ifdef _WIN32
  else if (SSL_CTX_load_verify_store(m_ssl_context, "org.openssl.winstore:") == 0) {
    throw topgg::exception::ssl("Unable to configure SSL context to use Windows System Certificate Store");
  }
#endif

#if (defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)) && defined(TOPGG_SSLKEYLOG)
  SSL_CTX_set_keylog_callback(m_ssl_context, [](const SSL* ssl, const char* line) {
    auto keylog{fopen(TOPGG_SSLKEYLOG, "a")};

    if (keylog != nullptr) {
      fprintf(keylog, "%s\n", line);
      fclose(keylog);
    }
  });
#endif

  SSL_CTX_set_verify(m_ssl_context, SSL_VERIFY_PEER, nullptr);

  if (SSL_CTX_set_default_verify_paths(m_ssl_context) == 0) {
    throw topgg::exception::ssl("Unable to configure SSL context to perform certificate verification");
  } else if ((m_ssl = SSL_new(m_ssl_context)) == nullptr || SSL_set_tlsext_host_name(m_ssl, "top.gg") == 0 || SSL_set1_dnsname(m_ssl, "top.gg") == 0) {
    throw topgg::exception::ssl("Unable to create and configure SSL instance");
  }

  struct addrinfo hints{};

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  int status{};

  if (getaddrinfo("top.gg", "443", &hints, &m_addrs) < 0 || m_addrs == nullptr) {
    throw topgg::exception{"Unable to retrieve top.gg's IP address"};
  } else if ((status = uv_tcp_init(m_loop, &m_socket)) < 0) {
    throw topgg::exception::uv("Unable to create TCP socket", status);
  } else if ((status = uv_async_init(m_loop, &m_async_close, topgg::http_backend::on_async_close)) < 0) {
    throw topgg::exception::uv("Unable to create UV async close", status);
  }

  m_socket.data = this;
  m_socket_connection.data = this;
  m_async_close.data = this;
}

void topgg::http_backend::connect() {
  TOPGG_LOG("Connecting to Top.gg");

  uv_tcp_connect(&m_socket_connection, &m_socket, m_addrs->ai_addr, topgg::http_backend::on_connect);
}

void topgg::http_backend::on_read(uv_stream_t* stream, ssize_t read_length, const uv_buf_t* buf) {
  TOPGG_LOGF("[EVENT: UV TCP READ] %d bytes", read_length);

  const auto self{reinterpret_cast<topgg::http_backend*>(stream->data)};

  if (read_length < 0) {
    if (read_length == UV_EOF) {
      self->shutdown();
    } else {
      self->socket_throw(topgg::exception::uv("Unable to continue reading from socket stream", static_cast<int>(read_length)));
    }
  } else if (read_length > 0) {
    if (const auto status{BIO_write(self->m_ssl_read_bio, buf->base, static_cast<int>(read_length))}; status != read_length) {
      self->socket_throw(topgg::exception::ssl("Unable to write entire socket stream data to BIO"));
    } else {
      self->flush_requests();
    }
  }

  delete[] buf->base;
}

void topgg::http_backend::on_connect(uv_connect_t* connection, int status) {
  TOPGG_LOGF("[EVENT: UV TCP CONNECT] status: %d", status);

  const auto self{reinterpret_cast<topgg::http_backend*>(connection->data)};

  self->m_open_requests = static_cast<size_t>(-1);

  if (status < 0) {
    return self->socket_throw(topgg::exception::uv("Unable to perform TCP handshake with Top.gg", status));
  } else if ((self->m_ssl_read_bio = BIO_new(BIO_s_mem())) == nullptr || (self->m_ssl_write_bio = BIO_new(BIO_s_mem())) == nullptr) {
    return self->socket_throw(topgg::exception::ssl("Unable to create SSL I/O"));
  }

  SSL_set_bio(self->m_ssl, self->m_ssl_read_bio, self->m_ssl_write_bio);
  SSL_set_connect_state(self->m_ssl);

  BIO_set_conn_hostname(self->m_ssl_read_bio, "top.gg" ":443");
  BIO_set_conn_hostname(self->m_ssl_write_bio, "top.gg" ":443");

  if ((status = uv_read_start(reinterpret_cast<uv_stream_t*>(&self->m_socket), [](uv_handle_t* handle, size_t length, uv_buf_t* buf) {
    buf->base = new char[length];
    buf->len = static_cast<unsigned long>(length);
  }, topgg::http_backend::on_read)) < 0) {
    return self->socket_throw(topgg::exception::uv("Unable to start reading socket stream", status));
  };

  self->flush_requests();
}

void topgg::http_backend::flush() {
  TOPGG_LOG("Flushing pending I/O bytes");

  unsigned char buf[8 * 1024];

  while (BIO_pending(m_ssl_write_bio) > 0) {
    const auto read_length{BIO_read(m_ssl_write_bio, buf, sizeof(buf))};

    if (read_length <= 0) {
      break;
    }

    auto* raw_uv_buf{new std::vector<unsigned char>(buf, buf + read_length)};
    auto request{new uv_write_t};

    request->data = raw_uv_buf;

    uv_buf_t uv_buf{};

    uv_buf.base = reinterpret_cast<char*>(raw_uv_buf->data());
    uv_buf.len = static_cast<unsigned long>(raw_uv_buf->size());

    const auto status{uv_write(
      request,
      reinterpret_cast<uv_stream_t*>(&m_socket),
      &uv_buf,
      1,
      [](uv_write_t* request, int status) {
        delete reinterpret_cast<std::vector<unsigned char>*>(request->data);
        delete request;
      }
    )};

    if (status < 0) {
      delete raw_uv_buf;
      delete request;
      return socket_throw(topgg::exception::uv("Unable to perform SSL handshake with Top.gg", status));
    }
  }
}

void topgg::http_backend::flush_requests() {
  if (!m_shook_hand) {
    if (const auto handshake_status{SSL_do_handshake(m_ssl)}; handshake_status == 1) {
      TOPGG_LOG("Handshake successful");

      auto status{SSL_get_verify_result(m_ssl)};

      if (status != X509_V_OK) {
        return socket_throw("Unable to verify certificate");
      }

      const unsigned char* alpn{nullptr};
      unsigned int alpn_length{};

      SSL_get0_alpn_selected(m_ssl, &alpn, &alpn_length);

      if (alpn == nullptr || alpn_length != 2 || memcmp("h2", alpn, 2) != 0) {
        return socket_throw("Unable to negotiate HTTP/2");
      } else if ((status = uv_tcp_nodelay(&m_socket, 1)) < 0) {
        return socket_throw(topgg::exception::uv("Unable to configure TCP file descriptor to no delay", status));
      }

      nghttp2_session_callbacks* callbacks{nullptr};

      if (nghttp2_session_callbacks_new(&callbacks) < 0) {
        return socket_throw("Out of memory");
      }

      nghttp2_session_callbacks_set_send_callback2(callbacks, topgg::http_backend::on_send);
      nghttp2_session_callbacks_set_on_header_callback(callbacks, topgg::http_backend::on_header);
      nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, topgg::http_backend::on_data_chunk);
      nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, topgg::http_backend::on_stream_close);

      status = nghttp2_session_client_new(&m_nghttp2, callbacks, this);
      nghttp2_session_callbacks_del(callbacks);

      if (status < 0) {
        return socket_throw("Out of memory");
      }

      nghttp2_settings_entry settings[] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
      };

      if ((status = nghttp2_submit_settings(m_nghttp2, NGHTTP2_FLAG_NONE, settings, 1)) < 0) {
        return socket_throw(topgg::exception::nghttp2("Unable to submit HTTP request settings", status));
      }

      m_shook_hand = true;
      m_open_requests = 0;
    } else {
      if (const auto handshake_error{SSL_get_error(m_ssl, handshake_status)}; handshake_error != SSL_ERROR_WANT_READ && handshake_error != SSL_ERROR_WANT_WRITE) {
        return socket_throw(topgg::exception::ssl("Unable to perform SSL handshake with Top.gg"));
      }

      TOPGG_LOG("Flushing handshake");

      return flush();
    }
  }

  TOPGG_LOG("Flushing pending requests");

  int status{};

  {
    std::lock_guard _guard{m_frontend->m_requests_mutex};

    while (!m_frontend->m_requests.empty()) {
      const auto request{m_frontend->m_requests[m_frontend->m_requests.size() - 1]};

      m_frontend->m_requests.pop_back();
      m_open_requests++;

      nghttp2_data_provider2 body_provider{};

      body_provider.source.ptr = request;
      body_provider.read_callback = topgg::http_request::on_body_read;

      if ((status = nghttp2_submit_request2(
        m_nghttp2,
        nullptr,
        request->m_headers,
        9,
        request->m_body.empty() ? nullptr : &body_provider,
        request
      )) < 0) {
        delete request;
        return socket_throw(topgg::exception::nghttp2("Unable to submit HTTP request", status));
      }

      m_ongoing_requests.push_back(request);
    }
  };

  TOPGG_LOG("Processing data from the remote peer");

  char buf[8 * 1024];
  int read_length{};

  while ((read_length = SSL_read(m_ssl, buf, sizeof(buf))) > 0) {
    if ((status = static_cast<int>(nghttp2_session_mem_recv2(
      m_nghttp2,
      reinterpret_cast<uint8_t*>(buf),
      read_length
    ))) < 0) {
      return socket_throw(topgg::exception::nghttp2("Unable to process data from the remote peer", status));
    }
  }

  const auto error{SSL_get_error(m_ssl, read_length)};

  if (error != SSL_ERROR_ZERO_RETURN && error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE) {
    return socket_throw(topgg::exception::ssl("Unable to process data from the remote peer"));
  }

  TOPGG_LOG("Sending pending frames to the remote peer");

  if ((status = nghttp2_session_send(m_nghttp2)) < 0) {
    return socket_throw(topgg::exception::nghttp2("Unable to send pending frames to the remote peer", status));
  }
}

nghttp2_ssize topgg::http_backend::on_send(nghttp2_session* http2, const uint8_t* data, size_t length, int flags, void* ptr) {
  TOPGG_LOGF("[EVENT: NGHTTP2 SEND] %d bytes", length);

  auto self{reinterpret_cast<topgg::http_backend*>(ptr)};

  self->m_pending_writes.push_back({std::vector<uint8_t>{data, data + length}, 0});

  while (!self->m_pending_writes.empty()) {
    auto& pending{self->m_pending_writes.front()};
    const auto write_length{SSL_write(self->m_ssl, pending.data.data() + pending.offset, static_cast<int>(pending.data.size() - pending.offset))};

    if (write_length > 0) {
      pending.offset += write_length;

      if (pending.offset == pending.data.size()) {
        self->m_pending_writes.pop_front();
      }

      continue;
    }

    const auto error{SSL_get_error(self->m_ssl, write_length)};

    if (error == SSL_ERROR_WANT_WRITE || error == SSL_ERROR_WANT_READ) {
      TOPGG_LOGF("Temporarily pausing SSL_write from error %d", error);

      return NGHTTP2_ERR_WOULDBLOCK;
    }

    self->socket_throw(topgg::exception::ssl("Unable to write SSL data"));

    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  TOPGG_LOG("Flushing SSL_write");

  self->flush();

  return static_cast<nghttp2_ssize>(length);
}

int topgg::http_backend::on_header(nghttp2_session* http2, const nghttp2_frame* frame, const uint8_t* name, size_t name_length, const uint8_t* value, size_t value_length, uint8_t flags, void* ptr) {
  if (frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
    TOPGG_LOGF("[EVENT: NGHTTP2 STREAM %d HEADER] %.*s: %.*s", frame->hd.stream_id, name_length, name, value_length, value);

    if (name_length == 7 && memcmp(name, ":status", 7) == 0 && value_length >= 3) {
      auto request{reinterpret_cast<topgg::http_request*>(nghttp2_session_get_stream_user_data(reinterpret_cast<topgg::http_backend*>(ptr)->m_nghttp2, frame->hd.stream_id))};

      request->m_status = (static_cast<uint16_t>(value[0] - '0') * 100) + (static_cast<uint16_t>(value[1] - '0') * 10) + static_cast<uint16_t>(value[2] - '0');
    }
  }

  return 0;
}

int topgg::http_backend::on_data_chunk(nghttp2_session* http2, uint8_t flags, int32_t stream_id, const uint8_t* data, size_t length, void* ptr) {
  TOPGG_LOGF("[EVENT: NGHTTP2 STREAM %d BODY CHUNK] %d bytes", stream_id, length);

  auto request{reinterpret_cast<topgg::http_request*>(nghttp2_session_get_stream_user_data(reinterpret_cast<topgg::http_backend*>(ptr)->m_nghttp2, stream_id))};

  request->m_response.append(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), length);

  return 0;
}

void topgg::http_backend::dispatch(topgg::http_request* request, uv_work_cb work_callback) {
  auto work{new uv_work_t};

  work->data = request;

  uv_queue_work(m_loop, work, work_callback, [](uv_work_t* work, int status) {
    delete reinterpret_cast<topgg::http_request*>(work->data);
    delete work;
  });
}

int topgg::http_backend::on_stream_close(nghttp2_session* http2, int32_t stream_id, uint32_t error, void* ptr) {
  TOPGG_LOGF("[EVENT: NGHTTP2 STREAM %d CLOSE] error: %d", stream_id, error);

  auto self{reinterpret_cast<topgg::http_backend*>(ptr)};
  auto request{reinterpret_cast<topgg::http_request*>(nghttp2_session_get_stream_user_data(self->m_nghttp2, stream_id))};

  self->m_ongoing_requests.erase(std::remove(self->m_ongoing_requests.begin(), self->m_ongoing_requests.end(), request), self->m_ongoing_requests.end());

  if (error == 0) {
    self->dispatch(request, [](uv_work_t* work) {
      reinterpret_cast<topgg::http_request*>(work->data)->dispatch_body();
    });
  } else {
    self->dispatch(request, [](uv_work_t* work) {
      reinterpret_cast<topgg::http_request*>(work->data)->dispatch_exception("nghttp2 stream closed with non-zero error code");
    });
  }

  if (self->m_open_requests > 0) {
    self->m_open_requests--;
  }

  if (self->m_open_requests == 0 && self->m_shutdown.load(std::memory_order::memory_order_relaxed)) {
    self->shutdown();
  }

  return 0;
}

void topgg::http_backend::on_async_close(uv_async_t* handle) {
  TOPGG_LOG("[EVENT: ASYNC CLOSE]");

  auto request{reinterpret_cast<topgg::http_backend*>(handle->data)};

  if (request->m_open_requests == 0) {
    request->shutdown();
  }

  uv_close(reinterpret_cast<uv_handle_t*>(handle), nullptr);
}

void topgg::http_backend::on_close(uv_handle_t* handle) {
  TOPGG_LOG("Freeing up backend (1/2)");

  auto self{reinterpret_cast<topgg::http_backend*>(handle->data)};

  if (self->m_ssl != nullptr) {
    SSL_shutdown(self->m_ssl);
    self->m_ssl = nullptr;
  }

  if (self->m_nghttp2 != nullptr) {
    nghttp2_session_del(self->m_nghttp2);
    self->m_nghttp2 = nullptr;
  }

  if (self->m_ssl_write_bio != nullptr) {
    BIO_free(self->m_ssl_write_bio);
    self->m_ssl_write_bio = nullptr;
  }

  if (self->m_ssl_read_bio != nullptr) {
    BIO_free(self->m_ssl_read_bio);
    self->m_ssl_read_bio = nullptr;
  }

  self->m_shook_hand = false;
}

void topgg::http_backend::loop() {
  TOPGG_LOG("Running libuv event loop");

  uv_run(m_loop, UV_RUN_DEFAULT);
}

void topgg::http_backend::shutdown(const bool blocking) {
  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_socket))) {
    TOPGG_LOGF("Shutting down backend (blocking: %d)", blocking);

    {
      std::lock_guard _guard{m_frontend->m_requests_mutex};

      while (!m_frontend->m_requests.empty()) {
        const auto request{m_frontend->m_requests[m_frontend->m_requests.size() - 1]};

        m_frontend->m_requests.pop_back();

        if (m_error.has_value()) {
          request->dispatch_exception(m_error.value());
        }

        delete request;
      }
    };

    while (!m_ongoing_requests.empty()) {
      const auto request{m_ongoing_requests[m_ongoing_requests.size() - 1]};

      m_ongoing_requests.pop_back();

      if (m_error.has_value()) {
        request->dispatch_exception(m_error.value());
      }

      delete request;
    }

    if (m_nghttp2 != nullptr) {
      nghttp2_session_terminate_session(m_nghttp2, NGHTTP2_NO_ERROR);
      nghttp2_session_send(m_nghttp2);
    }

    uv_close(reinterpret_cast<uv_handle_t*>(&m_socket), http_backend::on_close);

    if (blocking) {
      loop();
    }
  }
}

topgg::http_backend::~http_backend() {
  TOPGG_LOG("Freeing up backend (2/2)");

  shutdown(true);

  if (m_addrs != nullptr) {
    freeaddrinfo(m_addrs);
    m_addrs = nullptr;
  }

  if (m_ssl_context != nullptr) {
    SSL_CTX_free(m_ssl_context);
    m_ssl_context = nullptr;
  }

  if (m_loop != nullptr) {
    uv_loop_close(m_loop);
    m_loop = nullptr;
  }
}

topgg::http_request::http_request(const std::string_view& authorization, const std::string_view& method, const std::string& path, const topgg::http_request_callback& callback, const std::string& body): m_path("/api/v1" + path), m_content_length(std::to_string(body.size())), m_body(body), m_body_remaining(body.size()), m_callback(callback) {  
  set_header(0, ":method", method);
  set_header(1, ":scheme", "https");
  set_header(2, ":authority", "top.gg");
  set_header(3, ":path", m_path);
  set_header(4, "accept", "application/json");
  set_header(5, "authorization", authorization);
  set_header(6, "content-length", m_content_length);
  set_header(7, "content-type", "application/json");
  set_header(8, "user-agent", "topgg-cpp-sdk");
}

void topgg::http_request::set_header(const size_t index, const std::string_view& name, const std::string_view& value) {
  auto header{&m_headers[index]};

  header->flags = NGHTTP2_NV_FLAG_NO_COPY_NAME | NGHTTP2_NV_FLAG_NO_COPY_VALUE;
  header->name = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(name.data()));
  header->namelen = name.length();
  header->value = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(value.data()));
  header->valuelen = value.length();
}

ssize_t topgg::http_request::on_body_read(nghttp2_session* session, int32_t stream_id, uint8_t* data, size_t length, uint32_t* flags, nghttp2_data_source* source, void* ptr) {
  auto self{reinterpret_cast<topgg::http_request*>(ptr)};
  auto to_send{min(self->m_body_remaining, length)};

  TOPGG_LOGF("[EVENT: NGHTTP2 REQUEST BODY READ] sending %d bytes", to_send);

  if (to_send > 0) {
    memcpy(data, self->m_body.data() + (self->m_body.size() - self->m_body_remaining), to_send);
    self->m_body_remaining -= to_send;
  }

  if (self->m_body_remaining == 0) {
    *flags |= NGHTTP2_DATA_FLAG_EOF;
  }

  return to_send;
}

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
topgg::http_request::~http_request() {
  TOPGG_LOG("Freeing up request");
}
#endif

topgg::http_frontend::http_frontend(const std::string& token): m_authorization("Bearer " + token) {
  topgg::waker frontend_waker{};
  std::optional<topgg::exception> backend_init_error{std::nullopt};

  m_backend_thread = std::thread([this, &frontend_waker, &backend_init_error]() {
    TOPGG_LOG("Started backend thread");

    topgg::http_backend backend{this};

    m_backend = &backend;

    try {
      backend.init();
    } catch (const topgg::exception& error) {
      backend_init_error = error;
    }

    frontend_waker.notify();

    while (1) {
      backend.m_waker.wait();

      if (backend.m_shutdown.load(std::memory_order::memory_order_relaxed)) {
        break;
      }

      backend.connect();
      backend.loop();
    }
  });

  frontend_waker.wait();

  TOPGG_LOGF("Backend set up with error: %d", backend_init_error.has_value());

  if (backend_init_error.has_value()) {
    m_backend_thread.join();

    throw backend_init_error.value();
  }
}

void topgg::http_frontend::set_token(const std::string& token) {
  m_authorization = "Bearer " + token;
}

void topgg::http_frontend::fetch(const std::string_view& method, const std::string& path, const topgg::http_request_callback& callback, const bool defer, const std::string& body) {
  auto request{new topgg::http_request{m_authorization, method, path, callback, body}};

  {
    std::lock_guard _guard{m_requests_mutex};

    m_requests.push_back(request);
  };

  if (!defer) {
    m_backend->m_waker.notify();
  }
}

topgg::http_frontend::~http_frontend() {
  TOPGG_LOG("Freeing up frontend");

  m_backend->m_shutdown.store(true, std::memory_order::memory_order_relaxed);

  if (m_backend->m_async_close.data != nullptr) {
    uv_async_send(&m_backend->m_async_close);
  }

  m_backend->m_waker.notify();
  m_backend_thread.join();
}